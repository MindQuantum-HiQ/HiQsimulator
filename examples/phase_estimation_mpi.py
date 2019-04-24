import sys
import numpy as np
import cmath
import math

from projectq.ops import H, Z, All, R, Swap, Measure, X, get_inverse, QFT, Tensor, BasicGate
from projectq.meta import Loop, Control
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

def _bits_required_to_achieve_accuracy(n, epsilon):
    """
    Compute the number of bits required to achieve given probability and accuracy
    For details, see Eq. (5.35) on Nielsen's book

    Args:
        n (int): the number of bits that we wish our estimation of theta can be accurate
        epsilon (double): probability of success at least 1 - epsilon
    Returns:
        n_updated (int): number of bits required to acchieve the given accuracy and probability
    """
    tmp = math.log2(2 + 1/(2*epsilon))
    tmp = math.ceil(tmp)
    n_updated = int(n + tmp)
    return n_updated

def run_phase_estimation(eng, U, state, m, n):
    """
    Phase estimation algorithm on unitary U. 

    Args:
        eng (MainEngine): Main compiler engine to run the phase_estimation algorithm
        U (np.matrix): the unitary that is to be estimated
        state (qureg): the input state, which has the form |psi> otimes |0>.
                        State is composed of two parts: 
                            1. The first n qubits are the input state |psi>, which is commonly
                               chosen to be the eigenstate of U
                            2. The last n qubits are initialized to |0>, which is used to 
                               store the binary digits of the estimated phase
        m (int): number of qubits for input state psi
        n (int): number of qubits used to store the phase to given precision
    """
    # The beginning index for the phase qubits will have an offset
    OFFSET = m
    # Phase 1. performs the controlled U^{2^j} operations
    for k in range(n):
        H | state[OFFSET+k]
        
        with Control(eng, state[OFFSET+k]):
            # number of loops required to perfrom the controlled U^{2^j} operation
            num = int(math.pow(2, k)) 
            # use the buildin loop
            with Loop(eng, num):
                U | state[:OFFSET]

    # Phase 2. performs the inverse QFT operation
    # Step 2.1 swap the qubits
    for k in range(math.floor(n/2)):
        Swap | (state[OFFSET+k], state[OFFSET+n-k-1])
    # Step 2.2 perfrom the original inverse QFT
    get_inverse(QFT) | state[OFFSET:]

if __name__ == "__main__":

    # create a main compiler engine with a simulator backend:
    backend = SimulatorMPI(gate_fusion=True)
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
    		   LocalOptimizer(cache_depth)
    		   #,CommandPrinter()
    	   , GreedyScheduler()
    		   ]
    
    eng = HiQMainEngine(backend, engines) 
    m = 2
    epsilon = 0.1   # the estimation algorithm successs with probability 1 - epsilon 
    n_accuracy = 3  # the estimation algorithm estimates with 3 bits accuracy
    n = _bits_required_to_achieve_accuracy(n_accuracy, epsilon)
    
    ## we create a unitary U = R(cmath.pi*3/4) \ox R(cmath.pi*3/4)
    U = BasicGate() 
    theta = math.pi*3/4
    U.matrix = np.matrix([[1, 0, 0, 0], 
                          [0, cmath.exp(1j*theta), 0, 0], 
                          [0, 0, cmath.exp(1j*theta), 0], 
                          [0, 0, 0, cmath.exp(1j*2*theta)]])

    state = eng.allocate_qureg(m+n)

    # prepare the input state to be |psi>=|01>
    X | state[1]
    
    run_phase_estimation(eng, U, state, m, n)
    
    # we have to post-process the state that stores the estimated phase
    OFFSET = m
    Tensor(Measure) | state[OFFSET:]
    Tensor(Measure) | state[:OFFSET]
    eng.flush()
    
    outcome = "" # used to store the binary string of the estimated phase
    value = 0    # used to store the decimal value of the estimated phase
    for k in range(n_accuracy):
        bit = int(state[OFFSET+n-1-k])
        outcome = outcome + str(bit)
        value = value + bit/(1 << (k+1))

    print("===========================================================================")
    print("= This is the Phase Estimation algorithm demo")
    print("= The algorithm estimates the phase accurate to {} bits \n" 
          "= with probability of success at least {}".format(n_accuracy, 1 - epsilon))
    print("= Change the accuracy and probability by modifying n_accuracy and epsilon")
    print("= Change the unitary and psi by modifying U.matrix and state")
    print("= The estimated phase (in binary) is: 0.{}".format(outcome))
    print("= The estimated phase (in decimal) is: {}".format(value))
    print("===========================================================================")