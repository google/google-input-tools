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

if GGL_BUILD_GTK_SYSTEM_FRAMEWORK

INCLUDES		= -I$(top_builddir) \
			  -I$(top_srcdir)

extension_LTLIBRARIES	= gtk-system-framework.la
extensiondir		= $(GGL_MODULE_DIR)

gtk_system_framework_la_SOURCES = gtk_system_framework.cc

gtk_system_framework_la_CXXFLAGS = \
			  $(DEFAULT_COMPILE_FLAGS)

gtk_system_framework_la_CPPFLAGS = \
			  @GTK2_CFLAGS@ \
			  $(PREDEFINED_MACROS)

gtk_system_framework_la_LIBADD = \
			  @GTK2_LIBS@ \
			  $(top_builddir)/ggadget/libggadget@GGL_EPOCH@.la \
			  $(top_builddir)/ggadget/gtk/libggadget-gtk@GGL_EPOCH@.la \
			  $(top_builddir)/ggadget/xdg/libggadget-xdg@GGL_EPOCH@.la

gtk_system_framework_la_LDFLAGS = \
			  -no-undefined \
			  -avoid-version \
			  -module

endif
