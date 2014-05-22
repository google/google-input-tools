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

IF(GGL_BUILD_LIBGGADGET_GTK)

APPLY_CONFIG(CAIRO)
APPLY_CONFIG(FONTCONFIG)
APPLY_CONFIG(GTK2)
APPLY_CONFIG(RSVG)

SET(SRCS
  cairo_canvas.cc
  cairo_font.cc
  cairo_graphics.cc
  cairo_image_base.cc
  hotkey.cc
  key_convert.cc
  main_loop.cc
  menu_builder.cc
  pixbuf_image.cc
  single_view_host.cc
  tooltip.cc
  utilities.cc
  view_widget_binder.cc
)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-strict-aliasing")

IF(GGL_BUILD_LIBGGADGET_NPAPI)
  SET(SRCS ${SRCS} npapi_plugin_element.cc)
ENDIF(GGL_BUILD_LIBGGADGET_NPAPI)

if(RSVG_FOUND)
  SET(SRCS ${SRCS} rsvg_image.cc)
endif(RSVG_FOUND)

ADD_SUBDIRECTORY(tests)

ADD_LIBRARY(ggadget-gtk${GGL_EPOCH} SHARED ${SRCS})
TARGET_LINK_LIBRARIES(ggadget-gtk${GGL_EPOCH}
  ggadget-xdg${GGL_EPOCH}
  ${CAIRO_LIBRARIES}
  ${GTK2_LIBRARIES}
  ${GTHREAD_LIBRARIES}
  ${PANGO_LIBRARIES}
  ${FONTCONFIG_LIBRARIES}
  ${RSVG_LIBRARIES}
)

IF(GGL_BUILD_LIBGGADGET_NPAPI)
  TARGET_LINK_LIBRARIES(ggadget-gtk${GGL_EPOCH}
    ggadget-npapi${GGL_EPOCH})
ENDIF(GGL_BUILD_LIBGGADGET_NPAPI)

OUTPUT_LIBRARY(ggadget-gtk${GGL_EPOCH})

INSTALL(FILES
  cairo_graphics.h
  hotkey.h
  key_convert.h
  main_loop.h
  menu_builder.h
  single_view_host.h
  tooltip.h
  utilities.h
  view_widget_binder.h
  DESTINATION ${GGL_INCLUDE_DIR}/ggadget/gtk COMPONENT Devel)

IF(GGL_BUILD_LIBGGADGET_NPAPI)
  INSTALL(FILES
    npapi_plugin_element.h
    DESTINATION ${GGL_INCLUDE_DIR}/ggadget/gtk COMPONENT Devel)
ENDIF(GGL_BUILD_LIBGGADGET_NPAPI)

INSTALL_PKG_CONFIG(libggadget-gtk)

ENDIF(GGL_BUILD_LIBGGADGET_GTK)
