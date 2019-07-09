# Copyright Tomas Zeman 2018.
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

set(_clang_format_default_options
    "AccessModifierOffset: -5"
    "AllowShortFunctionsOnASingleLine: None"
    "AllowShortIfStatementsOnASingleLine: false"
    "AllowShortLoopsOnASingleLine: false"
    "BreakBeforeBraces: Custom"
    "BreakBeforeBinaryOperators: All"
    "BraceWrapping:"
    "  AfterClass: true"
    "  AfterControlStatement: false"
    "  AfterEnum: true"
    "  AfterExternBlock: true"
    "  AfterFunction: true"
    "  AfterNamespace: true"
    "  AfterStruct: true"
    "  AfterUnion: true"
    "  BeforeCatch: true"
    "  BeforeElse: true"
    "  SplitEmptyFunction: false"
    "  SplitEmptyRecord: false"
    "  SplitEmptyNamespace: false"
    "BreakInheritanceList: BeforeComma"
    "ColumnLimit: 80"
    "IndentWidth: 5"
    "KeepEmptyLinesAtTheStartOfBlocks: false"
    "NamespaceIndentation: Inner"
    "ReflowComments: true"
    "SortIncludes: true"
    "SpaceAfterCStyleCast: true"
    "SpaceAfterTemplateKeyword: true"
    "SpaceBeforeAssignmentOperators: true"
    "SpaceBeforeInheritanceColon: true"
    "SpaceBeforeParens: ControlStatements"
    "SpaceBeforeRangeBasedForLoopColon: false"
    "SpacesInAngles: false"
    "SpacesInCStyleCastParentheses: false"
    "SpaceInEmptyParentheses: false"
    "Standard: Cpp11"
    "TabWidth: 4"
    "UseTab: Never")

set(_clang_format_5_options
    "AlignEscapedNewlines: Right"
    "CompactNamespaces: true"
    "FixNamespaceComments: true"
    "SortUsingDeclarations: true")

set(_clang_format_6_options "IncludeBlocks: Regroup"
    "IndentPPDirectives: AfterHash")

macro(_generate_clang_format_config_content content)
  set(${content} ${_clang_format_default_options})
  if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 4.99)
    list(APPEND ${content} ${_clang_format_5_options})
  endif()
  if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 5.99)
    list(APPEND ${content} ${_clang_format_6_options})
  endif()
  list(SORT ${content})
  list(REMOVE_ITEM ${content} "BraceWrapping:")
  list(FIND ${content} "  AfterClass: true" _idx)
  list(INSERT ${content} ${_idx} "BraceWrapping:")
  set(${content} "BasedOnStyle: Google;;${${content}}")
endmacro()

function(clangformat_setup clangformat_srcs)
  if(NOT CLANGFORMAT_EXECUTABLE)
    set(CLANGFORMAT_EXECUTABLE clang-format)
  endif()

  _generate_clang_format_config_content(_clang_format_content)
  file(WRITE ${PROJECT_SOURCE_DIR}/.clang-format "")
  foreach(line ${_clang_format_content})
    file(APPEND ${PROJECT_SOURCE_DIR}/.clang-format "${line}\n")
  endforeach(line)

  if(NOT EXISTS ${CLANGFORMAT_EXECUTABLE})
    find_program(clangformat_executable_tmp ${CLANGFORMAT_EXECUTABLE})
    if(clangformat_executable_tmp)
      set(CLANGFORMAT_EXECUTABLE ${clangformat_executable_tmp})
      unset(clangformat_executable_tmp)
    else()
      message(
        FATAL_ERROR "ClangFormat: ${CLANGFORMAT_EXECUTABLE} not found! Aborting"
        )
    endif()
  endif()

  foreach(clangformat_src ${clangformat_srcs})
    get_filename_component(clangformat_src ${clangformat_src} ABSOLUTE)
    list(APPEND clangformat_srcs_tmp ${clangformat_src})
  endforeach()
  set(clangformat_srcs "${clangformat_srcs_tmp}")
  unset(clangformat_srcs_tmp)

  add_custom_target(${PROJECT_NAME}_clangformat
                    COMMAND ${CLANGFORMAT_EXECUTABLE} -style=file -i
                            ${clangformat_srcs}
                    COMMENT "Formating with ${CLANGFORMAT_EXECUTABLE} ...")

  if(TARGET clangformat)
    add_dependencies(clangformat ${PROJECT_NAME}_clangformat)
  else()
    add_custom_target(clangformat DEPENDS ${PROJECT_NAME}_clangformat)
  endif()
endfunction()
