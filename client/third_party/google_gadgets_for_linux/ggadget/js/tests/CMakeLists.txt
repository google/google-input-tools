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

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-conversion")

SET(LIBS
  gtest
  ggadget${GGL_EPOCH}
  editline
)

ADD_TEST_EXECUTABLE(massager massager.cc ../jscript_massager.cc)
TARGET_LINK_LIBRARIES(massager ${LIBS})
TEST_WRAPPER(massager)

ADD_TEST_EXECUTABLE(jscript_massager_test
  jscript_massager_test.cc
  ../jscript_massager.cc
  )
TARGET_LINK_LIBRARIES(jscript_massager_test ${LIBS})
TEST_WRAPPER(jscript_massager_test TRUE)

