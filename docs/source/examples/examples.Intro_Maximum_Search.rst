Maximum Searching Algorithm
============================

**An Introduction to Maximum Searching Algorithm**

The algorithm for searching for the maximum value solves the problem of
giving a data set and finding the element with the largest value. On a
classic computer, to complete such a task requires a traversal of the
database, the complexity is :math:`O(N)`. The quantum algorithm for
searching for the maximum value is based on the Grover search algorithm.
It was proposed by Christoph Durr and Peter Hoyer in January 1999 [A
quantum algorithm for finding the minimum]
(https://arxiv.org/pdf/quant-ph/9607014). Ashish Ahuja and Sanjiv Kapoor
then made a better complexity analysis and yielded a bound than the
original one of :math:`15\sqrt{N}` [A Quantum Algorithm for finding the
Maximum] (https://arxiv.org/abs/quant-ph/9911082v1). The complexity of
the algorithm is :math:`O(\sqrt(N)),` which is quadratic acceleration
compared to the classical algorithm. Below we briefly introduce this
quantum algorithm.

Basic idea of the quantum algorithm for searching the maximum value:
For the search space A (a dataset), select one of the elements as the
current maximum element randomly, and use the improved Grover search
algorithm, that is, the Grover algorithm that does not know the number
of target elements, to search for a larger element in A. Then replace
the maximum element with the new one. The expected value of the total
replacing times to get the maximum element is no more than
:math:`O(log N)`. After finding the maximum element, the Grover
algorithm cannot find a new element. Then the algorithm stops and
outputs the maximum element.

**Improved Grover search algorithm (Grover algorithm without knowing the number of target elements)**

As can be seen from the above description, the improved Grover search
algorithm plays an important role in the algorithm for searching for the
maximum. We use the HiQ demo to implement an improved Grover search
algorithm.

.. code-block:: python
   :linenos:

   def run_grover(eng, Dataset, oracle, threshold):
       """
       Args:
           eng (MainEngine): Main compiler engine to run Grover on.
           Dataset(list): The set to search for an element.
           oracle (function): The oracle black box, an n-qubit register. It is
            used to flip the relative phase for the correct bit string.
           threshold: The threshold, where the algorithm stops and outputs the local of the element in the database that is greater than the threshold.

       Returns:
           solution (list<int>): the location of Solution.
       """
       N = len(Dataset)
       n = math.ceil(math.log2(N))

       # number of maximum iterations we may run:
       num_it = int(math.sqrt(N)*9/4)

       # two arguments
       m = 1
       landa = 6/5

       # run i iterations
       i = 0
       while i<num_it:

           j = random.randint(0,int(m))
           x = eng.allocate_qureg(n)

           # start in uniform superposition, e.g. the index of all elements
           All(H) | x

           # prepare the oracle output qubit (the one that is flipped to indicate the solution. start in state 1/sqrt(2) * (|0> - |1>) s.t. a bit-flip turns into a (-1)-phase.
           oracle_out = eng.allocate_qubit()
           X | oracle_out
           H | oracle_out

            # run j iterations
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

           # measure
           All(Measure) | x
           Measure | oracle_out

           # read the measure value
           k = 0
           xvalue = 0
           while k<n:
               xvalue = xvalue+int(x[k])*(2**k)
               k+ = 1

           # compare to threshold
           if Dataset[xvalue]>threshold:
               return xvalue
           m = m*landa
           i = i+1

For the function Oracle, the code for its implementation is as follows

.. code-block:: python
   :linenos:

   def oracle(eng, x, Dataset, n0, output):
     """
       Marks the solutions by flipping the output qubit,

       Args:
           eng (MainEngine): Main compiler engine the algorithm is being run on.
           x (Qureg): n-qubit quantum register Grover search is run on.
           Dataset(list): The dataset.
           n0: The threshold.
           output (Qubit): Output qubit to flip in order to mark the solution.
       """

       # The size relationship is represented by 0,1, which is set to 1 if greater than n0, otherwise 0.
       fun = [0]*len(Dataset)
       fun = [x-n0 for x in Dataset]
       fun = np.maximum(fun, 0)
       fun = np.minimum(fun, 1)
       a = sum(fun)
       n = math.ceil(math.log2(len(Dataset)))

       while a>0:
           num = [0]*n
           p = fun.tolist().index(1)
           fun[p] = 0
           i = 0
           a = a-1
           while p/2 != 0:
               num[i] = p % 2
               p = p//2
               i = i+1
           a1 = sum(num)
           num1 = num
           while a1>0:
               p = num1.index(1)
               a1 = a1-1
               num1[p] = 0
               X | x[p]
           with Control(eng, x):
               X | output
           a1 = sum(num)
           while a1>0:
               p = num.index(1)
               a1 = a1-1
               num[p] = 0
               X | x[p]

**HiQ implementation of searching for the maximum value of the quantum algorithm**

The code for quantum algorithm searching for the maximum value is as
follows:

.. code-block:: python
   :linenos:

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

    from projectq.backends import SimulatorMPI
    from projectq.cengines import GreedyScheduler, HiQMainEngine

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
                LocalOptimizer(cache_depth)
                #,CommandPrinter()
            , GreedyScheduler()
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

**References:**

| [1] `A quantum algorithm for finding the minimum (arXiv:quant-ph/9607014) <https://arxiv.org/abs/quant-ph/9607014>`__
| [2] `A Quantum Algorithm for finding the Maximum (arXiv:quant-ph/9911082) <https://arxiv.org/abs/quant-ph/9911082>`__
