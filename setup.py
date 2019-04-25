import os
import re
import sys
import platform
import subprocess

from setuptools import setup, Extension, Feature, find_packages
from setuptools.command.build_ext import build_ext
from distutils.version import LooseVersion

# This reads the __version__ variable from _version.py
exec(open('_version.py').read())

# Readme file as long_description:
long_description = open('README.rst').read()

# Read in requirements.txt
with open('requirements.txt', 'r') as f_requirements:
    requirements = f_requirements.readlines()
requirements = [r.strip() for r in requirements]

class get_pybind_include(object):
    """Helper class to determine the pybind11 include path

    The purpose of this class is to postpone importing pybind11
    until it is actually installed, so that the ``get_include()``
    method can be invoked. """

    def __init__(self, user=False):
        self.user = user

    def __str__(self):
        import pybind11
        return pybind11.get_include(self.user)


class CMakeExtension(Extension):
    def __init__(self, target, name, sourcedir=''):
        self.target = target
        Extension.__init__(self, name, sources=[])
        self.sourcedir = os.path.abspath(sourcedir)


class CMakeBuild(build_ext):
    def run(self):
        try:
            out = subprocess.check_output(['cmake', '--version'])
        except OSError:
            raise RuntimeError("CMake must be installed to build the following extensions: " +
                               ", ".join(e.name for e in self.extensions))

        if platform.system() == "Windows":
            cmake_version = LooseVersion(re.search(r'version\s*([\d.]+)', out.decode()).group(1))
            if cmake_version < '3.1.0':
                raise RuntimeError("CMake >= 3.1.0 is required on Windows")

        for ext in self.extensions:
            self.build_extension(ext)

    def build_extension(self, ext):
        extdir = os.path.abspath(os.path.dirname(self.get_ext_fullpath(ext.name)))
        cmake_args = ['-DCMAKE_LIBRARY_OUTPUT_DIRECTORY=' + extdir,
                      '-DPYTHON_EXECUTABLE=' + sys.executable,
                      '-DCMAKE_VERBOSE_MAKEFILE:BOOL=ON']

        cfg = 'Debug_MPI' if self.debug else 'Release_MPI'
        build_args = ['--config', cfg]

        if platform.system() == "Windows":
            cmake_args += ['-DCMAKE_LIBRARY_OUTPUT_DIRECTORY_{}={}'.format(cfg.upper(), extdir)]
            if sys.maxsize > 2**32:
                cmake_args += ['-A', 'x64']
            build_args += ['--', '/m']
        else:
            cmake_args += ['-DCMAKE_BUILD_TYPE=' + cfg]
            build_args += ['--', '-j2']

        env = os.environ.copy()
        env['CXXFLAGS'] = '{} -DVERSION_INFO=\\"{}\\"'.format(env.get('CXXFLAGS', ''),
                                                              self.distribution.get_version())
        if not os.path.exists(self.build_temp):
            os.makedirs(self.build_temp)
        subprocess.check_call(['cmake', ext.sourcedir] + cmake_args, cwd=self.build_temp, env=env)
        subprocess.check_call(['cmake', '--build', '.', '--target', ext.target] + build_args, cwd=self.build_temp)


cppsim_mpi = Feature(
    "Distributed C++ simulator",
    standard=True,
    packages=['hiq/projectq/backends'
        , 'hiq/projectq/backends/_sim'
        , 'hiq/projectq/ops'],
    ext_modules=[CMakeExtension('_cppsim_mpi', 'hiq/projectq/backends/_sim/_cppsim_mpi')]
)

cppstabsim = Feature(
    "StabilizerSimulator",
    standard=True,
    ext_modules=[CMakeExtension('_cppstabsim', 'hiq/projectq/backends/_sim/_cppstabsim')]
)

scheduler = Feature(
    "Sheduler C++ backend",
    standard=True,
    packages=['hiq/projectq/cengines'],
    ext_modules=[CMakeExtension('_sched_cpp', 'hiq/projectq/cengines/_sched_cpp')]
)


if os.environ.get('READTHEDOCS') == 'False':
    setup(
        name='hiqsimulator',
        version='__version__',
        author='hiq',
        author_email='hiqinfo@huawei.com',
        description='A high performance distributed quantum simulator',
        long_description=long_description,
        url="https://github.com/Huawei-HiQ/HiQsimulator",
        features={'cppsim-mpi': cppsim_mpi, 'scheduler': scheduler, 'cppstabsim': cppstabsim},
        install_requires=requirements,
        cmdclass=dict(build_ext=CMakeBuild),
        zip_safe=False,
        license='Apache 2',
        packages=find_packages()
    )
else:
    setup(
        name='hiqsimulator',
        version='__version__',
        author='hiq',
        author_email='hiqinfo@huawei.com',
        description='A high performance distributed quantum simulator',
        long_description=long_description,
        url="https://github.com/Huawei-HiQ/HiQsimulator",
        install_requires=./docs/requirements_rtd,
        zip_safe=False,
        license='Apache 2',
        packages=['hiq/projectq/backends'
            , 'hiq/projectq/backends/_sim'
            , 'hiq/projectq/cengines'
            , 'hiq/projectq/ops'
        ]
    )
