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

if GGL_BUILD_QT_XML_HTTP_REQUEST

MOC_FILES		= qt_xml_http_request_internal.moc

INCLUDES		= -I$(top_builddir) \
			  -I$(top_srcdir)

noinst_HEADERS		= qt_xml_http_request_internal.h

extension_LTLIBRARIES	= qt-xml-http-request.la
extensiondir		= $(GGL_MODULE_DIR)

qt_xml_http_request_la_SOURCES = qt_xml_http_request.cc

nodist_qt_xml_http_request_la_SOURCES = \
			  $(MOC_FILES)

qt_xml_http_request_la_CXXFLAGS = \
			  $(DEFAULT_COMPILE_FLAGS)

qt_xml_http_request_la_CPPFLAGS = \
			  $(QT4NETWORK_CFLAGS) \
			  $(PREDEFINED_MACROS)

qt_xml_http_request_la_LIBADD = \
			  $(QT4NETWORK_LIBS) \
			  $(top_builddir)/ggadget/libggadget@GGL_EPOCH@.la

qt_xml_http_request_la_LDFLAGS = \
			  -no-undefined \
			  -avoid-version \
			  -module

BUILT_SOURCES		= $(MOC_FILES)
CLEANFILES		= $(MOC_FILES)
qt_xml_http_request_internal.moc:qt_xml_http_request_internal.h
	$(MOC) $< -o $@
endif
