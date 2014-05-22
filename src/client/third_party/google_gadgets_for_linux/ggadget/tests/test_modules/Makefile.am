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

EXTRA_DIST = CMakeLists.txt \
	     bar_module \
	     fake_module \
	     foo_module \
	     tux_module

INCLUDES		= -I$(top_builddir) \
			  -I$(top_srcdir)

AM_CXXFLAGS		= $(DEFAULT_COMPILE_FLAGS)

AM_LDFLAGS		= -avoid-version \
			  -rpath ${abs_builddir} \
			  -module

check_LTLIBRARIES	= fake-module.la \
			  tux-module.la

fake_module_la_SOURCES	= ../test_module.cc
fake_module_la_CPPFLAGS	= $(PREDEFINED_MACROS) \
			  -DMODULE_NAME=fake-module \
			  -DMODULE_NAME_UNDERSCORE=fake_module
fake_module_la_LIBADD	= $(top_builddir)/unittest/libgtest.la \
			  $(top_builddir)/ggadget/libggadget@GGL_EPOCH@.la


tux_module_la_SOURCES	= ../test_module.cc
tux_module_la_CPPFLAGS	= $(PREDEFINED_MACROS) \
			  -DMODULE_NAME=tux-module \
			  -DMODULE_NAME_UNDERSCORE=tux_module \
			  -DFRAMEWORK_EXTENSION=1 \
			  -DELEMENT_EXTENSION=1
tux_module_la_LIBADD	= $(top_builddir)/unittest/libgtest.la \
			  $(top_builddir)/ggadget/libggadget@GGL_EPOCH@.la
