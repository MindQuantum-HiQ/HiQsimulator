from __future__ import division
import math

from projectq.ops import H, Z, X, Measure, All, Ph, Rz
from projectq.meta import Loop, Compute, Uncompute, Control

from projectq.backends import CircuitDrawer, CommandPrinter
from projectq.cengines import (MainEngine,
                               AutoReplacer,
                               LocalOptimizer,
                               TagRemover,
                               DecompositionRuleSet)
import projectq.setups.decompositions

from hiq.projectq.backends import SimulatorMPI
from hiq.projectq.cengines import GreedyScheduler, HiQMainEngine

from mpi4py import MPI

def run_exactgrover(eng, n, oracle, oracle_modified):
    """
    Runs exact Grover's algorithm on n qubit using the two kind of provided 
    quantum oracles (oracle and oracle_modified).
    This is an algorithm which can output solution with probability 1.

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
    num_it = int(math.pi/4.*math.sqrt(1 << n))
    
    #phi is the parameter of modified oracle
    #varphi is the parameter of reflection across uniform superposition
    theta=math.asin(math.sqrt(1/(1 << n)))
    phi=math.acos(-math.cos(2*theta)/(math.sin(2*theta)*math.tan((2*num_it+1)*theta)))
    varphi=math.atan(1/(math.sin(2*theta)*math.sin(phi)*math.tan((2*num_it+1)*theta)))*2

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
   
    # prepare the oracle output qubit (the one that is flipped to indicate the
    # solution. start in state |1> s.t. a bit-flip turns into a e^(i*phi)-phase. 
    H | oracle_out
    oracle_modified(eng, x, oracle_out, phi)
    
    with Compute(eng):
        All(H) | x
        All(X) | x

    with Control(eng, x[0:-1]):
        Rz(varphi) | x[-1]
        Ph(varphi/2) | x[-1]

    Uncompute(eng)
    
    All(Measure) | x
    Measure | oracle_out

    eng.flush()
    # return result
    return [int(qubit) for qubit in x]


def alternating_bits_oracle(eng, qubits, output):
    """
    Marks the solution string 0, 1, 0,...,0 by flipping the 
    
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
    
def alternating_bits_oracle_modified(eng, qubits, output, phi):
    """
    Marks the solution string 0,1,0,... by applying phase gate to the output bits,
    conditioned on qubits being equal to the alternating bit-string.

    Args:
        eng (MainEngine): Main compiler engine the algorithm is being run on.
        qubits (Qureg): n-qubit quantum register Grover search is run on.
        output (Qubit): Output qubit to mark the solution by a phase gate with parameter phi.
    """
    with Compute(eng):
        All(X) | qubits[1::2]
    with Control(eng, qubits):
        Rz(phi) | output
        Ph(phi/2) | output

    Uncompute(eng)

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
    # run Grover search to find a 7-bit solution    

    print("=====================================================================")
    print("= This is the Grover algorithm demo")
    print("= Change the list to search by modifying alternating_bits_oracle")
    print("= The chosen list is: [0, 1, 0, ....]")
    print("= The second element is marked")
    print("= Calling ExactGrover algorithm to find the marked element now...")
    # run Grover search to find a 7-bit solution
    N = 12
    mark_bits = run_exactgrover(eng, N, alternating_bits_oracle, alternating_bits_oracle_modified)
    found = False
    for i in range(len(mark_bits)):
        if mark_bits[i] == 0:
            print("= Marked element is found, it index is: {}".format(i))
            found = True
    if not found:
        print("= Cannot found the marked element!")
    print("=====================================================================")    

