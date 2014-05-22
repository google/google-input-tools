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

if GGL_BUILD_SMJS_SCRIPT_RUNTIME
SUBDIRS = . tests

INCLUDES		= -I$(top_builddir) \
			  -I$(top_srcdir)

noinst_HEADERS		= converter.h \
			  js_function_slot.h \
			  js_native_wrapper.h \
			  js_script_context.h \
			  js_script_runtime.h \
			  json.h \
			  native_js_wrapper.h \
			  libmozjs_glue.h

noinst_LTLIBRARIES	= libggadget-smjs.la

libggadget_smjs_la_SOURCES = \
			  converter.cc \
			  js_function_slot.cc \
			  js_native_wrapper.cc \
			  js_script_context.cc \
			  js_script_runtime.cc \
			  json.cc \
			  native_js_wrapper.cc \
			  libmozjs_glue.cc

# add "-fcheck-new" to avoid 'operator new' warning when building jscntxt.h
libggadget_smjs_la_CXXFLAGS = \
			  $(DEFAULT_COMPILE_FLAGS) \
			  -fcheck-new

libggadget_smjs_la_CPPFLAGS = \
			  $(PREDEFINED_MACROS) \
			  $(LIBMOZJS_CFLAGS)

libggadget_smjs_la_LIBADD = \
			  $(top_builddir)/ggadget/js/libggadget-js@GGL_EPOCH@.la \
			  $(top_builddir)/ggadget/libggadget@GGL_EPOCH@.la

extension_LTLIBRARIES	= smjs-script-runtime.la
extensiondir		= $(GGL_MODULE_DIR)

smjs_script_runtime_la_SOURCES = \
			 smjs_script_runtime.cc

smjs_script_runtime_la_CXXFLAGS = \
			  $(DEFAULT_COMPILE_FLAGS)

smjs_script_runtime_la_CPPFLAGS = \
			  $(PREDEFINED_MACROS) \
			  $(LIBMOZJS_CFLAGS)

smjs_script_runtime_la_LIBADD = \
			  $(top_builddir)/ggadget/libggadget@GGL_EPOCH@.la \
			  libggadget-smjs.la \
			  $(LIBMOZJS_LIBS) \
			  -ldl

smjs_script_runtime_la_LDFLAGS = \
			  $(LIBMOZJS_LDFLAGS) \
			  -no-undefined \
			  -avoid-version \
			  -module
endif
