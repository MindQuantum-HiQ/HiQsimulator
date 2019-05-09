from projectq.ops import All, CNOT, H, Measure, Rz, X, Z
from projectq.meta import Dagger, Control
from projectq.backends import CircuitDrawer, CommandPrinter
from projectq.cengines import (MainEngine,
                               AutoReplacer,
                               LocalOptimizer,
                               TagRemover,
                               DecompositionRuleSet)
import projectq.setups.decompositions

from hiq.projectq.cengines import GreedyScheduler, HiQMainEngine
from hiq.projectq.backends import SimulatorMPI

from mpi4py import MPI

def _create_bell_pair(eng):
    """
    Returns a Bell-pair
    Args:
        eng (MainEngine): MainEngine from which to allocate the qubits.
    Returns:
        bell_pair (tuple<Qubits>): The Bell-pair.
    """
    b1 = eng.allocate_qubit()
    b2 = eng.allocate_qubit()

    H | b1
    CNOT | (b1, b2)

    return b1, b2


def run_teleport(eng, state_creation_function):
    """
    Runs quantum teleportation on the provided main compiler engine.
    Args:
        eng (MainEngine): Main compiler engine to run the circuit on.
        state_creation_function (function): Function which accepts the main
            engine and a qubit in state |0>, which it then transforms to the
            state that Alice would like to send to Bob.
    """
    # make a Bell-pair
    b1, b2 = _create_bell_pair(eng)

    # Alice creates a nice state to send
    psi = eng.allocate_qubit()
    print("= Step 1. Alice creates the state to be sent from |0>")
    state_creation_function(eng, psi)

    # entangle it with Alice's b1
    CNOT | (psi, b1)
    print("= Step 2. Alice entangles the state with her share of the Bell-pair")

    # measure two values (once in Hadamard basis) and send the bits to Bob
    H | psi
    Measure | psi
    Measure | b1
    msg_to_bob = [int(psi), int(b1)]
    print("= Step 3. Alice sends the classical message {} to Bob".format(msg_to_bob))

    # Bob may have to apply up to two operation depending on the message sent
    # by Alice:
    with Control(eng, b1):
        X | b2
    with Control(eng, psi):
        Z | b2

    # try to uncompute the psi state
    print("= Step 4. Bob tries to recover the state created by Alice")
    with Dagger(eng):
        state_creation_function(eng, b2)

    # check whether the uncompute was successful. The simulator only allows to
    # delete qubits which are in a computational basis state.
    del b2
    eng.flush()

    print("\t Bob successfully arrived at |0>")


if __name__ == "__main__":
    # create a main compiler engine with a simulator backend:
    backend = SimulatorMPI(gate_fusion=True, num_local_qubits=20)
    # backend = CircuitDrawer()
    # locations = {}
    # for i in range(module.nqubits):
    #     locations[i] = i
    # backend.set_qubit_locations(locations)
    
    cache_depth = 10
    rule_set = DecompositionRuleSet(modules=[projectq.setups.decompositions])
    engines = [TagRemover(),
    		   LocalOptimizer(cache_depth),
    		   AutoReplacer(rule_set),
    		   TagRemover(),
    		   LocalOptimizer(cache_depth),
    		   #,CommandPrinter(),
    		   GreedyScheduler()
    		   ]
    
    eng = HiQMainEngine(backend, engines)


    # define our state-creation routine, which transforms a |0> to the state
    # we would like to send. Bob can then try to uncompute it and, if he
    # arrives back at |0>, we know that the teleportation worked.
    def create_state(eng, qb):
        H | qb
        Rz(1.21) | qb

    print("==================================================================")
    print("= This is the Teleportation algorithm demo")
    print("= Change the state to send by modifying create_state function")
    print("= It is assumed that Alice and Bob share a Bell-pair in advance")
    run_teleport(eng, create_state)
    print("==================================================================")
