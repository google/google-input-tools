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

EXTRA_DIST = CMakeLists.txt

if GGL_BUILD_LIBGGADGET_DBUS

# Putting . into SBUDIRS list to force building current directory first.
SUBDIRS = . tests

INCLUDES		= -I$(top_builddir) \
			  -I$(top_srcdir)

noinst_HEADERS		= dbus_utils.h

dbusincludedir		= $(GGL_INCLUDE_DIR)/ggadget/dbus
dbusinclude_HEADERS	= dbus_proxy.h \
			  dbus_result_receiver.h

lib_LTLIBRARIES		= libggadget-dbus@GGL_EPOCH@.la

libggadget_dbus@GGL_EPOCH@_la_SOURCES = \
			  dbus_utils.cc \
			  dbus_proxy.cc

libggadget_dbus@GGL_EPOCH@_la_CXXFLAGS = \
			  $(DEFAULT_COMPILE_FLAGS)

libggadget_dbus@GGL_EPOCH@_la_CPPFLAGS = \
			  $(DBUS_CFLAGS) \
			  $(PREDEFINED_MACROS)

libggadget_dbus@GGL_EPOCH@_la_LIBADD = \
			  $(DBUS_LIBS) \
			  $(top_builddir)/ggadget/libggadget@GGL_EPOCH@.la

libggadget_dbus@GGL_EPOCH@_la_LDFLAGS = \
			  -version-info $(LIBGGADGET_VERSION) \
			  -no-undefined \
			  -export-dynamic

pkgconfigdir= $(libdir)/pkgconfig
pkgconfig_DATA= libggadget-dbus@GGL_EPOCH@.pc
endif
