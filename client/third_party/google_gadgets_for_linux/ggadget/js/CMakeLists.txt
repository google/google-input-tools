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

ADD_SUBDIRECTORY(tests)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-conversion")

ADD_CUSTOM_COMMAND(OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/jscript_massager.cc
  COMMAND flex -o jscript_massager.cc jscript_massager.l
  MAIN_DEPENDENCY jscript_massager.l
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

ADD_LIBRARY(ggadget-js${GGL_EPOCH} SHARED jscript_massager.cc js_utils.cc)
TARGET_LINK_LIBRARIES(ggadget-js${GGL_EPOCH} ggadget${GGL_EPOCH})
OUTPUT_LIBRARY(ggadget-js${GGL_EPOCH})

INSTALL(FILES
  jscript_massager.h
  js_utils.h
  DESTINATION ${GGL_INCLUDE_DIR}/ggadget/js COMPONENT Devel)

INSTALL_PKG_CONFIG(libggadget-js)
