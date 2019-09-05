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

from projectq.cengines import BasicEngine
from projectq.ops import AllocateQubitGate, FastForwardingGate, ZGate, Command, FlushGate, BasicGate, \
    DeallocateQubitGate
from projectq.types import BasicQubit, WeakQubitRef

from hiq.projectq.cengines import SwapScheduler, ClusterScheduler
from hiq.projectq.ops import MetaSwap, AllocateQuregGate
"""
Contains the projectq interface to a C++-based simulator, which has to be
built first. If the c++ simulator is not exported to python, a (slow) python
implementation is used as an alternative.
"""

BasicGate.is_diagonal = lambda _: False


class GreedyScheduler(BasicEngine):
    """
    Greedy Scheduler is a new method optimizing simulator performance
    not previously described.

    Key concepts:
        There are local and global qubits. If gate is acting on local qubits
        matrix-vector product can be calculated without access to non-local parts.
        To apply gate to global qubits one need to make them local, i.e. reorder
        qubits.

        Gates are reordered into sequences called stages. A stage contains gates
        acting on local qubits. Inside the stage gates form subsequences called
        clusters. Gates from the same cluster are fused into single multi-qubit
        gate and this gate is simulated by single matrix-vector multiplication.
        Between stages qubit reordering occur.

    Greedy algorithm calculates a permutation of gates and permutation
    of qubits which lead to minimum number of clusters in a stage and minimum number
    of stages during quantum circuit simulation.

    Example:
        Shor MPI creates the main engine using Greedy Scheduler.

    .. code-block:: python

       compilerengines = [GreedyScheduler()]
       simulator = SimulatorMPI(gate_fusion=True, num_local_qubits=20)

       # make the compiler and use the SimulatorMPI as a backend
       eng = HiQMainEngine(simulator, compilerengines)

       # In the above line HiQMainEngine can be replaced by MainEngine from
       # ProjectQ.

    Note:
        Greedy Scheduler should be the last engine in the list.

    """
    def __init__(self, supremacy_circuit=False, num_splits=10 ** 6, cluster_size=4):
        """
        Args:
            supremacy_circuit (bool): If you want to use random circuits, you can
                specify this parameter as True. Then all last CZ gates will be
                ignored, because they do not affect the result of final measurement.
            num_splits (int): Number of branch splits
            cluster_size (int): Maximum number of qubits in fused multi-qubit
                gate
        """
        BasicEngine.__init__(self)
        self._cmd_list = []
        self._was_scheduling = False
        self._init = False
        self._supremacy_circuit = supremacy_circuit
        self.NUM_SPLITS = num_splits
        self.CLUSTER_SIZE = cluster_size
        self._deallocations_cache = []

    def _from_id_to_qubit(self, id):
        return BasicQubit(self.main_engine, id)

    def _cache_cmd(self, cmd):
        self._cmd_list.append(cmd)

    def _prepare_ctrlz(self):
        local_qubits = self._get_local_ids_list_from_backend()
        global_qubits = self._get_global_ids_list_from_backend()

        for cmd in self._cmd_list:
            if isinstance(cmd.gate, ZGate):
                assert len(cmd.qubits) == 1 and len(cmd.qubits[0]) == 1
                ctrls = cmd.control_qubits
                other = cmd.qubits[0]

                if other[0].id in global_qubits:
                    for i in range(len(ctrls)):
                        if ctrls[i].id in local_qubits:
                            ctrls[i], other[0] = other[0], ctrls[i]
                            break

                cmd.qubits = (other,)
                cmd.control_qubits = ctrls

    def _get_commands(self):
        return [[qubit.id for sublist in cmd.qubits for qubit in sublist] for cmd in self._cmd_list], \
               [[qubit.id for qubit in cmd.control_qubits] for cmd in self._cmd_list], \
               [cmd.gate.is_diagonal() for cmd in self._cmd_list]

    def _call_cluster_scheduler(self):
        self._prepare_ctrlz()
        local_qubits = self._get_local_ids_list_from_backend()
        global_qubits = self._get_global_ids_list_from_backend()
        while True:
            gate, gate_ctrl, gate_diag = self._get_commands()
            cluster_scheduler = ClusterScheduler(gate, gate_ctrl, gate_diag, local_qubits, global_qubits,
                                                 self.CLUSTER_SIZE)
            avail = cluster_scheduler.ScheduleCluster()
            if len(avail) == 0:
                return
            for i in avail:
                self.send([self._cmd_list[i]])

            self.send([Command(self, FlushGate(), ([WeakQubitRef(self, -1)],))])

            for i in reversed(sorted(avail)):
                del self._cmd_list[i]
            # print("in", len(self._cmd_list))

    def _get_ids_list_from_backend(self):
        if hasattr(self.main_engine.backend, 'qubits_ids'):
            return self.main_engine.backend.qubits_ids
        else:
            return self.main_engine.backend.get_qubits_ids()

    def _get_local_ids_list_from_backend(self):
        return self.main_engine.backend.get_local_qubits_ids()

    def _get_global_ids_list_from_backend(self):
        return self.main_engine.backend.get_global_qubits_ids()

    def _remove_ending_cz(self):
        # print(len(self._cmd_list))
        i = len(self._cmd_list) - 1
        used = set()
        while i >= 0:
            bad = True
            if isinstance(self._cmd_list[i].gate, ZGate):
                for lst in self._cmd_list[i].all_qubits:
                    for qubit in lst:
                        if qubit.id in used:
                            bad = False
            else:
                bad = False

            if bad:
                self._cmd_list.pop(i)
            else:
                for lst in self._cmd_list[i].all_qubits:
                    for qubit in lst:
                        used.add(qubit.id)

            i -= 1
        # print(len(self._cmd_list))

    def _call_swap_scheduler(self):
        local_qubits = self._get_local_ids_list_from_backend()
        gate, gate_ctrl, gate_diag = self._get_commands()
        swap_scheduler = SwapScheduler(gate, gate_ctrl, gate_diag, self.NUM_SPLITS, len(local_qubits), True)
        new_locals = swap_scheduler.ScheduleSwap()

        if len(new_locals) == 0:
            swap_scheduler = SwapScheduler(gate, gate_ctrl, gate_diag, self.NUM_SPLITS, len(local_qubits), False)
            new_locals = swap_scheduler.ScheduleSwap()

        g_to_l = sorted(list(set(new_locals) - set(local_qubits)))
        l_to_g = []

        if len(g_to_l) > 0:
            lst = sorted(list(set(local_qubits) - set(new_locals)))
            assert len(lst) >= len(g_to_l)
            l_to_g = lst[:len(g_to_l)]

        return g_to_l, l_to_g

    def _check_commands(self):
        locals_size = len(self._get_local_ids_list_from_backend())
        for gate in self._get_commands()[0]:
            if locals_size < len(gate):
                raise Exception("Can't apply {}-qubits gate (only {} local qubits)".format(len(gate), locals_size))
            if len(gate) > 5:
                raise Exception("Can't apply {}-qubits gate (no more that 5 qubits allowed)".format(len(gate)))

    def _force_scheduling(self):
        if len(self._cmd_list) == 0:
            return

        self._check_commands()

        if self._supremacy_circuit:
            self._remove_ending_cz()

        if not self._was_scheduling:
            self._was_scheduling = True
            ids_list = self._get_ids_list_from_backend()
            g_to_l, l_to_g = self._call_swap_scheduler()

            for i in range(len(l_to_g)):
                p1 = ids_list.index(g_to_l[i])
                p2 = ids_list.index(l_to_g[i])
                ids_list[p1], ids_list[p2] = ids_list[p2], ids_list[p1]

            self.main_engine.backend.set_qubits_perm(ids_list)

        self._call_cluster_scheduler()

        while len(self._cmd_list) > 0:
            g_to_l, l_to_g = self._call_swap_scheduler()
            assert len(g_to_l) > 0

            pairs = []
            for i in range(len(g_to_l)):
                q0 = self._from_id_to_qubit(l_to_g[i])
                q1 = self._from_id_to_qubit(g_to_l[i])
                pairs += [q1, q0]

            new_cmd = MetaSwap.generate_command(pairs)
            self.send([new_cmd])

            self._call_cluster_scheduler()
            # print("out", len(self._cmd_list))

        assert len(self._cmd_list) == 0

    def _send_deallocations(self):
        for c in sorted(self._deallocations_cache, key=lambda cmd: cmd.qubits[0][0].id, reverse=True):
            self.send([c])

        del self._deallocations_cache[:]

    def receive(self, command_list):
        if not self._init:
            self._init = True

        for cmd in command_list:
            if isinstance(cmd.gate, DeallocateQubitGate):
                self._deallocations_cache.append(cmd)
            elif isinstance(cmd.gate, (AllocateQubitGate, AllocateQuregGate, FastForwardingGate)):
                self._force_scheduling()
                self._send_deallocations()
                self.send([cmd])
            else:
                if len(self._deallocations_cache) > 0:
                    self._force_scheduling()
                    self._send_deallocations()
                self._cache_cmd(cmd)
