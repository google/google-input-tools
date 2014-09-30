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

if GGL_BUILD_GTK_FLASH_ELEMENT

INCLUDES		= -I$(top_builddir) \
			  -I$(top_srcdir)

extension_LTLIBRARIES	= gtk-flash-element.la
extensiondir		= $(GGL_MODULE_DIR)

gtk_flash_element_la_SOURCES = gtk_flash_element.cc

gtk_flash_element_la_CXXFLAGS = $(DEFAULT_COMPILE_FLAGS)

gtk_flash_element_la_CPPFLAGS = $(PREDEFINED_MACROS)

gtk_flash_element_la_LIBADD = $(top_builddir)/ggadget/libggadget@GGL_EPOCH@.la \
			      $(top_builddir)/ggadget/npapi/libggadget-npapi@GGL_EPOCH@.la \
			      $(top_builddir)/ggadget/gtk/libggadget-gtk@GGL_EPOCH@.la

gtk_flash_element_la_LDFLAGS = -avoid-version \
			       -no-undefined \
			       -module

endif
