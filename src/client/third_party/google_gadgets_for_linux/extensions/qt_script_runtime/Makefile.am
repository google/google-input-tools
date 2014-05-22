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

if GGL_BUILD_QT_SCRIPT_RUNTIME
SUBDIRS = . tests

MOC_FILES		= js_function_slot.moc

INCLUDES		= -I$(top_builddir) \
			  -I$(top_srcdir)

noinst_HEADERS		= converter.h \
			  js_function_slot.h \
			  js_script_runtime.h \
			  js_script_context.h \
			  js_native_wrapper.h \
			  json.h

noinst_LTLIBRARIES	= libggadget-qtjs.la

libggadget_qtjs_la_SOURCES = \
			  converter.cc \
			  js_function_slot.cc \
			  js_script_context.cc \
			  js_native_wrapper.cc \
			  js_script_runtime.cc \
			  json.cc

libggadget_qtjs_la_CXXFLAGS = \
			  $(DEFAULT_COMPILE_FLAGS)

libggadget_qtjs_la_CPPFLAGS = \
			  $(QT4SCRIPT_CFLAGS) \
			  $(PREDEFINED_MACROS)

libggadget_qtjs_la_LIBADD = \
			  $(QT4SCRIPT_LIBS) \
			  $(top_builddir)/ggadget/libggadget@GGL_EPOCH@.la \
			  $(top_builddir)/ggadget/js/libggadget-js@GGL_EPOCH@.la

extension_LTLIBRARIES	= qt-script-runtime.la
extensiondir		= $(GGL_MODULE_DIR)

qt_script_runtime_la_SOURCES = \
			  qt_script_runtime.cc

qt_script_runtime_la_CXXFLAGS = \
			  $(DEFAULT_COMPILE_FLAGS)

qt_script_runtime_la_CPPFLAGS = \
			  $(QT4SCRIPT_CFLAGS) \
			  $(PREDEFINED_MACROS)

qt_script_runtime_la_LIBADD = \
			  $(QT4SCRIPT_LIBS) \
			  $(top_builddir)/ggadget/libggadget@GGL_EPOCH@.la \
			  libggadget-qtjs.la

qt_script_runtime_la_LDFLAGS = \
			  -no-undefined \
			  -avoid-version \
			  -module

BUILT_SOURCES		= $(MOC_FILES)
CLEANFILES		= $(MOC_FILES)

js_function_slot.moc:js_function_slot.h
	$(MOC) $< -o $@
endif
