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

# libggadget-dbus is mandatory for linux-system-framework.
if GGL_HOST_LINUX

# Putting . into SBUDIRS list to force building current directory first.
SUBDIRS = . tests

INCLUDES		= -I$(top_builddir) \
			  -I$(top_srcdir)

noinst_HEADERS		= file_system.h \
			  runtime.h \
			  machine.h \
			  memory.h \
			  network.h \
			  perfmon.h \
			  power.h \
			  process.h \
			  user.h \
			  wireless.h \
			  hal_strings.h

noinst_LTLIBRARIES	= libggadget-linux.la

libggadget_linux_la_SOURCES = \
			  file_system.cc \
			  runtime.cc \
			  memory.cc \
			  perfmon.cc \
			  process.cc

if GGL_BUILD_LIBGGADGET_DBUS
libggadget_linux_la_SOURCES += \
			  machine.cc \
			  power.cc \
			  user.cc

if GGL_HAVE_NETWORK_MANAGER
libggadget_linux_la_SOURCES += \
			  network.cc \
			  wireless.cc
endif
endif

libggadget_linux_la_CXXFLAGS = \
			  $(DEFAULT_COMPILE_FLAGS)

libggadget_linux_la_CPPFLAGS = \
			  $(X_CFLAGS) \
			  $(PREDEFINED_MACROS)

libggadget_linux_la_LIBADD = \
			  $(top_builddir)/ggadget/libggadget@GGL_EPOCH@.la \
			  $(top_builddir)/ggadget/xdg/libggadget-xdg@GGL_EPOCH@.la \
			  $(X_LIBS)

if GGL_BUILD_LIBGGADGET_DBUS
libggadget_linux_la_CPPFLAGS += $(DBUS_CFLAGS)
if GGL_HAVE_NETWORK_MANAGER
libggadget_linux_la_CPPFLAGS += $(NETWORK_MANAGER_CFLAGS)
endif
libggadget_linux_la_LIBADD += \
			  $(top_builddir)/ggadget/dbus/libggadget-dbus@GGL_EPOCH@.la \
			  $(DBUS_LIB)
endif

extension_LTLIBRARIES	= linux-system-framework.la
extensiondir		= $(GGL_MODULE_DIR)

linux_system_framework_la_SOURCES = linux_system_framework.cc

linux_system_framework_la_CXXFLAGS = \
			  $(DEFAULT_COMPILE_FLAGS)

linux_system_framework_la_CPPFLAGS = \
			  $(PREDEFINED_MACROS)

linux_system_framework_la_LIBADD = \
			  $(top_builddir)/ggadget/libggadget@GGL_EPOCH@.la \
			  libggadget-linux.la

if GGL_BUILD_LIBGGADGET_DBUS
linux_system_framework_la_LIBADD += \
			  $(top_builddir)/ggadget/dbus/libggadget-dbus@GGL_EPOCH@.la \
			  $(DBUS_LIB)
endif

linux_system_framework_la_LDFLAGS = \
			  -no-undefined \
			  -avoid-version \
			  -module

endif
