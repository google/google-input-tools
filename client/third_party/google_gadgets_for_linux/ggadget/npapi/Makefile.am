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

# Putting . into SBUDIRS list to force building current directory first.

EXTRA_DIST = CMakeLists.txt

if GGL_BUILD_LIBGGADGET_NPAPI

SUBDIRS= . tests

INCLUDES		= -I$(top_builddir) \
			  -I$(top_srcdir)

noinst_HEADERS		= npapi_wrapper.h \
			  npapi_utils.h \
			  npapi_plugin_script.h

npapiincludedir		= $(GGL_INCLUDE_DIR)/ggadget/npapi
npapiinclude_HEADERS	= npapi_plugin.h

lib_LTLIBRARIES		= libggadget-npapi@GGL_EPOCH@.la

libggadget_npapi@GGL_EPOCH@_la_SOURCES = \
			  npapi_utils.cc \
			  npapi_plugin.cc \
			  npapi_plugin_script.cc

libggadget_npapi@GGL_EPOCH@_la_CXXFLAGS = \
			  $(DEFAULT_COMPILE_FLAGS)

libggadget_npapi@GGL_EPOCH@_la_CPPFLAGS = \
			  $(PREDEFINED_MACROS)

libggadget_npapi@GGL_EPOCH@_la_LIBADD = \
			  $(top_builddir)/ggadget/libggadget@GGL_EPOCH@.la \
			  $(X_LIBS) \
			  -ldl

libggadget_npapi@GGL_EPOCH@_la_LDFLAGS = \
			  -version-info $(LIBGGADGET_VERSION) \
			  -no-undefined \
			  -export-dynamic

pkgconfigdir= $(libdir)/pkgconfig
pkgconfig_DATA= libggadget-npapi@GGL_EPOCH@.pc

endif
