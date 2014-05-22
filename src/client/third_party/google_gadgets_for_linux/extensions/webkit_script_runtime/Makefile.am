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

# EXTRA_DIST = CMakeLists.txt

if GGL_BUILD_WEBKIT_SCRIPT_RUNTIME
SUBDIRS = . tests

INCLUDES = -I$(top_builddir) \
				   -I$(top_srcdir)

noinst_HEADERS = \
				converter.h \
				java_script_core.h \
				js_script_context.h \
				js_script_runtime.h \
				json.h

lib_LTLIBRARIES = libggadget-webkitjs@GGL_EPOCH@.la

libggadget_webkitjs@GGL_EPOCH@_la_SOURCES = \
				converter.cc \
				js_script_context.cc \
				js_script_runtime.cc \
				json.cc

libggadget_webkitjs@GGL_EPOCH@_la_CXXFLAGS = \
				$(DEFAULT_COMPILE_FLAGS)

libggadget_webkitjs@GGL_EPOCH@_la_CPPFLAGS = \
				$(JAVA_SCRIPT_CORE_CFLAGS) \
				$(PREDEFINED_MACROS)

libggadget_webkitjs@GGL_EPOCH@_la_LIBADD = \
				$(JAVA_SCRIPT_CORE_LIBS) \
				$(top_builddir)/ggadget/libggadget@GGL_EPOCH@.la \
				$(top_builddir)/ggadget/js/libggadget-js@GGL_EPOCH@.la

libggadget_webkitjs@GGL_EPOCH@_la_LDFLAGS = \
				-version-info $(LIBGGADGET_VERSION) \
				-no-undefined \
				-export-dynamic

extension_LTLIBRARIES = webkit-script-runtime.la
extensiondir = $(GGL_MODULE_DIR)

webkit_script_runtime_la_SOURCES = \
				webkit_script_runtime.cc

webkit_script_runtime_la_CXXFLAGS = \
				$(DEFAULT_COMPILE_FLAGS)

webkit_script_runtime_la_CPPFLAGS = \
				$(JAVA_SCRIPT_CORE_CFLAGS) \
				$(PREDEFINED_MACROS)

webkit_script_runtime_la_LIBADD = \
				$(JAVA_SCRIPT_CORE_LIBS) \
				$(top_builddir)/ggadget/libggadget@GGL_EPOCH@.la \
				libggadget-webkitjs@GGL_EPOCH@.la

webkit_script_runtime_la_LDFLAGS = \
				-no-undefined \
				-avoid-version \
				-module

endif
