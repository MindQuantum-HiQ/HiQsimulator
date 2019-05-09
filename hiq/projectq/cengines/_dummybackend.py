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

import sys
from projectq.cengines import LastEngineException, BasicEngine
from projectq.ops import *
from typing import List
from hiq.projectq.ops import AllocateQuregGate
from hiq.projectq.ops._gates import MetaSwapGate


class FakeSimulator:
    def __init__(self):
        pass

    def allocate_qureg(self):
        pass


class DummyBackend(BasicEngine):
    def __init__(self, cluster_size, num_global):
        BasicEngine.__init__(self)

        self._num_global = num_global
        self._max_cluster_size = cluster_size
        self._k_not_found = pow(2, 64) - 1

        self._swaps = 0
        self._swap_fraction = 0
        self._runs = 0
        self._gates = 0
        self._last_gates = 0

        self._locals = []
        self._globals = [self._k_not_found] * num_global
        self._cluster = set()

        self._is_diag_cluster = True
        self._is_empty_cluster = True
        self._nothing = True

        self._simulator = FakeSimulator()
        self._dealloc = False

    def set_qubits_perm(self, p):
        self._locals = p[:len(self._locals)]
        self._globals = p[-len(self._globals):]

    def get_qubits_ids(self):
        return self._locals + self._globals

    def get_local_qubits_ids(self):
        return self._locals[:]

    def get_global_qubits_ids(self):
        return self._globals[:]

    def is_available(self, cmd):
        try:
            return BasicEngine.is_available(self, cmd)
        except LastEngineException:
            return True

    def _do_swap(self, pairs):
        if len(pairs) == 0:
            return
        assert len(pairs) % 2 == 0

        self._swaps += 1
        self._swap_fraction += 1 - 1 / pow(2, len(pairs) // 2)

        for i in range(0, len(pairs), 2):
            if pairs[i] in self._locals:
                p1 = self._locals.index(pairs[i])
                p2 = self._globals.index(pairs[i + 1])
            else:
                p1 = self._locals.index(pairs[i + 1])
                p2 = self._globals.index(pairs[i])
            self._locals[p1], self._globals[p2] = self._globals[p2], self._locals[p1]

        print("[DUMMY] Doing swap: qubits = {}".format(len(pairs) // 2))

    def _fuse(self, cmd):
        self._gates += 1
        self._nothing = False

        is_diag = cmd.gate.is_diagonal()
        if not is_diag:
            self._is_diag_cluster = False
            self._is_empty_cluster = False

        for q in self._cmd_to_qubits(cmd):
            if self._is_global_qubit(q) and not is_diag:
                print("[DUMMY] Error: Tried to apply not diagonal gate to global qubit")
                exit(1)
            elif self._is_local_qubit(q):
                self._cluster.add(q)
                self._is_empty_cluster = False

        for q in self._cmd_to_ctrl_qubits(cmd):
            if self._is_local_qubit(q):
                self._is_empty_cluster = False
                self._cluster.add(q)

        if len(self._cluster) > self._max_cluster_size:
            print("[DUMMY] Error: Too big cluster")
            exit(1)

    def _run(self):
        last_gates = self._last_gates
        self._last_gates = self._gates

        if self._nothing:
            return

        self._runs += 1
        if self._is_empty_cluster:
            print("[DUMMY] Doing run: FACTOR; gates = {}".format(self._gates - last_gates))
            return

        if self._is_diag_cluster:
            s = "[DUMMY] Doing run: DIAGONAL matrix size = {}; ids = {}; gates = {}"
        else:
            s = "[DUMMY] Doing run: DEFAULT matrix size = {}; ids = {}; gates = {}"
        print(s.format(len(self._cluster), self._cluster, self._gates - last_gates))

    def receive(self, command_list):
        sys.stdout = sys.stderr
        for cmd in command_list:
            assert not self._dealloc or isinstance(cmd.gate, (DeallocateQubitGate, FlushGate))

            if isinstance(cmd.gate, MetaSwapGate):
                self._do_swap(self._cmd_to_all_qubits(cmd))
            elif isinstance(cmd.gate, FlushGate):
                self._run()
                self._clear()
            elif isinstance(cmd.gate, AllocateQubitGate):
                assert False
            elif isinstance(cmd.gate, AllocateQuregGate):
                for q in self._cmd_to_all_qubits(cmd):
                    if len(self._locals) < self._max_cluster_size:
                        self._locals.append(q)
                    else:
                        try:
                            pos = self._globals.index(self._k_not_found)
                            self._globals[pos] = q
                        except ValueError:
                            self._locals.append(q)
            elif isinstance(cmd.gate, DeallocateQubitGate):
                self._dealloc = True
            elif isinstance(cmd.gate, MeasureGate):
                pass
            else:
                self._fuse(cmd)
        sys.stdout.flush()
        sys.stdout = sys.__stdout__

    def _clear(self):
        self._cluster = set()
        self._is_diag_cluster = True
        self._is_empty_cluster = True
        self._nothing = True

    @staticmethod
    def _cmd_to_all_qubits(cmd):
        return [qb.id for qr in cmd.all_qubits for qb in qr]

    @staticmethod
    def _cmd_to_qubits(cmd):
        return [qb.id for qr in cmd.qubits for qb in qr]

    @staticmethod
    def _cmd_to_ctrl_qubits(cmd):
        return [qb.id for qb in cmd.control_qubits]

    def _is_local_qubit(self, i):
        return i in self.get_local_qubits_ids()

    def _is_global_qubit(self, i):
        return i in self.get_global_qubits_ids()

    def print_statistics(self):
        print("==================== STATS ====================")
        print("Gates:", self._gates)
        print("Swaps:", self._swaps)
        print("Runs:", self._runs)
        print()
        print("Swap_fraction:", self._swap_fraction)
        print("Gate / run:", self._gates / self._runs)
