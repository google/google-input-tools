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

IF(GGL_BUILD_GTKMOZ_BROWSER_ELEMENT)

APPLY_CONFIG(GTK2)
APPLY_CONFIG(GTHREAD)
APPLY_CONFIG(SMJS)
APPLY_CONFIG(GTKMOZEMBED)

SET(LIB_SRCS browser_element.cc)

ADD_MODULE(gtkmoz-browser-element ${LIB_SRCS})
TARGET_LINK_LIBRARIES(gtkmoz-browser-element
  ggadget${GGL_EPOCH}
  ${GTK2_LIBRARIES}
)
OUTPUT_MODULE(gtkmoz-browser-element)

SET(CMAKE_CXX_FLAGS
  "${CMAKE_CXX_FLAGS} -Wno-non-virtual-dtor -Wno-conversion -fno-strict-aliasing")
STRING(REPLACE "-Wextra" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

IF(NOT GTKMOZEMBED_FOUND)
  MESSAGE(FATAL "gtkmozembed library not found!")
ENDIF(NOT GTKMOZEMBED_FOUND)

SET(CHILD_SRCS
  browser_child.cc
  browser_child_internal.cc
  ${CMAKE_SOURCE_DIR}/extensions/smjs_script_runtime/libmozjs_glue.cc
)
ADD_EXECUTABLE(gtkmoz-browser-child ${CHILD_SRCS})
TARGET_LINK_LIBRARIES(gtkmoz-browser-child
  ${SMJS_LIBRARIES}
  ${GTKMOZEMBED_LIBRARIES}
  ${GTK2_LIBRARIES}
  ${GTHREAD_LIBRARIES}
  ggadget${GGL_EPOCH}
  dl
)
OUTPUT_EXECUTABLE(gtkmoz-browser-child)
INSTALL(TARGETS gtkmoz-browser-child
  DESTINATION "${LIB_INSTALL_DIR}/${PRODUCT_NAME}/")

ENDIF(GGL_BUILD_GTKMOZ_BROWSER_ELEMENT)
