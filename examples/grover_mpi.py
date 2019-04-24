import math

from projectq.ops import H, Z, X, Measure, All, Ph
from projectq.meta import Loop, Compute, Uncompute, Control

from projectq.cengines import (MainEngine,
                               AutoReplacer,
                               LocalOptimizer,
                               TagRemover,
                               DecompositionRuleSet)
from hiq.projectq.cengines import GreedyScheduler, HiQMainEngine
from hiq.projectq.backends import SimulatorMPI
import projectq.setups.decompositions

from mpi4py import MPI

"""
To run example use the command line:
$ mpirun -np 4 python ./grover_mpi.py
"""


def run_grover(eng, n, oracle):
    """
    Runs Grover's algorithm on n qubit using the provided quantum oracle.

    Args:
        eng (MainEngine): Main compiler engine to run Grover on.
        n (int): Number of bits in the solution.
        oracle (function): Function accepting the engine, an n-qubit register,
            and an output qubit which is flipped by the oracle for the correct
            bit string.

    Returns:
        solution (list<int>): Solution bit-string.
    """
    x = eng.allocate_qureg(n)

    # start in uniform superposition
    All(H) | x

    # number of iterations we have to run:
    num_it = int(math.pi / 4. * math.sqrt(1 << n))
    print("Number of iterations we have to run: {}".format(num_it))

    # prepare the oracle output qubit (the one that is flipped to indicate the
    # solution. start in state 1/sqrt(2) * (|0> - |1>) s.t. a bit-flip turns
    # into a (-1)-phase.
    oracle_out = eng.allocate_qubit()
    X | oracle_out
    H | oracle_out

    # run num_it iterations
    with Loop(eng, num_it):
        # oracle adds a (-1)-phase to the solution
        oracle(eng, x, oracle_out)

        # reflection across uniform superposition
        with Compute(eng):
            All(H) | x
            All(X) | x

        with Control(eng, x[0:-1]):
            Z | x[-1]

        Uncompute(eng)

        All(Ph(math.pi / n)) | x

    All(Measure) | x
    Measure | oracle_out

    eng.flush()
    # return result
    return [int(qubit) for qubit in x]


def alternating_bits_oracle(eng, qubits, output):
    """
    Marks the solution string 0,1,0,..... by flipping the 
    
    qubit,
    conditioned on qubits being equal to the alternating bit-string.

    Args:
        eng (MainEngine): Main compiler engine the algorithm is being run on.
        qubits (Qureg): n-qubit quantum register Grover search is run on.
        output (Qubit): Output qubit to flip in order to mark the solution.
    """
    with Compute(eng):
        X | qubits[1]
    with Control(eng, qubits):
        X | output
    Uncompute(eng)


if __name__ == "__main__":
    # create a main compiler engine with a simulator backend:
    backend = SimulatorMPI(gate_fusion=True)

    cache_depth = 10
    rule_set = DecompositionRuleSet(modules=[projectq.setups.decompositions])
    engines = [TagRemover()
               , LocalOptimizer(cache_depth)
               , AutoReplacer(rule_set)
               , TagRemover()
               , LocalOptimizer(cache_depth)
               , GreedyScheduler()
               ]

    eng = HiQMainEngine(backend, engines)

    if MPI.COMM_WORLD.Get_rank() == 0:
        print("=====================================================================")
        print("= This is the Grover algorithm demo")
        print("= Change the list to search by modifying alternating_bits_oracle")
        print("= The chosen list is: [0, 1, 0, ....]")
        print("= The second element is marked")
        print("= Calling Grover algorithm to find the marked element now...")

    # run Grover search to find a 7-bit solution
    N = 7
    mark_bits = run_grover(eng, N, alternating_bits_oracle)

    if MPI.COMM_WORLD.Get_rank() == 0:
        found = False
        for i in range(len(mark_bits)):
            if mark_bits[i] == 0:
                print("= Marked element is found, its index is: {}".format(i))
                found = True
        if not found:
            print("= Cannot find the marked element!")
        print("=====================================================================")
