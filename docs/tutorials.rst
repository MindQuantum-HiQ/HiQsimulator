.. _tutorials:

Tutorials
=========


Welcome to the HiQ manual. The Huawei quantum computing software HiQ 
provides a complete programming environment for quantum circuit composing 
and simulations.HiQ is based on and compatible with
`ProjectQ <https://github.com/ProjectQ-Framework/ProjectQ>`__. So far,
HiQ supports multiple hardware backends and an API frontend, and has
been deployed on `HUAWEI CLOUD <http://hiq.huaweicloud.com>`__ to provide quantum computing simulation
services. The frontend of the HiQ simulator has a quantum circuit
composer GUI, a coding API, and a BlockUI for hybrid classical-quantum
programming. The backend is compatible with the API of ProjectQ, and can
be compiled to various sets of instruction for different quantum chips,
and to high-performance C++ programs for parallel and distributed
circuit simulations on cloud. The HiQ simulator, based on its
functionalities, includes a single-amplitude simulation engine, a
full-amplitude simulation engine and an error-correction circuit
simulation engine.

For a detailed documentation of a scene, click on its name below:

.. toctree::
    :maxdepth: 1
    :titlesonly:
    
    /tutorials/tutorials.cloud
    /tutorials/tutorials.local
    /tutorials/tutorials.backends