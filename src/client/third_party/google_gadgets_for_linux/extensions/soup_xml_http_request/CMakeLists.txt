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

IF(GGL_BUILD_SOUP_XML_HTTP_REQUEST)
ADD_SUBDIRECTORY(tests)

APPLY_CONFIG(LIBSOUP)
APPLY_CONFIG(LIBSOUP_GNOME)
ADD_MODULE(soup-xml-http-request soup_xml_http_request.cc)
TARGET_LINK_LIBRARIES(soup-xml-http-request
  ${LIBSOUP_LIBRARIES}
  ${LIBSOUP_GNOME_LIBRARIES}
  ggadget${GGL_EPOCH})
OUTPUT_MODULE(soup-xml-http-request)

ENDIF(GGL_BUILD_SOUP_XML_HTTP_REQUEST)
