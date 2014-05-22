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

IF(GGL_BUILD_LIBXML2_XML_PARSER)

APPLY_CONFIG(LIBXML2)
ADD_MODULE(libxml2-xml-parser libxml2_xml_parser.cc)
TARGET_LINK_LIBRARIES(libxml2-xml-parser ${LIBXML2_LIBRARIES} ggadget${GGL_EPOCH})
OUTPUT_MODULE(libxml2-xml-parser)

ENDIF(GGL_BUILD_LIBXML2_XML_PARSER)
