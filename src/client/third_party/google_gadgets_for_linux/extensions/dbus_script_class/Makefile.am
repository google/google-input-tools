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

if GGL_BUILD_DBUS_SCRIPT_CLASS

INCLUDES		= -I$(top_builddir) \
			  -I$(top_srcdir)

noinst_HEADERS		= scriptable_dbus_object.h

extension_LTLIBRARIES	= dbus-script-class.la
extensiondir		= $(GGL_MODULE_DIR)

dbus_script_class_la_SOURCES = \
			  dbus_script_class.cc \
			  scriptable_dbus_object.cc

dbus_script_class_la_CXXFLAGS = \
			  $(DEFAULT_COMPILE_FLAGS)

dbus_script_class_la_CPPFLAGS = \
			  $(DBUS_CFLAGS) \
			  $(PREDEFINED_MACROS)

dbus_script_class_la_LIBADD = \
			  $(DBUS_LIBS) \
			  $(top_builddir)/ggadget/libggadget@GGL_EPOCH@.la \
			  $(top_builddir)/ggadget/dbus/libggadget-dbus@GGL_EPOCH@.la

dbus_script_class_la_LDFLAGS = \
			  -no-undefined \
			  -avoid-version \
			  -module
endif
