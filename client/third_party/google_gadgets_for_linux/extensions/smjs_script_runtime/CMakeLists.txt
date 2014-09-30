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

IF(GGL_BUILD_SMJS_SCRIPT_RUNTIME)

APPLY_CONFIG(SMJS)

ADD_SUBDIRECTORY(tests)

# Added -fcheck-new to disable the warning about operator new returning NULL
# in jscntxt.h.
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcheck-new")
STRING(REPLACE "-Wextra" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

SET(SRCS
  converter.cc
  js_function_slot.cc
  js_native_wrapper.cc
  js_script_context.cc
  js_script_runtime.cc
  json.cc
  native_js_wrapper.cc
  libmozjs_glue.cc
)

ADD_LIBRARY(ggadget-smjs STATIC ${SRCS})

ADD_MODULE(smjs-script-runtime smjs_script_runtime.cc)
TARGET_LINK_LIBRARIES(smjs-script-runtime
  ggadget-smjs
  ggadget${GGL_EPOCH}
  ggadget-js${GGL_EPOCH}
  ${SMJS_LIBRARIES}
  dl
)
OUTPUT_MODULE(smjs-script-runtime)

ENDIF(GGL_BUILD_SMJS_SCRIPT_RUNTIME)
