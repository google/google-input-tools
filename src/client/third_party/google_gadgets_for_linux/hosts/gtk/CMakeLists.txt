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

IF(GGL_BUILD_GTK_HOST)

# Generates ggl-gtk.desktop file
INSTALL_BINARY_DESKTOP(ggl-gtk)

APPLY_CONFIG(CAIRO)
APPLY_CONFIG(GTK2)

LINK_DIRECTORIES(${CMAKE_BINARY_DIR}/output/lib)

ADD_DEFINITIONS(-DGGL_APP_NAME=\\\"ggl-gtk\\\")

SET(SRCS
  main.cc
  gtk_host_base.cc
  sidebar_gtk_host.cc
  simple_gtk_host.cc
  standalone_gtk_host.cc
)

ADD_EXECUTABLE(ggl-gtk ${SRCS})
OUTPUT_EXECUTABLE(ggl-gtk)

TARGET_LINK_LIBRARIES(ggl-gtk
  ggadget${GGL_EPOCH}
  ggadget-gtk${GGL_EPOCH}
  ${CAIRO_LIBRARIES}
  ${GTK2_LIBRARIES}
)

INSTALL( TARGETS ggl-gtk
    RUNTIME DESTINATION ${BIN_INSTALL_DIR}
    LIBRARY DESTINATION ${LIB_INSTALL_DIR}
    ARCHIVE DESTINATION ${LIB_INSTALL_DIR}
)
ENDIF(GGL_BUILD_GTK_HOST)
