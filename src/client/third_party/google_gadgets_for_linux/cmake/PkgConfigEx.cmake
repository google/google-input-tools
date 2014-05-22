#
# Copyright 2008 Google Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

#! Use pkg-config to get information of a library package and update current
#! cmake environment.
#!
#! @param _package the package name.
#! @param _min_version minimum version required.
#! @param[out] _inc_dirs return the include directories, can be used in
#!     @c INCLUDE_DIRECTORIES.
#! @param[out] _definitions return the C/C++ definitions, can be used in
#!     @c ADD_DEFINITIONS.
#! @param[out] _link_dirs return the link directories, can be used in
#!     @c LINK_DIRECTORIES.
#! @param[out] _linker_flags return the link flags, can be appended to
#!     @c CMAKE_EXE_LINKER_FLAGS or @c CMAKE_SHARED_LINKER_FLAGS variable.
#! @param[out] _libraries return the list of library names, can be used in
#!     @c TARGET_LINK_LIBRARIES.
#! @paramoptional[out] _found return if the required library is found.  If this
#!     parameter is not provided, an error message will shown and cmake quits.
MACRO(PKGCONFIG_EX _package _min_version
      _inc_dirs _definitions _link_dirs _linker_flags _libraries)
  EXECUTE_PROCESS(
    COMMAND pkg-config ${_package} --atleast-version=${_min_version}
    RESULT_VARIABLE PKGCONFIG_EX_return_value)
  IF(${PKGCONFIG_EX_return_value} EQUAL 0)
    # Get include directories.
    EXECUTE_PROCESS(
      COMMAND pkg-config ${_package} --cflags-only-I
      OUTPUT_VARIABLE ${_inc_dirs}
      OUTPUT_STRIP_TRAILING_WHITESPACE)
    STRING(REGEX REPLACE "^-I" "" ${_inc_dirs} "${${_inc_dirs}}")
    STRING(REGEX REPLACE " -I" ";" ${_inc_dirs} "${${_inc_dirs}}")

    # Get other cflags other than -I include directories.
    EXECUTE_PROCESS(
      COMMAND pkg-config ${_package} --cflags-only-other
      OUTPUT_VARIABLE ${_definitions}
      OUTPUT_STRIP_TRAILING_WHITESPACE)

    # Get library names.
    EXECUTE_PROCESS(
      COMMAND pkg-config ${_package} --libs-only-l
      OUTPUT_VARIABLE ${_libraries}
      OUTPUT_STRIP_TRAILING_WHITESPACE)
    STRING(REGEX REPLACE "^-l" "" ${_libraries} "${${_libraries}}")
    STRING(REGEX REPLACE " -l" ";" ${_libraries} "${${_libraries}}")

    # Get -L link flags.
    EXECUTE_PROCESS(
      COMMAND pkg-config ${_package} --libs-only-L
      OUTPUT_VARIABLE ${_link_dirs}
      OUTPUT_STRIP_TRAILING_WHITESPACE)
    STRING(REGEX REPLACE "^-L" "" ${_link_dirs} "${${_link_dirs}}")
    STRING(REGEX REPLACE " -L" ";" ${_link_dirs} "${${_link_dirs}}")

    # Get other link flags.
    EXECUTE_PROCESS(
      COMMAND pkg-config ${_package} --libs-only-other
      OUTPUT_VARIABLE ${_linker_flags}
      OUTPUT_STRIP_TRAILING_WHITESPACE)

    IF(${ARGC} GREATER 7)
      SET(${ARGV7} 1)
    ENDIF(${ARGC} GREATER 7)
  ELSEIF(${ARGC} GREATER 7)
    SET(${ARGV7} 0)
  ELSE(${ARGC} GREATER 7)
    MESSAGE(FATAL_ERROR
      "Required package ${_package} version>=${_min_version} not found")
  ENDIF(${PKGCONFIG_EX_return_value} EQUAL 0)
ENDMACRO(PKGCONFIG_EX _package _min_version
         _inc_dirs _definitions _link_dirs _linker_flags _libraries)

#! Convenient wrapper of @c PKGCONFIG_EX that can assign to a set of variables
#! using the following naming conventions:
#!   - prefix_INCLUDE_DIR
#!   - prefix_DEFINITIONS
#!   - prefix_LINK_DIR
#!   - prefix_LINKER_FLAGS
#!   - prefix_LIBRARIES
#! @param _package the package name.
#! @param _min_version minimum version required.
#! @param _prefix the prefix of the output variable names.
#! @paramoptional[out] _found return if the required library is found.  If this
#!     parameter is not provided, an error message will shown and cmake quits.
MACRO(GET_CONFIG _package _min_version _prefix)
  PKGCONFIG_EX(${_package} ${_min_version}
    ${_prefix}_INCLUDE_DIR ${_prefix}_DEFINITIONS
    ${_prefix}_LINK_DIR ${_prefix}_LINKER_FLAGS ${_prefix}_LIBRARIES
    ${ARGV3})
ENDMACRO(GET_CONFIG _package _min_version _prefix)

#! Apply configurations found by @c GET_CONFIG to the current environment
#! for a package. Libraries are not applied. It should be applied to specific
#! target using the @c TARGET_LINK_LIBRARIES macro.
MACRO(APPLY_CONFIG _prefix)
  INCLUDE_DIRECTORIES(${${_prefix}_INCLUDE_DIR})
  ADD_DEFINITIONS(${${_prefix}_DEFINITIONS})
  LINK_DIRECTORIES(${${_prefix}_LINK_DIR})
  SET(CMAKE_EXE_LINKER_FLAGS
    "${CMAKE_EXE_LINKER_FLAGS} ${${_prefix}_LINKER_FLAGS}")
  SET(CMAKE_SHARED_LINKER_FLAGS
    "${CMAKE_SHARED_LINKER_FLAGS} ${${_prefix}_LINKER_FLAGS}")
ENDMACRO(APPLY_CONFIG _prefix)

MACRO(PKG_GET_VARIABLE _package _variable _value)
  EXECUTE_PROCESS(
    COMMAND pkg-config "--variable=${_variable}" "${_package}"
    OUTPUT_VARIABLE "${_value}"
    OUTPUT_STRIP_TRAILING_WHITESPACE)
ENDMACRO(PKG_GET_VARIABLE _package _variable _value)
