XSIMD Fallback
==============

The XSIMD fallback library is aimed at compiling XSIMD code for people that do not have XSIMD installed, while remaining fully compatible with the original XSIMD. It is safe to use this library alongside the original XSIMD as the functions and operators defined here are removed from overload resolution for the original XSIMD types.

One possible use cases would be to allow this code to compile, even in the case where no AVX2 support is enabled:

.. code-block:: c

   xsimd::batch<uint32_t, 8> v; // only defined by XSIMD if AVX2 support is enabled

Compared to the one provided as part of XSIMD, this implementation makes heavy use of expression templates to avoid unnessary copies and temporaries for better performance. To take full advantage of these features, it is highly recommended to use a recent version of your compiler and make sure to enable compiler optimizations during the compilation process.

.. note::

   The recommended way of using this library is by including the ``xsimd_include.hpp`` file. In this way, you ensure that your code will work regardless of whether the user has the XSIMD library installed on his system or not.

.. toctree::
   :caption: Table of contents
   
   xsimd_fallback_batch
   xsimd_fallback_expressions
   xsimd_fallback_functions
   xsimd_fallback_misc
   xsimd_fallback_details
