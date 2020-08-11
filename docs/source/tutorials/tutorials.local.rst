Programming on a Local Computer
===============================


Installation
------------

Detailed installation instructions can be found on the :ref:`Installation <installation>` page.

How to Start Running
---------------------

**Running on a single node:**

If you just run the simulator on a server, simply run:

.. code-block:: bash

   mpirun -np 2 python3 ./examples/teleport_mpi.py

The parameter **-np** indicates the number of parallel processes.  The
parameters can be adjusted according to the CPU resources of the server, and
the HiQsimulator will balance the allocation of memory and computing resources
between processes.

.. note::

   On Windows you might need to use ``mpiexec -n`` instead of ``mpirun -np``.

**Running through the slurm job scheduler:**

If you want to run HiQ programs on a clustered environment, it is recommended
to install a cluster management software, such as Slurm.  For example, the
above example can be split over 4 MPI nodes (each with 2 processes) and
writing a Slurm submission script "run.slurm":

.. code-block:: bash

    #!/usr/bin/env bash
    # SBATCH -N 4                  # 4 nodes are requested
    # SBATCH -t 00:03:00           # Walltime, 3 minutes
    # SBATCH -n 8                  # 8 processes are requested
    # SBATCH --ntasks-per-socket=1 # 1 process per allocated socket
    # SBATCH --hint=compute_bound  # Use all cores in each socket, one thread
    #                              # per core
    # SBATCH --exclusive           # node should not be shared with other jobs

    mpirun  python3 ./examples/teleport_mpi.py       # Execute program

Then, copy the running script and program to all cluster nodes with the same
directory.  You can also create an NFS share directory and execute script and
program in the shared directory.  Detailed methods can be found in `Ubuntu NFS
<https://help.ubuntu.com/lts/serverguide/network-file-system.html.en>`__.

Finally, submit task:

.. code-block:: bash

    sbatch  run.slurm

Slurm will automatically assign MPI nodes to execute programs.  Same as the
single node, the Hiqsimulator will balance the allocation of memory and
computing resources between nodes and processes.

More information about Slurm can be found in `Slurm documentation
<https://slurm.schedmd.com/documentation.html>`__.


A simple example of programming HiQ in Python
---------------------------------------------

To understand the Python API of HiQ programming framework, we will first go
through an instance of coding, and then dig into the details of connecting the
programming framework to HiQ backends in the next session.

**An instance: random number generation**

We demonstrate the following "random_number.py" as an example of programming
to generate a random number in the local computer. We assume the code is
placed under the directory of examples. The code reads:

.. code-block:: python
   :linenos:

    from projectq.ops import H, Measure
    from projectq.backends import SimulatorMPI
    from projectq.cengines import GreedyScheduler, HiQMainEngine

    from mpi4py import MPI

    # Create main engine to compile the code to machine instructions(required)
    eng = HiQMainEngine(SimulatorMPI(gate_fusion=True, num_local_qubits=20))

    # Use the method provided by the main engine to create a qubit
    q1 = eng.allocate_qubit()

    # Apply the Hadamard gate to the qubit so that it generates a
    # superposition of 0 and 1 states
    H | q1

    # Measure the qubit with a basis spanned by {|0>, |1>}
    Measure | q1

    # Call the main engine to execute
    eng.flush()

    # Obtain the output. Note that the result is still stored in the qubit
    # object yet clashed into a classical bit
    print("Measured: {}".format(int(q1)))

To run example use command line:

.. code-block:: bash

   mpirun -np 2 python3 random_number.py

.. note::

   On Windows you might need to use ``mpiexec -n`` instead of ``mpirun -np``.

Get the following output():

.. code-block:: bash

    Measured: 0
    Measured: 0

These values are obtained from the C++ simulator as HiQ's backend, and
each 0/1 output is pseudo-random.
