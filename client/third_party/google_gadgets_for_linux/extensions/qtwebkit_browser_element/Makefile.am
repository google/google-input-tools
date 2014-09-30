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

if GGL_BUILD_QTWEBKIT_BROWSER_ELEMENT
MOC_FILES		= browser_element_internal.moc

INCLUDES		= -I$(top_builddir) \
			  -I$(top_srcdir)

noinst_HEADERS		= browser_element.h \
			  browser_element_internal.h

nodist_qtwekit_browser_element = $(MOC_FILES)

extension_LTLIBRARIES	= qtwebkit-browser-element.la
extensiondir		= $(GGL_MODULE_DIR)

qtwebkit_browser_element_la_SOURCES = \
			  browser_element.cc

qtwebkit_browser_element_la_CXXFLAGS = \
			  $(DEFAULT_COMPILE_FLAGS)

qtwebkit_browser_element_la_CPPFLAGS = \
			  $(QT4WEBKIT_CFLAGS) \
			  $(PREDEFINED_MACROS)

qtwebkit_browser_element_la_LIBADD = \
			  $(QT4WEBKIT_LIBS) \
			  $(top_builddir)/ggadget/libggadget@GGL_EPOCH@.la \
			  $(top_builddir)/ggadget/qt/libggadget-qt@GGL_EPOCH@.la

qtwebkit_browser_element_la_LDFLAGS = \
			  -no-undefined \
			  -avoid-version \
			  -module
BUILT_SOURCES		= $(MOC_FILES)
CLEANFILES		= $(MOC_FILES)

browser_element_internal.moc:browser_element_internal.h
	$(MOC) $< -o $@
endif
