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

#EXTRA_DIST = CMakeLists.txt

INCLUDES		= -I$(top_builddir) \
			  -I$(top_srcdir)

extension_LTLIBRARIES	= html-flash-element.la
extensiondir		= $(GGL_MODULE_DIR)

html_flash_element_la_SOURCES = html_flash_element.cc

html_flash_element_la_CXXFLAGS = $(DEFAULT_COMPILE_FLAGS)

html_flash_element_la_CPPFLAGS = $(PREDEFINED_MACROS)

html_flash_element_la_LIBADD = $(top_builddir)/ggadget/libggadget@GGL_EPOCH@.la

html_flash_element_la_LDFLAGS = -no-undefined -avoid-version -module
