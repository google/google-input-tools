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

IF(GGL_BUILD_WEBKIT_SCRIPT_RUNTIME)

APPLY_CONFIG(JAVA_SCRIPT_CORE)

ADD_SUBDIRECTORY(tests)

SET(SRCS
  converter.cc
  js_script_context.cc
  js_script_runtime.cc
  json.cc
)

ADD_LIBRARY(ggadget-webkitjs${GGL_EPOCH} SHARED ${SRCS})
OUTPUT_LIBRARY(ggadget-webkitjs${GGL_EPOCH})

ADD_MODULE(webkit-script-runtime webkit_script_runtime.cc)
TARGET_LINK_LIBRARIES(webkit-script-runtime
  ggadget-webkitjs${GGL_EPOCH}
  ggadget${GGL_EPOCH}
  ggadget-js${GGL_EPOCH}
  ${JAVA_SCRIPT_CORE_LIBRARIES}
  dl
)
OUTPUT_MODULE(webkit-script-runtime)

ENDIF(GGL_BUILD_WEBKIT_SCRIPT_RUNTIME)
