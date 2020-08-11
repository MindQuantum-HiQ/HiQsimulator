.. _installation:

Requirements
============

.. toctree::
   :maxdepth: 2

In order to get started with HiQ-Circuit, you will need to have a C++ compiler installed on your system as well as a few libraries:

    - CMake >= 3.5
    - MPI
    - Boost >= 1.69
    - hwloc
    - gflags
    - glog

Below you will find detailed installation instructions for various operating systems. If you have the dependencies already installed, you can directly skip to :ref:`hiq_circuit_installation`.

Pre-requisite installation
==========================

Here you will find detailed installation instructions for various operating systems.

Linux
-----

Ubuntu/Debian
+++++++++++++

After having installed the build tools (for g++):

.. code-block:: bash

   sudo apt-get install build-essential

You only need to install Python (and the package manager). For version 3, run

.. code-block:: bash

   sudo apt-get install python3-dev python3-pip python3-tk


Then install the rest of the required libraries using the following command:

.. code-block:: bash

   sudo apt-get install cmake \
       libopenmpi-dev openmpi-bin \
       libboost-all-dev \
       libhwloc-dev \
       libgoogle-glog-dev

ArchLinux/Manjaro
+++++++++++++++++

Make sure that you have a C/C++ compiler installed:

.. code-block:: bash

   sudo pacman -Syu gcc

You only need to install Python (and the package manager). For version 3, run

.. code-block:: bash

   sudo pacman -Syu python python-pip tk

Then install the rest of the required libraries using the following command:

.. code-block:: bash

   sudo pacman -Syu cmake \
       openmpi \
       boost \
       hwloc \
       google-glog


CentOS 7
++++++++

Run the following commands:

.. code-block:: bash

   sudo yum install -y epel-release
   sudo yum install -y centos-release-scl
   sudo yum install -y devtoolset-8
   sudo yum check-update -y

   scl enable devtoolset-8 bash
   sudo yum install -y gcc-c++ make git cmake3
   sudo yum install -y python3 python3-devel python3-pip
   sudo yum install -y openmpi-devel
   sudo yum install -y boost169-devel boost169-openmpi-devel boost169-python3-devel
   sudo yum install -y glog-devel gflags-devel hwloc-devel

   sudo alternatives --install /usr/local/bin/cmake cmake /usr/bin/cmake3 20 \
	--slave /usr/local/bin/ctest ctest /usr/bin/ctest3 \
	--slave /usr/local/bin/cpack cpack /usr/bin/cpack3 \
	--slave /usr/local/bin/ccmake ccmake /usr/bin/ccmake3 \
	--family cmake

   . /usr/share/Modules/init/sh
   module load mpi

.. note::

   Note that if you open a new shell prompt, you might need to re-enter the following command in order to properly setup your environment for MPI

   .. code-block:: bash

      module load mpi


CentOS 8
++++++++


Run the following commands:

.. code-block:: bash

   sudo dnf config-manager --set-enabled PowerTools
   sudo yum install -y epel-release
   sudo yum check-update -y

   sudo yum install -y gcc-c++ make git cmake
   sudo yum install -y python3 python3-devel python3-pip
   sudo yum install -y openmpi-devel
   sudo yum install -y boost-devel boost-openmpi-devel boost-python3-devel
   sudo yum install -y glog-devel gflags-devel hwloc-devel

   . /etc/profile.d/modules.sh
   module load mpi

.. note::

   Note that if you open a new shell prompt, you might need to re-enter the following command in order to properly setup your environment for MPI

   .. code-block:: bash

      module load mpi


Mac OS
------

We require that a C++ compiler is installed on your system. There are two options you can choose from:

   1. Using Homebrew
   2. Using MacPorts


Before moving on, install the XCode command line tools by opening a terminal window and running the following command:

.. code-block:: bash

   xcode-select --install

Homebrew
++++++++

Install Homebrew with the following command:

.. code-block:: bash

   /usr/bin/ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"

Then proceed to install Python as well as a C++ compiler (note: gcc installed via Homebrew may lead to some issues therefore we choose clang):

.. code-block:: bash

   brew install python llvm tcl-tk

Then install the rest of the required libraries/programs using the following command:

.. code-block:: bash

   sudo port install cmake openmpi boost-mpi hwloc glog


MacPorts
++++++++

Visit `macports.org <https://www.macports.org/install.php>`_ and install the latest version that corresponds to your operating system's version. Afterwards, open a new terminal window.

Then, use macports to install Python 3.8 with pip by entering the following command

.. code-block:: bash

   sudo port install python38 py38-pip py38-tkinter

A warning might show up about using Python from the terminal.In this case, you should also install

.. code-block:: bash

   sudo port install py38-gnureadline

Then install the rest of the required libraries/programs and a C++ compiler using the following command:

.. code-block:: bash

   sudo port install cmake clang-9.0 openmpi-clang boost+openmpi+python38 hwloc google-glog


Windows
-------

On Windows, we only support installing the some of the dependencies using the `Chocolatey package manager <https://chocolatey.org/>`_. Unfortunately, not all the required libraries can be installed using Chocolatey at the time of this writing. However, we provide some pre-compiled binaries at https://github.com/Huawei-HiQ/windows-binaries/releases/

In the following, all the commands are to be run from within a PowerShell window. In some cases, you might need to run PowerShell as administrator.

Chocolatey
++++++++++

First install Chocolatey using the installer following the instructions on their `website <https://chocolatey.org/docs/installation>`_. Once that is done, you can start by installing some of the required packages. Reboot as needed during the process.

.. code-block:: powershell

   choco install -y visualstudio2019-workload-vctools --includeOptional
   choco install -y windows-sdk-10-version-2004-all
   choco install -y cmake git

MPI libraries
+++++++++++++

We can now move onto installing the MPI libraries and programs. Download the latest release of the Microsoft MPI from https://github.com/microsoft/Microsoft-MPI/releases/latest. Download both the `msmpisdk.msi` and `msmpisetup.exe` files. Once downloaded, you can install them by simply running the following commands:

.. code-block:: powershell

   .\msmpisetup.exe -unattend -full
   msiexec.exe /quiet /i .\msmpisdk.msi

   # To download the two files above, you could install wget using:
   # choco install -y wget
   # and then download the packages using commands similar to the ones below:
   # wget https://github.com/microsoft/Microsoft-MPI/releases/download/v10.1.1/msmpisdk.msi
   # wget https://github.com/microsoft/Microsoft-MPI/releases/download/v10.1.1/msmpisetup.exe

.. note::

   You can check that the installation was successful by checking that you have both of the following folders present on your system:

   - ``C:\Program Files\Microsoft MPI``
   - ``C:\Program Files (x86)\Microsoft SDKs\MPI``

Python
++++++

Installing Python is as simply as running the following commands:

.. code-block:: powershell

   choco install -y python3 --version 3.8.3
   cmd /c mklink "C:\Python38\python3.exe" "C:\Python38\python.exe"

You can then install the required Python dependencies using the following commands:

.. code-block:: powershell

   python3 -m pip install -U mpi4py hiq-projectq

Boost
+++++

You can find pre-compiled version of Boost here: https://github.com/Huawei-HiQ/windows-binaries/releases/. Download the latest release corresponding to your Visual C++ version and unzip the content of the archive to ``C:\Boost``. For example using:

.. code-block:: powershell

   Expand-Archive -Path boost173_python38_msmpi10_1_1_vs2019.zip -DestinationPath "C:\Boost"

.. note::

   If you cannot find a pre-compiled binary for your system configuration, you will have to manually compile and install the Boost libraries.

   Download the latest source ZIP file on your computer and run the following commands:

   .. code-block:: powershell

      Expand-Archive -Path 'boost*.zip' -DestinationPath $env:temp
      Move-Item $env:temp\boost_* $env:temp\boost_src
      Set-Location $env:temp\boost_src

      $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
      $vcvarspath = &$vswhere -latest -products * `
	  -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
	  -property installationPath

      cmd.exe /c "call `"$vcvarspath\VC\Auxiliary\Build\vcvars64.bat`" `
	  && set > %temp%\vcvars.txt"

      Get-Content "$env:temp\vcvars.txt" | Foreach-Object {
	  if ($_ -match "^(.*?)=(.*)$") {
	      Set-Content "env:\$($matches[1])" $matches[2]
	  }
      }

      .\bootstrap.bat

      echo "using msvc ;" > user-config.jam
      echo "using python ;" > user-config.jam
      echo "using mpi ;" > user-config.jam

      .\b2.exe --user-config=user-config.jam `
	  variant=release `
	  debug-symbols=off `
	  address-model=64 `
	  threading=multi `
	  runtime-link=shared `
	  link=shared,static `
	  --layout=system `
	  -j4 -q install


Eigen
+++++

Download the latest version of Eigen from their website
http://eigen.tuxfamily.org/index.php?title=Main_Page

Then compile and Compile and install Eigen using CMake:

.. code-block:: powershell

   Expand-Archive -Path 'eigen-*.zip' -DestinationPath $env:temp
   Move-Item $env:temp\eigen* $env:temp\eigen_src
   New-Item $env:temp\eigen_src\build -ItemType Directory

   Set-Location $env:temp\eigen_src\build
   cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF ..
   cmake --build . --config Release --target install


Gflags
++++++

Download the latest version of gflags from their website
https://github.com/gflags/gflags/releases/latest

Then compile and Compile and install Eigen using CMake:

.. code-block:: powershell

   Expand-Archive -Path '$env:temp\gflags-*.zip' -DestinationPath '$env:temp'; `
   Move-Item $env:temp\gflags* $env:temp\gflags_src
   New-Item $env:temp\gflags_src\cmake_build -ItemType Directory
   Set-Location $env:temp\gflags_src\cmake_build

   cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF ..
   cmake --build . --config Release --target install


Glog
++++

Download the latest version of gflags from their website
https://github.com/google/glog/releases/latest

Then compile and Compile and install Eigen using CMake:

.. code-block:: powershell

   Expand-Archive -Path 'v*.zip' -DestinationPath '$env:temp'; `
   Move-Item $env:temp\glog* $env:temp\glog_src
   New-Item $env:temp\glog_src\cmake_build -ItemType Directory
   Set-Location $env:temp\glog_src\cmake_build

   cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF ..
   cmake --build . --config Release --target install

Hwloc
+++++

Download the latest Windows pre-compiled binaries from https://www.open-mpi.org/projects/hwloc/ and unzip them to ``C:\Program Files (x86)\hwloc``

.. code-block:: powershell

   Expand-Archive -Path hwloc-win64-build-*.zip -DestinationPath "C:\Program Files (x86)"
   Move-Item "C:\Program Files (x86)\hwloc*" "C:\Program Files (x86)\hwloc"


.. _hiq_circuit_installation:

HiQ-Circuit installation
========================

The latest version of HiQ-Circuit can be found on Pypi:
https://pypi.org/project/hiq-circuit/. In order to start using hiq-circuit,
simply run the following command

.. code-block:: bash

	python -m pip install --user hiq-circuit

or, alternatively, `clone/download <https://github.com/Huawei-HiQ/HiQSimulator>`_ this repo (e.g., to your /home directory) and run

.. code-block:: bash

	cd /home/hiq-circuit
	python -m pip install --user .

.. note::
    The HiQsimulator relies on HiQ-Projectq (programming language, compiler),
    which will be installed automatically.  HiQ-ProjectQ is a fork of ProjectQ
    that is 100% compatible with the original ProjectQ package but with some
    added functionality.  You can find more information in the `projectq
    tutorials <https://projectq.readthedocs.io/en/latest/index.html>`__.
    
.. note::    
    If you download the project code by copying or some other methods (ie. not
    by cloning the repository as shown above), you might need to manually copy
    the latest version of the `pybind11 source code
    <https://github.com/pybind/pybind11>`__ in the corresponding folder within
    the root folder of hiq-circuit.

Linux
-----

CentOS 7 & 8
++++++++++++

If you are using the ``boost169*`` packages, you might need to run some extra commands in order for the compilation to be successful:

.. code-block:: bash

   export BOOST_ROOT=/usr/include/boost169
   export BOOST_LIBRARYDIR=/usr/lib64/boost169
   sudo ln -s /usr/lib64/openmpi/lib/boost169/libboost_mpi.so /usr/lib64/boost169/
   python3 -m pip install --user hiq-circuit


Mac OS
------

On Mac OS, you might want to specify the compiler explicitely so that you benefit from all the functionalities.

Homebrew
++++++++

Run the following command:

.. code-block::

   env CC=/usr/local/opt/llvm/bin/clang \
   CXX=/usr/local/opt/llvm/bin/clang++ \
   python3 -m pip install --user hiq-circuit

MacPorts
++++++++

Run the following command:

.. code-block::

   env CC=clang-mp-9.0 CXX=clang++-mp-9.0 \
   python3 -m pip install --user hiq-circuit
