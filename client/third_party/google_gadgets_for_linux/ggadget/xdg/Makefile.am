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

SUBDIRS 		= tests

INCLUDES		= -I$(top_builddir) \
			  -I$(top_srcdir)

noinst_HEADERS		= icon_theme.h
xdgincludedir		= $(GGL_INCLUDE_DIR)/ggadget/xdg
xdginclude_HEADERS	= desktop_entry.h \
			  utilities.h

lib_LTLIBRARIES		= libggadget-xdg@GGL_EPOCH@.la

libggadget_xdg@GGL_EPOCH@_la_SOURCES = \
			  desktop_entry.cc \
			  icon_theme.cc \
			  utilities.cc

libggadget_xdg@GGL_EPOCH@_la_CPPFLAGS = \
			  $(PREDEFINED_MACROS) \
			  -DXDG_PREFIX=_ggl_xdg

libggadget_xdg@GGL_EPOCH@_la_CXXFLAGS = \
			  $(DEFAULT_COMPILE_FLAGS)

libggadget_xdg@GGL_EPOCH@_la_LIBADD = \
			  $(top_builddir)/ggadget/libggadget@GGL_EPOCH@.la

if GGL_ENABLE_XDGMIME
libggadget_xdg@GGL_EPOCH@_la_LIBADD += \
			  $(top_builddir)/third_party/xdgmime/libxdgmime.la
endif

libggadget_xdg@GGL_EPOCH@_la_LDFLAGS = \
			  -version-info $(LIBGGADGET_VERSION) \
			  -no-undefined \
			  -export-dynamic

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libggadget-xdg@GGL_EPOCH@.pc
