#
#  Copyright 2007 Google Inc.
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

IF(GGL_BUILD_QT_SCRIPT_RUNTIME)

APPLY_CONFIG(QT)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-conversion")

SET(CMAKE_INCLUDE_CURRENT_DIR ON)
ADD_SUBDIRECTORY(tests)

SET(SRCS
  js_script_context.cc
  js_script_runtime.cc
  js_function_slot.cc
  js_native_wrapper.cc
  converter.cc
  json.cc
)

QT4_AUTOMOC(js_function_slot.cc)

ADD_LIBRARY(ggadget-qtscript STATIC ${SRCS})

ADD_MODULE(qt-script-runtime qt_script_runtime.cc)
TARGET_LINK_LIBRARIES(qt-script-runtime
  ggadget${GGL_EPOCH}
  ggadget-qtscript
  ggadget-js${GGL_EPOCH}
  ${QT_LIBRARIES}
  ${QT_QTSCRIPT_LIBRARIES}
)
OUTPUT_MODULE(qt-script-runtime)

ENDIF(GGL_BUILD_QT_SCRIPT_RUNTIME)
