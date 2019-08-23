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

from projectq import MainEngine
from projectq.ops import Command, Deallocate
from projectq.types import Qureg, Qubit, WeakQubitRef

from hiq.projectq.ops import AllocateQuregGate


class HiQMainEngine(MainEngine):
    """
    HiQ Main Engine provides all functionality of the main compiler engine.
    It is an extension of ProjectQ MainEngine adopted for using with
    SimulatorMPI.

    """
    def allocate_qureg(self, n, init=0.0):
        """
        Allocate n qubits and return them as a quantum register, which is a
        list of qubit objects.

        Args:
            n (int): Number of qubits to allocate
            init (complex): Assign this value to every amplitude
        Returns:
            Qureg of length n, a list of n newly allocated qubits.
        """
        cmd = Command(self, AllocateQuregGate(init), ())
        if self.backend.is_available(cmd):
            qubits = Qureg()
            for i in range(n):
                new_id = self.main_engine.get_new_qubit_id()
                qb = Qubit(self, new_id)
                qubits.append(qb)
                self.main_engine.active_qubits.add(qb)

            cmd = Command(self, AllocateQuregGate(init), (qubits,))
            self.send([cmd])
            return qubits
        else:
            return super(MainEngine, self).allocate_qureg(n)

    def deallocate_qubit(self, qubit):
        """
        Deallocate a qubit (and sends the deallocation command down the
        pipeline). If the qubit was allocated as a dirty qubit, add
        DirtyQubitTag() to Deallocate command.

        Args:
            qubit (BasicQubit): Qubit to deallocate.
        Raises:
            ValueError: Qubit already deallocated. Caller likely has a bug.
        """
        if qubit.id == -1:
            raise ValueError("Already deallocated.")

        is_dirty = qubit.id in self.dirty_qubits
        weak_copy = WeakQubitRef(qubit.engine, qubit.id)

        if not isinstance(qubit, WeakQubitRef):
            if qubit in self.active_qubits:
                self.active_qubits.remove(qubit)
                qubit.id = -1

        from projectq.meta import DirtyQubitTag
        self.send([Command(self,
                           Deallocate,
                           (Qureg([weak_copy]),),
                           tags=[DirtyQubitTag()] if is_dirty else [])])
