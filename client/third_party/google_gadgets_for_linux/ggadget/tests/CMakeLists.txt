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
APPLY_CONFIG(LIBXML2)

MACRO(UNIT_TEST TEST_NAME)
  IF(${ARGC} GREATER 1)
    SET(FILES ${ARGV})
    LIST(REMOVE_AT FILES 0)
    ADD_TEST_EXECUTABLE(${TEST_NAME} ${TEST_NAME}.cc ${FILES})
  ELSE(${ARGC} GREATER 1)
    ADD_TEST_EXECUTABLE(${TEST_NAME} ${TEST_NAME}.cc)
  ENDIF(${ARGC} GREATER 1)
  TARGET_LINK_LIBRARIES(${TEST_NAME} gtest ggadget${GGL_EPOCH} ${PTHREAD_LIBRARIES} ${LIBXML2_LIBRARIES})
  TEST_WRAPPER(${TEST_NAME} TRUE)
ENDMACRO(UNIT_TEST TEST_NAME)

ADD_SUBDIRECTORY(test_modules)

TEST_RESOURCE_DIR(file_manager_test_data file_manager_test_data_dest)
ADD_DIR_TO_ZIP(file_manager_test_data file_manager_test_data_dest.gg)

UNIT_TEST(backoff_test)
UNIT_TEST(basic_element_test)
UNIT_TEST(color_test)
UNIT_TEST(common_test)
UNIT_TEST(digest_utils_test)
UNIT_TEST(elements_test)
UNIT_TEST(element_factory_test)
UNIT_TEST(encryptor_test)
UNIT_TEST(extension_manager_test)
UNIT_TEST(file_manager_test)
UNIT_TEST(image_cache_test)
UNIT_TEST(locales_test)
UNIT_TEST(math_utils_test)
UNIT_TEST(messages_test)
UNIT_TEST(module_test)
UNIT_TEST(native_main_loop_test native_main_loop.cc)
UNIT_TEST(scriptable_helper_test scriptables.cc)
UNIT_TEST(scriptable_enumerator_test scriptables.cc)
UNIT_TEST(signal_test slots.cc)
UNIT_TEST(slot_test slots.cc)
UNIT_TEST(string_utils_test)
UNIT_TEST(system_utils_test)
UNIT_TEST(text_formats_test)
UNIT_TEST(unicode_utils_test)
UNIT_TEST(uuid_test)
UNIT_TEST(variant_test)
UNIT_TEST(view_test)
UNIT_TEST(xml_dom_test)
UNIT_TEST(xml_parser_test)
UNIT_TEST(xml_http_request_test native_main_loop.cc)
