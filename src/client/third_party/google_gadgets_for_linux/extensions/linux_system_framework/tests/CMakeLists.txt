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

ADD_DEFINITIONS(-DUNIT_TEST)

APPLY_CONFIG(X11)

MACRO(UNIT_TEST TEST_NAME)
  IF(${ARGC} GREATER 1)
    SET(FILES ${ARGV})
    LIST(REMOVE_AT FILES 0)
    ADD_TEST_EXECUTABLE(${TEST_NAME} ${TEST_NAME}.cc ${FILES})
  ELSE(${ARGC} GREATER 1)
    ADD_TEST_EXECUTABLE(${TEST_NAME} ${TEST_NAME}.cc)
  ENDIF(${ARGC} GREATER 1)
  TARGET_LINK_LIBRARIES(${TEST_NAME}
    gtest ggadget${GGL_EPOCH} ggadget-linux ggadget-dbus${GGL_EPOCH}
    ${X11_LIBRARIES})
  TEST_WRAPPER(${TEST_NAME} TRUE)
ENDMACRO(UNIT_TEST TEST_NAME)

UNIT_TEST(filesystem_test)
UNIT_TEST(filesystem_folder_test)
UNIT_TEST(filesystem_file_test)
UNIT_TEST(filesystem_textstream_test)
UNIT_TEST(memory_test)
UNIT_TEST(perfmon_test ${CMAKE_SOURCE_DIR}/ggadget/tests/native_main_loop.cc)
UNIT_TEST(process_test)

IF(GGL_BUILD_LIBGGADGET_DBUS)
UNIT_TEST(machine_test)
UNIT_TEST(power_test)
ENDIF(GGL_BUILD_LIBGGADGET_DBUS)

IF(GGL_HAVE_NETWORK_MANAGER)
UNIT_TEST(network_test)
ENDIF(GGL_HAVE_NETWORK_MANAGER)
