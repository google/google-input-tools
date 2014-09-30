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

# EXTRA_DIST =

if GGL_BUILD_LIBGGADGET_GTK
if GGL_BUILD_SOUP_XML_HTTP_REQUEST

INCLUDES	= -I$(top_builddir) \
		  -I$(top_srcdir)

LDADD		= $(PTHRREAD_LIBS) \
		  $(GTK2_LIBS) \
		  $(top_builddir)/unittest/libgtest.la \
		  $(top_builddir)/ggadget/libggadget@GGL_EPOCH@.la \
		  $(top_builddir)/ggadget/gtk/libggadget-gtk@GGL_EPOCH@.la

AM_CPPFLAGS	= $(PREDEFINED_MACROS) $(GTK2_CFLAGS)
AM_CXXFLAGS	= $(DEFAULT_COMPILE_FLAGS)

check_PROGRAMS	= soup_xml_http_request_test

soup_xml_http_request_test_SOURCES = soup_xml_http_request_test.cc

TESTS_ENVIRONMENT	= $(LIBTOOL) --mode=execute $(MEMCHECK_COMMAND)
TESTS 			= $(check_PROGRAMS)

endif
endif
