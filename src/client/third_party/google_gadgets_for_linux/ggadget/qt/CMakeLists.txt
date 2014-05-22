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

IF(GGL_BUILD_LIBGGADGET_QT)

APPLY_CONFIG(X11)
APPLY_CONFIG(QT)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-conversion")

SET(CMAKE_INCLUDE_CURRENT_DIR ON)
SET(SRCS
  qt_canvas.cc
  qt_font.cc
  qt_view_widget.cc
  qt_graphics.cc
  qt_image.cc
  qt_main_loop.cc
  qt_menu.cc
  qt_view_host.cc
  utilities.cc
)

QT4_AUTOMOC(
  qt_menu.cc
  qt_main_loop.cc
  qt_view_widget.cc
  qt_view_host.cc
  utilities.cc
)

ADD_SUBDIRECTORY(tests)

ADD_LIBRARY(ggadget-qt${GGL_EPOCH} SHARED ${SRCS})
TARGET_LINK_LIBRARIES(ggadget-qt${GGL_EPOCH}
  ggadget-xdg${GGL_EPOCH}
  ${X11_LIBRARIES}
  ${QT_LIBRARIES}
)

OUTPUT_LIBRARY(ggadget-qt${GGL_EPOCH})

INSTALL(FILES
  qt_view_widget.h
  qt_graphics.h
  qt_main_loop.h
  qt_menu.h
  qt_view_host.h
  utilities.h
  DESTINATION ${GGL_INCLUDE_DIR}/ggadget/qt COMPONENT Devel)

INSTALL_PKG_CONFIG(libggadget-qt)
ENDIF(GGL_BUILD_LIBGGADGET_QT)
