include(CheckCXXCompilerFlag)

# Find a compatible compiler flag from a series of list of possible flags
# For each list of whitespace separated compiler flags passed in argument, this
# function will append or save the compatible flags in ${var_prefix}_c and
# ${var_prefix}_cxx (if C++ is enabled)
macro(check_compiler_flags var_prefix)
  set(_cxx_opts)

  foreach(_flag_list ${ARGN})
    separate_arguments(_flag_list)

    foreach(_flag ${_flag_list})
      # Drop the first character (most likely either '-' or '/')
      string(SUBSTRING ${_flag} 1 -1 _flag_name)
      string(REGEX REPLACE "[-:/]" "_" _flag_name ${_flag_name})

      check_cxx_compiler_flag(${_flag} cxx_compiler_has_${_flag_name})
      if(cxx_compiler_has_${_flag_name})
	list(APPEND _cxx_opts ${_flag})
	break()
      endif()
    endforeach()
  endforeach()

  if(DEFINED ${var_prefix}_cxx)
    list(APPEND ${var_prefix}_cxx ${_cxx_opts})
  else()
    set(${var_prefix}_cxx ${_cxx_opts})
  endif()
endmacro()

# ==============================================================================

macro(add_avx2_to_target target)
  if(NOT _avx2_flag_cxx)
    message(FATAL_ERROR "Compiler does not support AVX2 support. This is "
                        "required for target: ${target}")
  endif()
  target_compile_options(${target} PRIVATE ${_avx2_flag_cxx})
endmacro(add_avx2_to_target)

# ==============================================================================
