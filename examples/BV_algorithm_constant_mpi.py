import math

from projectq.ops import H, Z, X, Measure, All, CNOT
from projectq.meta import Loop, Compute, Uncompute, Control

from projectq.backends import CircuitDrawer, CommandPrinter
from projectq.cengines import (MainEngine,
                               AutoReplacer,
                               LocalOptimizer,
                               TagRemover,
                               DecompositionRuleSet)

from hiq.projectq.cengines import GreedyScheduler, HiQMainEngine
import projectq.setups.decompositions
from hiq.projectq.backends import SimulatorMPI

from mpi4py import MPI

def run_BV_algorithm(eng, n, oracle):
    """
    Args:
        eng (MainEngine): the Main engine on which we run the BV algorithm
        n (int): number of qubits
        oracle (function): oracle used by the BV algorithm
    """
    x = eng.allocate_qureg(n)
    
    # start in uniform superposition
    All(H) | x
    
    oracle_out = eng.allocate_qubit()
    X | oracle_out
    H | oracle_out
    
    oracle(eng, x, oracle_out)
    
    All(H) | x
    
    All(Measure) | x
    Measure | oracle_out

    eng.flush()
    
    return [int(qubit) for qubit in x]

def BV_oracle(eng, qubits, output):
    """
    This is an oracle for linear function f(x) = s cdot x, where s=[1,0,0,0,...].
    This oracle mark the basis x for which f(x)=1.

    Args:
        eng (MainEngine): Main compiler engine the algorithm is being run on.
        qubits (Qureg): n-qubit quantum register BV_algorithm is run on.
        output (Qubit): Output qubit to flip in order to mark the solution.
    """
    CNOT | (qubits[0], output)
    
if __name__ == "__main__":
    # create a main compiler engine with a simulator backend:
    backend = SimulatorMPI(gate_fusion=True, num_local_qubits=20)
    
    cache_depth = 10
    rule_set = DecompositionRuleSet(modules=[projectq.setups.decompositions])
    engines = [TagRemover(),
    		   LocalOptimizer(cache_depth),
    		   AutoReplacer(rule_set),
    		   TagRemover(),
    		   LocalOptimizer(cache_depth),
    		   #CommandPrinter(),
    		   GreedyScheduler()
    		   ]
    
    eng = HiQMainEngine(backend, engines)  
    
    n = 10 # number of qubits
    
    print("===========================================================")
    print("= This is the Bernstein-Vazirani algorithm demo")
    print("= you can change the unknown string by modifying the oracle")
    
    s = run_BV_algorithm(eng, n, BV_oracle)
    s_str = "".join([str(ch) for ch in s])
    print("= Length of the bit string: {}".format(n))
    print("= The unknown bit string is: {}".format(s_str))
    print("===========================================================")
    
