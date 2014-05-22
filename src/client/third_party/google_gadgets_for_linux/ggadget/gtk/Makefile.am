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

EXTRA_DIST = CMakeLists.txt

if GGL_BUILD_LIBGGADGET_GTK

# Putting . into SBUDIRS list to force building current directory first.
SUBDIRS = . tests

INCLUDES		= -I$(top_builddir) \
			  -I$(top_srcdir)

noinst_HEADERS		= cairo_canvas.h \
			  cairo_font.h \
			  cairo_image_base.h \
			  pixbuf_image.h

gtkincludedir		= $(GGL_INCLUDE_DIR)/ggadget/gtk
gtkinclude_HEADERS	= cairo_graphics.h \
			  hotkey.h \
			  key_convert.h \
			  main_loop.h \
			  menu_builder.h \
			  single_view_host.h \
			  tooltip.h \
			  utilities.h \
			  view_widget_binder.h

lib_LTLIBRARIES		= libggadget-gtk@GGL_EPOCH@.la

libggadget_gtk@GGL_EPOCH@_la_SOURCES = \
			  cairo_canvas.cc \
			  cairo_font.cc \
			  cairo_graphics.cc \
			  cairo_image_base.cc \
			  hotkey.cc \
			  key_convert.cc \
			  main_loop.cc \
			  menu_builder.cc \
			  pixbuf_image.cc \
			  single_view_host.cc \
			  tooltip.cc \
			  utilities.cc \
			  view_widget_binder.cc

libggadget_gtk@GGL_EPOCH@_la_CXXFLAGS = \
			  $(DEFAULT_COMPILE_FLAGS) \
			  -fno-strict-aliasing

libggadget_gtk@GGL_EPOCH@_la_CPPFLAGS = \
			  $(GTK2_CFLAGS) \
			  $(PREDEFINED_MACROS)

libggadget_gtk@GGL_EPOCH@_la_LIBADD = \
			  $(GTK2_LIBS) \
			  $(top_builddir)/ggadget/libggadget@GGL_EPOCH@.la \
			  $(top_builddir)/ggadget/xdg/libggadget-xdg@GGL_EPOCH@.la

libggadget_gtk@GGL_EPOCH@_la_LDFLAGS = \
			  -version-info $(LIBGGADGET_VERSION) \
			  -no-undefined \
			  -export-dynamic

if GTK2_TARGET_X11
  libggadget_gtk@GGL_EPOCH@_la_CPPFLAGS += $(X_CFLAGS) $(STARTUP_NOTIFICATION_CFLAGS)
  libggadget_gtk@GGL_EPOCH@_la_LIBADD += $(X_LIBS) $(STARTUP_NOTIFICATION_LIBS)
endif

if GGL_HAVE_RSVG
  noinst_HEADERS += rsvg_image.h
  libggadget_gtk@GGL_EPOCH@_la_SOURCES += rsvg_image.cc
  libggadget_gtk@GGL_EPOCH@_la_CPPFLAGS += $(RSVG_CFLAGS)
  libggadget_gtk@GGL_EPOCH@_la_LIBADD += $(RSVG_LIBS)
endif

if GGL_BUILD_LIBGGADGET_NPAPI
  gtkinclude_HEADERS += npapi_plugin_element.h
  libggadget_gtk@GGL_EPOCH@_la_SOURCES += npapi_plugin_element.cc
  libggadget_gtk@GGL_EPOCH@_la_LIBADD += $(top_builddir)/ggadget/npapi/libggadget-npapi@GGL_EPOCH@.la
endif

pkgconfigdir= $(libdir)/pkgconfig
pkgconfig_DATA= libggadget-gtk@GGL_EPOCH@.pc
endif
