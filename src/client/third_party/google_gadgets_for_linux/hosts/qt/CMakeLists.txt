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

IF(GGL_BUILD_QT_HOST)

# Generates ggl-qt.desktop file
INSTALL_BINARY_DESKTOP(ggl-qt)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-conversion")

APPLY_CONFIG(QT)
SET(CMAKE_INCLUDE_CURRENT_DIR ON)
ADD_DEFINITIONS(-DGGL_APP_NAME=\\\"ggl-qt\\\")

LINK_DIRECTORIES(${CMAKE_BINARY_DIR}/output/lib)

SET(SRCS
  main.cc
  qt_host.cc
)
QT4_AUTOMOC(qt_host.cc)

ADD_EXECUTABLE(ggl-qt ${SRCS})
OUTPUT_EXECUTABLE(ggl-qt)

TARGET_LINK_LIBRARIES(ggl-qt
  ggadget${GGL_EPOCH}
  ggadget-qt${GGL_EPOCH}
  ${QT_LIBRARIES}
)

INSTALL( TARGETS ggl-qt
    RUNTIME DESTINATION ${BIN_INSTALL_DIR}
    LIBRARY DESTINATION ${LIB_INSTALL_DIR}
    ARCHIVE DESTINATION ${LIB_INSTALL_DIR}
)

ENDIF(GGL_BUILD_QT_HOST)
