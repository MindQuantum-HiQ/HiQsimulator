Bernstein-Vazirani Algorithm
============================

**An Introduction to Bernstein-Vazirani Algorithm**

Bernstein-Vazirani algorithm was proposed by Bernstein Ethan and
Vazitani Umesh in 1993. This algorithm can solve the Bernstein-Vazirani
problem, which can be described as follows.

Give a Boolean function

.. math:: f_s:\{0,1\}^n\rightarrow \{0,1\}: f_s(x)=s\cdot x, x,s\in\{0,1\}^n.

Here, :math:`s` is an unknown bit string and

.. math:: s\cdot x=(\sum_{i=1}^n s_ix_i) mod~2,

where :math:`x_i​` and :math:`s_i​` are :math:`i​`-th bit of :math:`x​`
and :math:`s​`, respectively.

Our goal is to determine the bit string :math:`s​`. The classical
algorithms of this problem need at least :math:`\Omega(n)​` black boxes
of :math:`f_s​`, but Bernstein-Vazirani algorithm only needs
:math:`O(1)​` black boxes of :math:`f_s​`.

**Realization of Bernstein-Vazirani Algorithm on HiQ**

.. code-block:: python
   :linenos:

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
        backend = SimulatorMPI(gate_fusion=True)
        
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
        
        n = 10 # number of qubits
        
        print("===========================================================")
        print("= This is the Bernstein-Vazirani algorithm demo")
        print("= you can change the unknown string by modifying the oracle")
        
        s = run_BV_algorithm(eng, n, BV_oracle)
        s_str = "".join([str(ch) for ch in s])
        print("= Length of the bit string: {}".format(n))
        print("= The unknown bit string is: {}".format(s_str))
        print("===========================================================")
