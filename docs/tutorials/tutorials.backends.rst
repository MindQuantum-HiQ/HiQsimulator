.. _Use-HiQsimulator-as-backends:

Use HiQsimulator as backends
=============================


Full-amplitude simulator
-------------------------

The full-amplitude quantum computing simulator is a basic module of the
HiQ quantum computation simulation software. It can calculate the
amplitude of all quantum state density matrix elements after evolution.
We take this simulator as an example to describe the basic syntax of HiQ
programming.

The design concept of HiQ is to provide intuitive syntax for users to
write quantum programs, making the learning curve relatively smooth.
Based on this idea, the syntax of HiQ is similar to that of mathematical
symbols in quantum computing. For example, we now turn the x-revolving
gate to a certain qubit, and the rotation angle of the revolving gate is
theta. Mathematically, this operation can be expressed as

.. math::
    Rx(\theta) \left | \psi \right \rangle


where :math:`\left | \psi \right \rangle` represents the state of the qubit. The corresponding HiQ
implementation is
::

    Rx(theta) | qubit

The state of the qubit register Rx belongs to the quantum gate type. It
reloads \| (or) operator. The reload is to apply the Rx gate to the
qubit. More importantly, the \| operator separates the classical
parameter (object on the left-hand-side of \| -- the Rx(theta) operator,
in this case) and the quantum parameter (object on the right-hand-side
of \|, or the qubit here) to facilitate the compiler to handle them
individually.

Below, we will briefly introduce the compiling environment:

1. Import packages required for implementing the algorithm.

.. code-block:: python
   :linenos:

    from projectq.ops import H, Z, X, CNOT, Measure, All, Ph
    from projectq.meta import Loop, Compute, Uncompute, Control
    from projectq.cengines import (MainEngine,
                                    AutoReplacer,
                                    LocalOptimizer,
                                    TagRemover,
                                    DecompositionRuleSet)
    import projectq.setups.decompositions

    from hiq.projectq.backends import SimulatorMPI
    from hiq.projectq.cengines import GreedyScheduler, HiQMainEngine

    from mpi4py import MPI


(1)Package ops include all supported quantum gates.

(2)Package meta includes optimizations of some implemented operations.

(3)Package backends include all supported backends.

(4)Package engines include all modules of the compiler engine.

(5)Package setups can be used to configure the main engine.

(6)Package mpi4py are used to call the dependent MPI library.


2. Indicate the backend simulator and create a main engine.

.. code-block:: python
   :linenos:

    # create a main compiler engine with a simulator backend:
    cache_depth = 10
    rule_set = DecompositionRuleSet(modules  = [projectq.setups.decompositions])
    engines = [TagRemover(),
                LocalOptimizer(cache_depth),
                AutoReplacer(rule_set),
                TagRemover(),
                LocalOptimizer(cache_depth),
                GreedyScheduler()]

    eng = HiQMainEngine(SimulatorMPI(gate_fusion=True, num_local_qubits=20), engines)
    
.. note::
    The parameter **num_local_qubits** represents the number of qubits supported by an MPI node, 
    depending on the available memory of the node. The 20 qubits probably require about 16M of memory. 
    For each additional qubit, the required memory will double. 
    In actual use, the appropriate value can be set according to the algorithm requirements, 
    the available memory of the single node, and the number of nodes.

3. Program the algorithm logic.

.. code-block:: python
   :linenos:

    # make a Bell-pair
    b1, b2 = create_bell_pair(eng)

    # Alice creates a nice state to send
    psi = eng.allocate_qubit()

    print("= Step 1. Alice creates the state to be sent from \|0>")
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

    print("\\t Bob successfully arrived at \|0>")

where the creation function for the Bell pair could be implemented as:

.. code-block:: python
   :linenos:
    def create_bell_pair(eng):
        b1, b2 = eng.allocate_qureg(2)
        H | b1
        CNOT | (b1, b2)
        return b1, b2


Single amplitude simulator
--------------------------

Comming soon.


Quantum Error Correction simulator (Stabilizer Circuit simulator)
------------------------------------------------------------------

The Stabilizer Circuit Simulator can be used to efficiently simulate
circuits merely composed by a Stabilizer gate set (including CNOT,
Hadamard, and Phase gate) and measurement meters. This kind of quantum
circuit is called Stabilizer circuit, which is basic for studying
quantum error correction and fault-tolerant quantum circuit designs. It
is also an effective means for the research of error correction encoder
and decoder. By using algorithm optimization and distributed computing
capabilities in the HiQ software package, the simulator can easily
simulate hundreds of thousands of qubits of Stabilizer quantum circuits.
The basic procedure is as follows:

1. Import the simulator package.

.. code-block:: python
   :linenos:

    from projectq.ops import CNOT, H, Measure
    from hiq.projectq.backends import StabilizerSimulator
    from hiq.projectq.cengines import HiQMainEngine


2. Initialize the simulator.

.. code-block:: python
   :linenos:

    simulator = StabilizerSimulator(9)
    eng = HiQMainEngine(simulator, [])


3. Program the algorithm logic.

.. code-block:: python
   :linenos:

    #allocate
    qubits = eng.allocate_qureg(9)
    
    #prepares a uniform superposition over 5-bit strings in qubits 0 to 4
    H | qubits[0]
    H | qubits[1]
    H | qubits[2]
    H | qubits[3]
    H | qubits[4]

    #computes f in qubits 5 to 8
    CNOT | (qubits[0], qubits[5])
    CNOT | (qubits[1], qubits[5])
    CNOT | (qubits[1], qubits[6])
    CNOT | (qubits[2], qubits[6])
    CNOT | (qubits[2], qubits[7])
    CNOT | (qubits[3], qubits[7])
    CNOT | (qubits[3], qubits[8])
    CNOT | (qubits[4], qubits[8])

    #measures those qubits "for pedagogical purposes."
    Measure | qubits[5]
    Measure | qubits[6]
    Measure | qubits[7]
    Measure | qubits[8]
    eng.flush()
    print("= The qubits5-8 state:{}{}{}{} ".format(int(qubits[5]),
           int(qubits[6]), int(qubits[7]), int(qubits[8])))

    #performs a Fourier transform on qubits 0 to 4
    H | qubits[0]
    H | qubits[1]
    H | qubits[2]
    H | qubits[3]
    H | qubits[4]

    #measure
    Measure | qubits[0]
    Measure | qubits[1]
    Measure | qubits[2]
    Measure | qubits[3]
    Measure | qubits[4]
    eng.flush()
    print("= The qubits0-4 state: {}{}{}{}{}".format(int(qubits[0]),
           int(qubits[1]), int(qubits[2]), int(qubits[3]), int(qubits[4])))
