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

TEST_IMAGES		= 120day.png \
			  base.png \
			  kitty419.jpg \
			  testmask.png \
			  opaque.png

EXTRA_DIST		= $(TEST_IMAGES) CMakeLists.txt

INCLUDES		= -I$(top_builddir) \
			  -I$(top_srcdir)

AM_CPPFLAGS		= @GTK2_CFLAGS@ \
			  $(PREDEFINED_MACROS)

AM_CXXFLAGS		= $(DEFAULT_COMPILE_FLAGS)

LDADD			= @GTK2_LIBS@ \
			  $(top_builddir)/unittest/libgtest.la \
			  $(top_builddir)/ggadget/libggadget@GGL_EPOCH@.la \
			  $(top_builddir)/ggadget/gtk/libggadget-gtk@GGL_EPOCH@.la

if GTK2_TARGET_X11
  LDADD			+= $(X_LIBS)
endif

check_PROGRAMS		= cairo_canvas_test \
			  cairo_graphics_test \
			  basic_element_draw_test \
			  main_loop_test \
			  hotkey_test

cairo_canvas_test_SOURCES	= cairo_canvas_test.cc
cairo_graphics_test_SOURCES	= cairo_graphics_test.cc
basic_element_draw_test_SOURCES	= basic_element_draw_test.cc
main_loop_test_SOURCES		= main_loop_test.cc
hotkey_test_SOURCES		= hotkey_test.cc

TESTS_ENVIRONMENT	= $(LIBTOOL) --mode=execute $(MEMCHECK_COMMAND)
TESTS 			= $(check_PROGRAMS)

all-local:
	(if test x"$(abs_srcdir)" != x"$(abs_builddir)"; then \
	  for i in $(TEST_IMAGES); do \
	    cp -f $(abs_srcdir)/"$$i" $(abs_builddir)/; \
	  done; \
	fi)

