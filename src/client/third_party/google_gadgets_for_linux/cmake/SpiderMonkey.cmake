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

MACRO(TRY_COMPILE_SMJS _test_program _definitions _link_flags)
  TRY_COMPILE(TRY_COMPILE_SMJS_RESULT
    ${CMAKE_BINARY_DIR}/configure
    ${CMAKE_SOURCE_DIR}/cmake/${_test_program}
    CMAKE_FLAGS
      -DINCLUDE_DIRECTORIES:STRING=${SMJS_INCLUDE_DIR}
      -DLINK_DIRECTORIES:STRING=${SMJS_LINK_DIR}
      -DCMAKE_EXE_LINKER_FLAGS:STRING=${_link_flags}
      -DLINK_LIBRARIES:STRING=${SMJS_LIBRARIES}
    COMPILE_DEFINITIONS ${_definitions}
    OUTPUT_VARIABLE TRY_COMPILE_SMJS_OUTPUT)
  # MESSAGE("'${TRY_COMPILE_SMJS_RESULT}' '${TRY_COMPILE_SMJS_RESULT}'")
ENDMACRO(TRY_COMPILE_SMJS _test_program _definitions _link_flags)

MACRO(TRY_RUN_SMJS _test_program _definitions _link_flags)
  TRY_RUN(TRY_RUN_SMJS_RESULT TRY_RUN_SMJS_COMPILE_RESULT
    ${CMAKE_BINARY_DIR}/configure
    ${CMAKE_SOURCE_DIR}/cmake/${_test_program}
    CMAKE_FLAGS
      -DINCLUDE_DIRECTORIES:STRING=${SMJS_INCLUDE_DIR}
      -DLINK_DIRECTORIES:STRING=${SMJS_LINK_DIR}
      -DCMAKE_EXE_LINKER_FLAGS:STRING=${_link_flags}
      -DLINK_LIBRARIES:STRING=${SMJS_LIBRARIES}
    COMPILE_DEFINITIONS ${_definitions}
    OUTPUT_VARIABLE TRY_RUN_SMJS_OUTPUT)
  # MESSAGE("'${TRY_RUN_SMJS_COMPILE_RESULT}' '${TRY_RUN_SMJS_RESULT}'" '${TRY_RUN_SMJS_OUTPUT}')
ENDMACRO(TRY_RUN_SMJS _test_program _definitions _link_flags)

SET(SMJS_LIST
  mozilla-js
  xulrunner-js
  firefox2-js
  firefox-js
  seamonkey-js
  )
FOREACH(js ${SMJS_LIST})
  IF(NOT SMJS_FOUND)
    GET_CONFIG(${js} 0 SMJS SMJS_FOUND)
    IF(SMJS_FOUND)
      SET(SMJS_NAME ${js})
    ENDIF(SMJS_FOUND)
  ENDIF(NOT SMJS_FOUND)
ENDFOREACH(js ${SMJS_LIST})

IF(SMJS_FOUND)
  IF(UNIX)
    SET(SMJS_DEFINITIONS "${SMJS_DEFINITIONS} -DXP_UNIX")
  ENDIF(UNIX)

  TRY_COMPILE_SMJS(
    test_js_threadsafe.c
    "${SMJS_DEFINITIONS} -DMIN_SMJS_VERSION=160 -DJS_THREADSAFE"
    "${CMAKE_EXE_LINKER_FLAGS} ${SMJS_LINKER_FLAGS}")

  IF(TRY_COMPILE_SMJS_RESULT)
    SET(SMJS_DEFINITIONS "${SMJS_DEFINITIONS} -DJS_THREADSAFE")
  ELSE(TRY_COMIPLE_SMJS_RESULT)
    TRY_COMPILE_SMJS(
      test_js_threadsafe.c
      "${SMJS_DEFINITIONS} -DMIN_SMJS_VERSION=160"
      "${CMAKE_EXE_LINKER_FLAGS} ${SMJS_LINKER_FLAGS}")
    IF(NOT TRY_COMPILE_SMJS_RESULT)
      SET(SMJS_FOUND 0)
      MESSAGE("Failed to try run SpiderMonkey, the library version may be too low.")
    ENDIF(NOT TRY_COMPILE_SMJS_RESULT)
  ENDIF(TRY_COMPILE_SMJS_RESULT)
ENDIF(SMJS_FOUND)

IF(SMJS_FOUND)
  TRY_RUN_SMJS(
    test_js_mozilla_1_8_branch.c
    "${SMJS_DEFINITIONS} -DMOZILLA_1_8_BRANCH"
    "${CMAKE_EXE_LINKER_FLAGS} ${SMJS_LINKER_FLAGS}")

  IF(TRY_RUN_SMJS_COMPILE_RESULT AND "${TRY_RUN_SMJS_RESULT}" STREQUAL "0")
    SET(SMJS_DEFINITIONS "${SMJS_DEFINITIONS} -DMOZILLA_1_8_BRANCH")
  ENDIF(TRY_RUN_SMJS_COMPILE_RESULT AND "${TRY_RUN_SMJS_RESULT}" STREQUAL "0")
ENDIF(SMJS_FOUND)
