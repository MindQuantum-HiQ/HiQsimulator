HHL Algorithm
=============

**HHL algorithm introduction**

Proposed by Aram Harrow et al in 2009, the HHL algorithm is used to
solve linear equations. Under the premise of some assumptions, this
algorithm can solve the linear equations with exponential acceleration
compared with the classical algorithm. HHL algorithm is the key step in
the current quantum artificial intelligence algorithm.

The basic idea of HHL algorithm:

The HHL algorithm mainly solves the linear equations
:math:`{\rm{A}}\overrightarrow {\rm{x}} {\rm{ = \vec b}}`, where the
matrix A and vector :math:`\vec b` are known.

The approximate steps of the HHL algorithm are as follows:

   (1) Initialization: Initialize the vector :math:`\vec b` to be
       quantum state :math:`\left| b \right\rangle`.

   (2) Estimate the eigenvalues of matrix A: Use the phase estimation
       algorithm to get the value of :math:`\lambda` in
       :math:`{e^{2\pi i\lambda }}` which is also the eigenstate of
       matrix A.

   (3) Controlled rotation: Controlled rotation of auxiliary qubit is
       performed according to the estimated eigenvalues. Then use the
       approximate value :math:`\frac{\theta }{{{2^r}}}` as
       :math:`{\rm{sin}}\left( {\frac{\theta }{{{2^r}}}} \right)`, which
       is the reciprocal of the eigenvalue.

   (4) Reset :math:`\left| {{q_0}{q_1}} \right\rangle` as
       :math:`\left| {00} \right\rangle` using the reverse phase
       estimation algorithm.

   (5) Measure :math:`\left| {q_3} \right\rangle`; If the result is 1,
       the corresponding quantum state :math:`\left| {q_2} \right\rangle = \left| {x} \right\rangle`. Then the approximate
       solution of the equations can be obtained.

The quantum circuit diagram of the HHL algorithm is as follows:

.. image:: /images/HHL.png

**References:**

| [1] `Quantum algorithm for solving linear systems of equations <https://arxiv.org/abs/0811.3171>`__
| [2] `Quantum Algorithm Implementations for Beginners <https://arxiv.org/abs/1804.03719>`__
