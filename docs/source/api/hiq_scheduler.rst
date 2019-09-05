HiQ Scheduler
=============

**HiQ scheduler** is  a quantum cuircuit scheduler. It consists of two parts: Swap Scheduler and Cluster Scheduler.

**Swap Scheduler** calculates IDs of qubits that should be local. It tries to maximize the number of gates in a single stage.

**Cluster Scheduler** works within a single stage and merges gates into clusters using Greedy Algorithm: it tries to maximize the number of gates in a single cluster.
Each time the scheduler is run, it calculates and returns a cluster.

See :meth:`GreedyScheduler class <hiq.projectq.cengines.GreedyScheduler>` to learn more about local and global qubits, stages and clusters.

.. toctree::
   :caption: Table of contents

   swap_scheduler
   cluster_scheduler
