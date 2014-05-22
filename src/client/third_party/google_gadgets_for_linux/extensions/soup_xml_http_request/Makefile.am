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

if GGL_BUILD_SOUP_XML_HTTP_REQUEST
SUBDIRS = . tests

INCLUDES		= -I$(top_builddir) \
			  -I$(top_srcdir)

extension_LTLIBRARIES	= soup-xml-http-request.la
extensiondir		= $(GGL_MODULE_DIR)

soup_xml_http_request_la_SOURCES = soup_xml_http_request.cc

soup_xml_http_request_la_CXXFLAGS = \
			  $(DEFAULT_COMPILE_FLAGS)

soup_xml_http_request_la_CPPFLAGS = \
			  $(LIBSOUP_CFLAGS) \
			  $(PREDEFINED_MACROS)

soup_xml_http_request_la_LIBADD = \
			  $(LIBSOUP_LIBS) \
			  $(PTHREAD_LIBS) \
			  $(top_builddir)/ggadget/libggadget@GGL_EPOCH@.la

soup_xml_http_request_la_LDFLAGS = \
			  -no-undefined \
			  -avoid-version \
			  -module

endif
