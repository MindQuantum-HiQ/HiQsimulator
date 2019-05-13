#   Copyright 2019 <Huawei Technologies Co., Ltd>
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
"""
Tests for hiq.projectq.backends._sim._stabilizer_simulator.py
"""

import math
import random
import pytest

from projectq import MainEngine
from projectq.cengines import (BasicEngine, BasicMapperEngine, DummyEngine,
                               LocalOptimizer, NotYetMeasuredError)
from projectq.ops import (All, Allocate, CNOT, Command, H, Measure,
                          QubitOperator, S, Toffoli, X)
from projectq.meta import Control, LogicalQubitIDTag
from projectq.types import WeakQubitRef

from projectq.backends import Simulator
from hiq.projectq.backends import StabilizerSimulator


def test_is_cpp_simulator_present():
    import hiq.projectq.backends._sim._cppstabsim
    assert hiq.projectq.backends._sim._cppstabsim


@pytest.fixture(params=["mapper", "no_mapper"])
def mapper(request):
    """
    Adds a mapper which changes qubit ids by adding 1
    """
    if request.param == "mapper":

        class TrivialMapper(BasicMapperEngine):
            def __init__(self):
                BasicEngine.__init__(self)
                self.current_mapping = dict()

            def receive(self, command_list):
                for cmd in command_list:
                    for qureg in cmd.all_qubits:
                        for qubit in qureg:
                            if qubit.id == -1:
                                continue
                            elif qubit.id not in self.current_mapping:
                                previous_map = self.current_mapping
                                previous_map[qubit.id] = qubit.id + 1
                                self.current_mapping = previous_map
                    self._send_cmd_with_mapped_ids(cmd)

        return TrivialMapper()
    if request.param == "no_mapper":
        return None


def test_simulator_is_available():
    backend = DummyEngine(save_commands=True)
    eng = MainEngine(backend, [])
    qureg = eng.allocate_qureg(2)
    X | qureg[0]
    S | qureg[0]
    H | qureg[0]
    CNOT | (qureg[0], qureg[1])
    Measure | qureg[0]
    qureg[0].__del__()
    qureg[1].__del__()
    assert len(backend.received_commands) == 9

    # Test that allocate, measure, and deallocate are available.
    sim = StabilizerSimulator(3)
    for cmd in backend.received_commands:
        assert sim.is_available(cmd)

    eng = MainEngine(sim, [])
    qureg = eng.allocate_qureg(3)
    X | qureg[0]
    S | qureg[0]
    H | qureg[0]
    CNOT | (qureg[0], qureg[1])
    All(Measure) | qureg

    with pytest.raises(Exception):
        Toffoli | (qureg[0], qureg[1], qureg[2])
        eng.flush()

    with pytest.raises(Exception):
        with Control(eng, qureg[1:]):
            X | qureg[0]
        eng.flush()


def test_unavailable_methods():
    sim = StabilizerSimulator(2)
    eng = MainEngine(sim, [])
    qureg = eng.allocate_qureg(2)

    with pytest.raises(NotImplementedError):
        op = QubitOperator('Z2')
        eng.backend.apply_qubit_operator(op, qureg)

    with pytest.raises(NotImplementedError):
        op0 = QubitOperator('Z0')
        eng.backend.get_expectation_value(op0, qureg)

    with pytest.raises(NotImplementedError):
        eng.backend.get_amplitude('00', qureg)

    with pytest.raises(NotImplementedError):
        wf = [0., 0., math.sqrt(0.2), math.sqrt(0.8)]
        eng.backend.set_wavefunction(wf, qureg)

    with pytest.raises(NotImplementedError):
        eng.backend.cheat()


def test_simulator_functional_measurement():
    sim = StabilizerSimulator(5)
    eng = MainEngine(sim, [])
    qubits = eng.allocate_qureg(5)
    # entangle all qubits:
    H | qubits[0]
    for qb in qubits[1:]:
        CNOT | (qubits[0], qb)

    All(Measure) | qubits
    eng.flush()

    bit_value_sum = sum([int(qubit) for qubit in qubits])
    assert bit_value_sum == 0 or bit_value_sum == 5


def test_simulator_measure_mapped_qubit():
    sim = StabilizerSimulator(1)
    eng = MainEngine(sim, [])
    qb1 = WeakQubitRef(engine=eng, idx=1)
    qb2 = WeakQubitRef(engine=eng, idx=2)
    cmd0 = Command(engine=eng, gate=Allocate, qubits=([qb1], ))
    cmd1 = Command(engine=eng, gate=X, qubits=([qb1], ))
    cmd2 = Command(
        engine=eng,
        gate=Measure,
        qubits=([qb1], ),
        controls=[],
        tags=[LogicalQubitIDTag(2)])
    with pytest.raises(NotYetMeasuredError):
        int(qb1)
    with pytest.raises(NotYetMeasuredError):
        int(qb2)
    eng.send([cmd0, cmd1, cmd2])
    eng.flush()
    with pytest.raises(NotYetMeasuredError):
        int(qb1)
    assert int(qb2) == 1


def test_simulator_probability(mapper, capsys):
    sim = StabilizerSimulator(6)
    engine_list = [LocalOptimizer()]
    if mapper is not None:
        engine_list.append(mapper)
    eng = MainEngine(sim, engine_list=engine_list)
    qubits = eng.allocate_qureg(6)
    All(H) | qubits
    eng.flush()
    bits = [0, 0, 1, 0, 1, 0]
    for i in range(6):
        assert (eng.backend.get_probability(
            bits[:i], qubits[:i]) == pytest.approx(0.5**i))
    extra_qubit = eng.allocate_qubit()
    with pytest.raises(RuntimeError):
        eng.backend.get_probability([0], extra_qubit)
    del extra_qubit

    All(H) | qubits
    eng.flush()
    assert eng.backend.get_probability('0' * len(qubits),
                                       qubits) == pytest.approx(1.0)
    assert eng.backend.get_probability('1' * len(qubits),
                                       qubits) == pytest.approx(0.0)
    All(Measure) | qubits


def test_simulator_collapse_wavefunction(mapper):
    sim = StabilizerSimulator(16)
    engine_list = [LocalOptimizer()]
    if mapper is not None:
        engine_list.append(mapper)
    eng = MainEngine(sim, engine_list=engine_list)
    qubits = eng.allocate_qureg(4)
    # unknown qubits: raises
    with pytest.raises(RuntimeError):
        eng.backend.collapse_wavefunction(qubits, [0] * 4)
    eng.flush()
    eng.backend.collapse_wavefunction(qubits, [0] * 4)
    assert pytest.approx(eng.backend.get_probability([0] * 4, qubits)) == 1.
    All(H) | qubits[:-1]
    eng.flush()
    assert pytest.approx(eng.backend.get_probability([0] * 4, qubits)) == .125
    # impossible outcome: raises
    prob = eng.backend.get_probability([1, 0, 1, 0], qubits)
    with pytest.raises(RuntimeError):
        eng.backend.collapse_wavefunction(qubits, [0] * 3 + [1])
    # Make sure nothing the state of the simulator has not changed
    assert pytest.approx(prob) == eng.backend.get_probability([1, 0, 1, 0],
                                                              qubits)
    eng.backend.collapse_wavefunction(qubits[1:], [0, 1, 0])
    probability = eng.backend.get_probability([1, 0, 1, 0], qubits)
    assert probability == pytest.approx(.5)
    eng.backend.set_qubits(qubits, [0] * 3 + [1])
    H | qubits[0]
    CNOT | (qubits[0], qubits[1])
    eng.flush()
    eng.backend.collapse_wavefunction([qubits[0]], [1])
    probability = eng.backend.get_probability([1, 1], qubits[0:2])
    assert probability == pytest.approx(1.)


def test_simulator_no_uncompute_exception():
    sim = StabilizerSimulator(2)
    eng = MainEngine(sim, [])
    qubit = eng.allocate_qubit()
    H | qubit
    with pytest.raises(RuntimeError):
        qubit[0].__del__()
    # If you wanted to keep using the qubit, you shouldn't have deleted it.
    assert qubit[0].id == -1


class MockStabilizerSimulatorBackend(object):
    def __init__(self):
        self.run_cnt = 0

    def run(self):
        self.run_cnt += 1


def test_simulator_flush():
    sim = StabilizerSimulator(2)
    sim._simulator = MockStabilizerSimulatorBackend()

    eng = MainEngine(sim)
    eng.flush()

    assert sim._simulator.run_cnt == 1


def test_simulator_send():
    sim = StabilizerSimulator(2)
    backend = DummyEngine(save_commands=True)

    eng = MainEngine(backend, [sim])

    qubit = eng.allocate_qubit()
    H | qubit
    Measure | qubit
    del qubit
    eng.flush()

    assert len(backend.received_commands) == 5


def test_simulator_functional_entangle():
    sim = StabilizerSimulator(6)
    eng = MainEngine(sim, [])
    qubits = eng.allocate_qureg(5)
    # entangle all qubits:
    H | qubits[0]
    for qb in qubits[1:]:
        CNOT | (qubits[0], qb)

    # Check the probabilities
    eng.flush()
    assert eng.backend.get_probability('0' * len(qubits),
                                       qubits) == pytest.approx(0.5)
    assert eng.backend.get_probability('1' * len(qubits),
                                       qubits) == pytest.approx(0.5)

    # Untangle all qubits
    for qb in qubits[-1::-1]:
        CNOT | (qubits[0], qb)
    H | qubits[0]

    # Check the probabilities
    eng.flush()
    assert eng.backend.get_probability('0' * len(qubits),
                                       qubits) == pytest.approx(1.0)
    assert eng.backend.get_probability('1' * len(qubits),
                                       qubits) == pytest.approx(0.0)

    All(Measure) | qubits


def test_simulator_convert_logical_to_mapped_qubits():
    sim = StabilizerSimulator(2)
    mapper = BasicMapperEngine()

    def receive(command_list):
        pass

    mapper.receive = receive
    eng = MainEngine(sim, [mapper])
    qubit0 = eng.allocate_qubit()
    qubit1 = eng.allocate_qubit()
    mapper.current_mapping = {
        qubit0[0].id: qubit1[0].id,
        qubit1[0].id: qubit0[0].id
    }
    assert (sim._convert_logical_to_mapped_qureg(qubit0 + qubit1) == qubit1 +
            qubit0)


def test_compare_simulators():
    num_cycles = 20
    num_qubits = 16
    num_gates = 100
    num_probability_check = 100

    for c in range(num_cycles):
        sim = Simulator()
        eng = MainEngine(sim, [])
        qureg = eng.allocate_qureg(num_qubits)

        sim_stab = StabilizerSimulator(num_qubits)
        eng_stab = MainEngine(sim_stab, [])
        qureg_stab = eng_stab.allocate_qureg(num_qubits)

        for gate in range(num_gates):
            r = random.randint(0, 4)
            qb0 = random.randint(0, num_qubits - 1)
            if r == 0:
                H | qureg[qb0]
                H | qureg_stab[qb0]
            elif r == 1:
                S | qureg[qb0]
                S | qureg_stab[qb0]
            elif r == 1:
                X | qureg[qb0]
                X | qureg_stab[qb0]
            elif r == 1:
                qb1 = random.randint(0, num_qubits - 1)
                CNOT | (qureg[qb0], qureg[qb1])
                CNOT | (qureg_stab[qb0], qureg_stab[qb1])
        eng.flush()
        eng_stab.flush()

        for p in range(num_probability_check):
            qubit_ids = set()
            qubits = []
            qubits_stab = []

            num_qubits_for_check = random.randint(1, num_qubits)
            while len(qubits) < num_qubits_for_check:
                qb = random.randint(0, num_qubits - 1)
                if qb not in qubit_ids:
                    qubit_ids.add(qb)
                    qubits.append(qureg[qb])
                    qubits_stab.append(qureg_stab[qb])

            bitstring = [
                random.choice('01') for _ in range(num_qubits_for_check)
            ]

            prob = eng.backend.get_probability(bitstring, qubits)
            assert pytest.approx(prob) == eng_stab.backend.get_probability(
                bitstring, qubits_stab)

            if prob > 0:
                eng.backend.collapse_wavefunction(qubits, bitstring)
                eng_stab.backend.collapse_wavefunction(qubits, bitstring)
                assert eng.backend.get_probability(bitstring,
                                                   qubits) == pytest.approx(1)
                assert eng_stab.backend.get_probability(
                    bitstring, qubits) == pytest.approx(1)
            else:
                with pytest.raises(RuntimeError):
                    eng.backend.collapse_wavefunction(qubits, bitstring)
                with pytest.raises(RuntimeError):
                    eng_stab.backend.collapse_wavefunction(qubits, bitstring)

        All(Measure) | qureg
        All(Measure) | qureg_stab
