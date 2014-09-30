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

EXTRA_DIST = CMakeLists.txt

INCLUDES		= -I$(top_builddir) \
			  -I$(top_srcdir)

AM_CPPFLAGS		= $(QT4_CFLAGS) \
			  $(PREDEFINED_MACROS)

AM_CXXFLAGS		= $(DEFAULT_COMPILE_FLAGS)

LDADD			= $(QT4_LIBS) \
			  $(top_builddir)/unittest/libgtest.la \
			  $(top_builddir)/ggadget/libggadget@GGL_EPOCH@.la \
			  $(top_builddir)/ggadget/qt/libggadget-qt@GGL_EPOCH@.la


check_PROGRAMS		= main_loop_test

main_loop_test_SOURCES		= main_loop_test.cc

TESTS_ENVIRONMENT	= $(LIBTOOL) --mode=execute $(MEMCHECK_COMMAND)
TESTS 			= $(check_PROGRAMS)

all-local:
	(if test x"$(abs_srcdir)" != x"$(abs_builddir)"; then \
	  for i in $(TEST_IMAGES); do \
	    cp $(abs_srcdir)/"$$i" $(abs_builddir)/; \
	  done; \
	fi)

