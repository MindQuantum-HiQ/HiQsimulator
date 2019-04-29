import math
import random
import numpy as np

from projectq.ops import H, Z, X, Measure, All
from projectq.meta import Loop, Compute, Uncompute, Control


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

def run_grover(eng, Dataset, oracle, threshold):
    """
    Runs modified Grover's algorithm to find an element in a set larger than a 
    given value threshold, using the provided quantum oracle.

    Args:
        eng (MainEngine): Main compiler engine to run Grover on.
        Dataset(list): The set to search for an element.
        oracle (function): Function accepting the engine, an n-qubit register, 
        Dataset, the threshold, and an output qubit which is flipped by the
        oracle for the correct bit string.
        threshold: the threshold.

    Returns:
        solution (list<int>): the location of Solution.
    """
    N = len(Dataset)     
    n = math.ceil(math.log2(N))
    
    # number of iterations we have to run:
    num_it = int(math.sqrt(N)*9/4)
     
    m = 1
    landa = 6/5 
     
    # run num_it iterations
    i = 0
    while i<num_it:

        random.seed(i) 
        j = random.randint(0,int(m))
            
        x = eng.allocate_qureg(n)
    
        # start in uniform superposition
        All(H) | x
        
        # prepare the oracle output qubit (the one that is flipped to indicate the
        # solution. start in state 1/sqrt(2) * (|0> - |1>) s.t. a bit-flip turns
        # into a (-1)-phase.
        oracle_out = eng.allocate_qubit()
        X | oracle_out
        H | oracle_out
             
        #run j iterations
        with Loop(eng, j):
            # oracle adds a (-1)-phase to the solution
            oracle(eng, x, Dataset,threshold, oracle_out)
    
            # reflection across uniform superposition
            with Compute(eng):
                All(H) | x
                All(X) | x
    
            with Control(eng, x[0:-1]):
                Z | x[-1]
    
            Uncompute(eng)
    
        All(Measure) | x
        Measure | oracle_out
         
        #read the measure value 
        k=0
        xvalue=0
        while k<n:
            xvalue=xvalue+int(x[k])*(2**k)
            k+=1
            
        #compare to threshold
        if Dataset[xvalue]>threshold:
            return xvalue
        m=m*landa
        i=i+1
        
    #fail to find a solution (with high probability the solution doesnot exist)
    return("no answer")   
    

def oracle(eng, x, Dataset, n0, output):
    """
    Marks the solutions by flipping the output qubit.

    Args:
        eng (MainEngine): Main compiler engine the algorithm is being run on.
        x (Qureg): n-qubit quantum register Grover search is run on.
        Dataset(list): The dataset.
        n0: The threshold.
        output (Qubit): Output qubit to flip in order to mark the solution..
    """
    fun = [0]*len(Dataset)
    fun = [x-n0 for x in Dataset]
    fun = np.maximum(fun, 0)
    fun = np.minimum(fun, 1)
    a = sum(fun)
    n = math.ceil(math.log2(len(Dataset)))
    
    while a>0:
        num=[0]*n
        p = fun.tolist().index(1)
        fun[p] = 0
        i=0
        a=a-1
        while p/2 != 0:
            num[i] = p % 2
            p = p//2
            i = i+1       
        a1 = sum(num)  
        num1=num
        while a1>0:
            p = num1.index(1)
            a1 = a1-1
            num1[p]=0
            X | x[p]             
        with Control(eng, x):
            X | output
        a1=sum(num)
        while a1>0:
            p = num.index(1)
            a1 = a1-1
            num[p]=0
            X | x[p] 
        
        
def find_maximum(eng, Dataset):
    """
    The algorithm to find the maximum data in a dataset. The length of the input
    dataset should be 2^x, x is a positive integer.
    
    Args:
        eng (MainEngine): Main compiler engine the algorithm is being run on.
        Dataset(list): The dataset.
    """
    random.seed(len(Dataset))  
    y=random.randint(0,len(Dataset)-1)        
    a1=y
    # choose a base at random:
    a=run_grover(eng, Dataset, oracle, Dataset[y])
    i=0
    while i<int(math.log2(len(Dataset))):
        if a!="no answer":
            a1=a
            a=run_grover(eng, Dataset, oracle, Dataset[a1])
            i=i+1
        else:
            a=run_grover(eng, Dataset, oracle, Dataset[a1])
            i=i+0.5
    return(a1)

def _is_power2(num):
    """
    Checks whether a given number is a power of two
    """
    return num != 0 and ((num & (num - 1)) == 0)

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
    		   LocalOptimizer(cache_depth)
    		   #,CommandPrinter()
    	   , GreedyScheduler()
    		   ]
    
    eng = HiQMainEngine(backend, engines) 
    
    Dataset = [1,6,2,3,9,1,6,8,1,6,2,12,2,1,6,1]
    if not _is_power2(len(Dataset)):
        raise ValueError("The size of the dataset must be a power of 2!")

    print("=======================================================================")
    print("= This is the Max Element Search algorithm demo")
    print("= The algorithm searches for the index of the maximum element in a set")
    print("= Change the set to be searched by modifying the dataset variable")
    # search for the index of the max element
    print("= The dataset is: ", end='')
    print(Dataset)
    max_index = find_maximum(eng, Dataset)
    if max_index == None:
        print("= The algorithm fails to find the maximum element and its index")
    else:
        print("= The maximum element is: {}".format(Dataset[max_index]))
        print("= The index of the maximum element is: {}".format(max_index))
    print("=======================================================================")
