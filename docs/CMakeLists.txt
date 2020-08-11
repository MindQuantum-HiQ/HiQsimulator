# ==============================================================================
#
# Copyright 2020 <Huawei Technologies Co., Ltd>
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# ==============================================================================

find_package(Python REQUIRED COMPONENTS Interpreter)
find_package(Sphinx REQUIRED)

# ==============================================================================

set(_hiq_public_headers)
foreach(tgt ${_doc_targets})
  get_target_property(_header_list ${tgt} INTERFACE_SOURCES)
  list(APPEND _hiq_public_headers ${_header_list})
endforeach()

# ==============================================================================
# Doxygen

set(DOXYGEN_INPUT_DIR ${PROJECT_SOURCE_DIR}/src)
set(DOXYGEN_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/doxygen)
set(DOXYGEN_INDEX_FILE ${DOXYGEN_OUTPUT_DIR}/html/index.html)
set(DOXYFILE_IN ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile)
set(DOXYFILE_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

# ------------------------------------------------------------------------------

file(READ ${DOXYFILE_IN} _doxyfile_content)
string(REPLACE "INPUT                  = ../src"
               "INPUT                  = ${DOXYGEN_INPUT_DIR}"
               _doxyfile_content
               ${_doxyfile_content})
# Not using DOXYGEN_OUTPUT_DIR because Doxygen adds '/doxygen' to the end of
# this path
string(REPLACE "OUTPUT_DIRECTORY       ="
               "OUTPUT_DIRECTORY       = ${CMAKE_CURRENT_BINARY_DIR}"
               _doxyfile_content
               ${_doxyfile_content})
file(WRITE ${DOXYFILE_OUT} ${_doxyfile_content})
file(MAKE_DIRECTORY ${DOXYGEN_OUTPUT_DIR})

# ------------------------------------------------------------------------------

add_custom_command(OUTPUT ${DOXYGEN_INDEX_FILE}
                   DEPENDS ${_hiq_public_headers}
                   COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYFILE_OUT}
                   MAIN_DEPENDENCY ${DOXYFILE_OUT}
                   ${DOXYFILE_IN}
                   COMMENT "Generating docs")

add_custom_target(doxygen DEPENDS ${DOXYGEN_INDEX_FILE})

# ==============================================================================
# Sphinx (sphinx-build)

set(SPHINX_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/source)
set(SPHINX_BUILD ${CMAKE_CURRENT_BINARY_DIR}/sphinx)
set(SPHINX_INDEX_FILE ${SPHINX_BUILD}/index.html)

# ------------------------------------------------------------------------------

# Only regenerate Sphinx when:
# - Doxygen has rerun
# - Our doc files have been updated
# - The Sphinx config has been updated

add_custom_command(
  OUTPUT ${SPHINX_INDEX_FILE}
  COMMAND $<TARGET_FILE:Python::Interpreter>
          ${Sphinx_EXECUTABLE}
          -b
          html
          # Tell Breathe where to find the Doxygen output
          -Dbreathe_projects.HiQSimulator=${DOXYGEN_OUTPUT_DIR}/xml
          ${SPHINX_SOURCE} ${SPHINX_BUILD}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  DEPENDS
    # Other docs files you want to track should go here (or in some variable)
    ${CMAKE_CURRENT_SOURCE_DIR}/source/index.rst ${DOXYGEN_INDEX_FILE}
  MAIN_DEPENDENCY ${SPHINX_SOURCE}/conf.py
  COMMENT "Generating documentation with Sphinx")

# ==============================================================================
# Sphinx (sphinx-build)

set(DOCS_INDEX_FILE ${CMAKE_CURRENT_BINARY_DIR}/index.html)
set(DOCS_INDEX_FILE_IN ${CMAKE_CURRENT_SOURCE_DIR}/index.html.in)

# ------------------------------------------------------------------------------

add_custom_command(OUTPUT ${DOCS_INDEX_FILE}
                   COMMAND ${CMAKE_COMMAND}
                           -E
                           copy
                           ${DOCS_INDEX_FILE_IN}
                           ${DOCS_INDEX_FILE})

# ==============================================================================
# Add an install target to install the docs

# Nice named target so we can run the job easily
add_custom_target(docs DEPENDS doxygen ${SPHINX_INDEX_FILE} ${DOCS_INDEX_FILE})

include(GNUInstallDirs)
install(DIRECTORY ${SPHINX_BUILD} DESTINATION ${CMAKE_INSTALL_DOCDIR})
