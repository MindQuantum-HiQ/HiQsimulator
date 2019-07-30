Using Shor’s Algorithm to Achieve Factor Decomposition
======================================================

**Introduction to Shor’s Algorithm**

The time complexity of Shor’s algorithm to decompose integer N on a
quantum computer is :math:`logN`, which is almost the exponential
acceleration of :math:`e` for the most effective classical factorization
algorithm known. Such acceleration may break modern encryption mechanism
such as RSA on a quantum computer.

**The basic idea of Shor’s algorithm**

The main problem to be solved by Shor’s algorithm is: given an integer
N, find its prime factor. That is, determine the two prime factors
:math:`p_1`\ and :math:`p_2` which satisfy :math:`p_1\cdot p_2=N` for a
given large number of :math:`N` in polynomial time. Before introducing
the steps of Shor’s algorithm, I would like to discuss more about the
number theory.

**Number theory:**

Factorization involves some knowledge of number theory and can be
reduced to the following functions:

.. math::


   f(x) = a^x\mathrm{mod} N

For a seek of :math:`a`, :math:`a` and :math:`N`\ are mutually
exclusive, you can get a factor right away by calling :math:`gcd (a,N)`,
as the period :math:`r` of :math:`f(x)` satisfies :math:`f(x) = (x+r)`.
In this case, we can get :math:`a^x = a^{x+r}\mathrm{mod}N\forall x`. And some integer :math:`q`
satisfies :math:`a^r-1 = (a^{r/2}-1)(a^{r/2}+1) = qN​` in the equation :math:`a^r = 1+qN​`.
This also shows that you can find the factor of :math:`N​` by using :math:`gcd​`.

Therefore, the core of Shor’s algorithm is to transform the problem of
large number decomposition into the problem of finding period (as shown
in the following). We can construct a unitary matrix :math:`U_{x,N} \left| {j} \right\rangle \left| {k} \right\rangle -> \left| {j} \right\rangle \left| {x^jk \mathrm{mod} N} \right\rangle`,
then the result of end satisfies :math:`{x^r} = 1\left( {\bmod N} \right)`.

Below we take :math:`N = 15` as an example to introduce the steps of Shor’s
algorithm in factorization:

   (1) Select an arbitrary number, such as :math:`a = 2` (<15)

   (2) :math:`gcd⁡ (a,N) = gcd (2,15) = 1`

   (3) Fine the period of function :math:`f(x) = {a^x}\bmod N`, which
       satisfies :math:`f(x + r) = f\left( x \right)`

   (4) Get r = 4 through the circuit diagram calculation

   (5) :math:`gcd ({a^{\frac{r}{2}}} + 1,N) = gcd (5,15) = 5`

   (6) :math:`gcd ({a^{\frac{r}{2}}} - 1,N) = gcd (3,15) = 5`

   (7) For :math:`N = 15`, the two decomposed prime numbers are 3 and 5. Problem solved.

Quantum circuit of Shor’s algorithm:

.. image:: /images/shor.png
