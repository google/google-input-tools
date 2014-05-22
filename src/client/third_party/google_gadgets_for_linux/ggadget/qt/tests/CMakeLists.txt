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

MACRO(UNIT_TEST TEST_NAME)
  ADD_TEST_EXECUTABLE(${TEST_NAME} ${TEST_NAME}.cc)
  TARGET_LINK_LIBRARIES(${TEST_NAME}
    gtest
    ggadget${GGL_EPOCH}
    ggadget-qt${GGL_EPOCH})
  TEST_WRAPPER(${TEST_NAME} TRUE)
ENDMACRO(UNIT_TEST TEST_NAME)

UNIT_TEST(main_loop_test)

