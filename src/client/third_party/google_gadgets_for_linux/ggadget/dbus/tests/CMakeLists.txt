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

APPLY_CONFIG(PTHREAD)

SET(LIBS
  ${DBUS_LIBRARIES}
  ${PTHREAD_LIBRARIES}
  gtest
  ggadget${GGL_EPOCH}
  ggadget-dbus${GGL_EPOCH}
)

ADD_TEST_EXECUTABLE(dbus_test dbus_test.cc ../../tests/native_main_loop.cc)
TARGET_LINK_LIBRARIES(dbus_test ${LIBS})
TEST_WRAPPER(dbus_test TRUE)

ADD_TEST_EXECUTABLE(dbus_marshaller_test dbus_marshaller_test.cc)
TARGET_LINK_LIBRARIES(dbus_marshaller_test ${LIBS})
TEST_WRAPPER(dbus_marshaller_test TRUE)

ADD_TEST_EXECUTABLE(dbus_result_receiver_test dbus_result_receiver_test.cc)
TARGET_LINK_LIBRARIES(dbus_result_receiver_test ${LIBS})
TEST_WRAPPER(dbus_result_receiver_test TRUE)
