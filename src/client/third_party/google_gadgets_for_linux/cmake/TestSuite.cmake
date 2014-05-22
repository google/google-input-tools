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

# Some commands under different platforms.
IF(WIN32)
  MACRO(TEST_WRAPPER _wrappee_base)
    # TODO: not implemented yet.
  ENDMACRO(TEST_WRAPPER)
ELSE(WIN32)
  MACRO(ADD_TEST_EXECUTABLE target)
    IF(NOT ADD_TEST_EXECUTABLE_no_werror) 
      SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-extra -Wno-error")
      SET(ADD_TEST_EXECUTABLE_no_werror TRUE) 
    ENDIF(NOT ADD_TEST_EXECUTABLE_no_werror) 
    ADD_EXECUTABLE("${target}" EXCLUDE_FROM_ALL ${ARGN})
  ENDMACRO(ADD_TEST_EXECUTABLE)

  MACRO(TEST_WRAPPER _wrappee_base)
    SET(TEST_WRAPPER_template ${CMAKE_SOURCE_DIR}/cmake/test_wrapper.sh)
    GET_TARGET_PROPERTY(TEST_WRAPPER_wrappee_location ${_wrappee_base} LOCATION)
    SET(TEST_WRAPPER_output ${TEST_WRAPPER_wrappee_location}.sh)
    ADD_CUSTOM_TARGET("${_wrappee_base}_wrap" ALL
      DEPENDS ${TEST_WRAPPER_wrappee_location}.sh)
    ADD_CUSTOM_COMMAND(OUTPUT ${TEST_WRAPPER_output}
      COMMAND sed
          -e "s,%%LIBRARY_PATH%%,${CMAKE_BINARY_DIR}/output/lib,g"
          -e "s,%%WRAPPEE_BASE%%,${_wrappee_base},g"
          -e "s,%%CMAKE_CURRENT_BINARY_DIR%%,${CMAKE_CURRENT_BINARY_DIR},g"
          ${TEST_WRAPPER_template}
          > ${TEST_WRAPPER_output}
      COMMAND chmod +x ${TEST_WRAPPER_output}
      DEPENDS ${TEST_WRAPPER_template})
    IF(${ARGC} GREATER 1)
      ADD_TEST(${_wrappee_base} ${TEST_WRAPPER_output})
    ENDIF(${ARGC} GREATER 1)
  ENDMACRO(TEST_WRAPPER)

  MACRO(JS_TEST_WRAPPER _wrappee_shell _wrappee_js)
    SET(JS_TEST_WRAPPER_template ${CMAKE_SOURCE_DIR}/cmake/js_test_wrapper.sh)
    SET(JS_TEST_WRAPPER_output ${CMAKE_CURRENT_BINARY_DIR}/${_wrappee_js}.sh)
    ADD_CUSTOM_TARGET("${_wrappee_js}_wrap" ALL
      DEPENDS ${JS_TEST_WRAPPER_output})
    GET_TARGET_PROPERTY(SHELL_LOCATION ${_wrappee_shell} LOCATION)
    ADD_CUSTOM_COMMAND(OUTPUT ${JS_TEST_WRAPPER_output}
      COMMAND sed
          -e "s,%%LIBRARY_PATH%%,${CMAKE_BINARY_DIR}/output/lib,g"
          -e "s,%%SHELL_LOCATION%%,${SHELL_LOCATION},g"
          -e "s,%%WRAPPEE_JS%%,${_wrappee_js},g"
          -e "s,%%CMAKE_SOURCE_DIR%%,${CMAKE_SOURCE_DIR},g"
          -e "s,%%CMAKE_CURRENT_SOURCE_DIR%%,${CMAKE_CURRENT_SOURCE_DIR},g"
          -e "s,%%WRAPPEE_SHELL%%,${_wrappee_shell},g"
          -e "s,%%CMAKE_CURRENT_BINARY_DIR%%,${CMAKE_CURRENT_BINARY_DIR},g"
          ${JS_TEST_WRAPPER_template}
          > ${JS_TEST_WRAPPER_output}
      COMMAND chmod +x ${JS_TEST_WRAPPER_output}
      DEPENDS ${JS_TEST_WRAPPER_template})
    IF(${ARGC} GREATER 2)
      ADD_TEST(${_wrappee_js} ${JS_TEST_WRAPPER_output})
    ENDIF(${ARGC} GREATER 2)
  ENDMACRO(JS_TEST_WRAPPER)
ENDIF(WIN32)

MACRO(TEST_RESOURCES)
  FOREACH(TEST_RESOURCES_filename ${ARGN})
    COPY_FILE(${CMAKE_CURRENT_SOURCE_DIR}/${TEST_RESOURCES_filename}
      ${CMAKE_CURRENT_BINARY_DIR})
  ENDFOREACH(TEST_RESOURCES_filename)
ENDMACRO(TEST_RESOURCES)

MACRO(TEST_RESOURCE_DIR _dir_name _target_name)
  COPY_DIR(${CMAKE_CURRENT_SOURCE_DIR}/${_dir_name}
           ${CMAKE_CURRENT_BINARY_DIR}/${_target_name})
ENDMACRO(TEST_RESOURCE_DIR _dir_name)
