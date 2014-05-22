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
SUBDIRS 		= tests

INCLUDES		= -I$(top_builddir) \
			  -I$(top_srcdir)

EXTRA_DIST		= jscript_massager.l \
			  test_scripts \
			  CMakeLists.txt

jsincludedir		= $(GGL_INCLUDE_DIR)/ggadget/js
jsinclude_HEADERS	= jscript_massager.h \
			  js_utils.h

lib_LTLIBRARIES		= libggadget-js@GGL_EPOCH@.la

libggadget_js@GGL_EPOCH@_la_SOURCES = \
			  jscript_massager.cc \
			  js_utils.cc

libggadget_js@GGL_EPOCH@_la_CPPFLAGS = \
			  $(DEFAULT_COMPILE_FLAGS)

libggadget_js@GGL_EPOCH@_la_CXXFLAGS = \
			  $(DEFAULT_COMPILE_FLAGS)

libggadget_js@GGL_EPOCH@_la_LIBADD = \
			  $(top_builddir)/ggadget/libggadget@GGL_EPOCH@.la

libggadget_js@GGL_EPOCH@_la_LDFLAGS = \
			  -version-info $(LIBGGADGET_VERSION) \
			  -no-undefined \
			  -export-dynamic

jscript_massager.cc : jscript_massager.l
	$(LEX) -o $@ $<

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libggadget-js@GGL_EPOCH@.pc
