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

IF(GGL_BUILD_SOUP_XML_HTTP_REQUEST)

APPLY_CONFIG(GTK2)
APPLY_CONFIG(PTHREAD)

ADD_TEST_EXECUTABLE(soup_xml_http_request_test
  soup_xml_http_request_test.cc
)
TARGET_LINK_LIBRARIES(soup_xml_http_request_test
  ${GTK2_LIBRARIES}
  ${PTHREAD_LIBRARIES}
  ggadget${GGL_EPOCH}
  ggadget-gtk${GGL_EPOCH}
  gtest
)
TEST_WRAPPER(soup_xml_http_request_test TRUE)

ENDIF(GGL_BUILD_SOUP_XML_HTTP_REQUEST)
