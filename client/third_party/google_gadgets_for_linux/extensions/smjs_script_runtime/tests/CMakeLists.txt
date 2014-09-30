#
#  Copyright 2008 Google Inc.
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
#

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcheck-new")

SET(LIBS
  gtest
  ggadget${GGL_EPOCH}
  ggadget-js${GGL_EPOCH}
  editline
  ggadget-smjs
  dl
  ${SMJS_LIBRARIES}
)

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/third_party/editline)

# General js_shell without custom objects.
ADD_TEST_EXECUTABLE(js_shell
  js_shell.cc
  init_custom_objects_stub.cc
)
TARGET_LINK_LIBRARIES(js_shell ${LIBS})
TEST_WRAPPER(js_shell)

# Customized js_shell with test scriptables.
ADD_TEST_EXECUTABLE(wrapper_test_shell
  js_shell.cc
  wrapper_test_shell.cc
  ${CMAKE_SOURCE_DIR}/ggadget/tests/scriptables.cc
)
TARGET_LINK_LIBRARIES(wrapper_test_shell ${LIBS})
TEST_WRAPPER(wrapper_test_shell)

STRING(REPLACE "-Wall" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
STRING(REPLACE "-Wall" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
STRING(REPLACE "-Werror" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
STRING(REPLACE "-Werror" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

ADD_TEST_EXECUTABLE(cross_context_test
  cross_context_test.cc)
TARGET_LINK_LIBRARIES(cross_context_test ${LIBS})
TEST_WRAPPER(cross_context_test TRUE)

# Customized js_shell for testing DOM.
ADD_TEST_EXECUTABLE(dom_test_shell
  js_shell.cc
  dom_test_shell.cc
)
TARGET_LINK_LIBRARIES(dom_test_shell ${LIBS})
TEST_WRAPPER(dom_test_shell)

JS_TEST_WRAPPER(js_shell json_test.js TRUE)
JS_TEST_WRAPPER(wrapper_test_shell wrapper_test.js TRUE)
JS_TEST_WRAPPER(dom_test_shell dom1_test.js TRUE)

IF(GGL_BUILD_LIBGGADGET_DBUS)
ADD_TEST_EXECUTABLE(dbus_object_test_shell
  js_shell.cc
  dbus_object_test_shell.cc
  ${CMAKE_SOURCE_DIR}/ggadget/tests/native_main_loop.cc)
TARGET_LINK_LIBRARIES(dbus_object_test_shell ${LIBS})
JS_TEST_WRAPPER(dbus_object_test_shell dbus_object_test.js TRUE)
ENDIF(GGL_BUILD_LIBGGADGET_DBUS)
