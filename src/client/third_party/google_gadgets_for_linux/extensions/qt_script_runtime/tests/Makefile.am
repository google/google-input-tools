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
			  $(QT4SCRIPT_LIBS) \
			  ../libggadget-qtjs.la \
			  $(top_builddir)/unittest/libgtest.la \
			  $(top_builddir)/ggadget/libggadget@GGL_EPOCH@.la \
			  $(top_builddir)/ggadget/js/libggadget-js@GGL_EPOCH@.la \
			  $(top_builddir)/ggadget/qt/libggadget-qt@GGL_EPOCH@.la

check_PROGRAMS		= dom_test_shell \
			  js_shell \
			  wrapper_test_shell \
			  cross_context_test

dom_test_shell_SOURCES	= js_shell.cc \
			  dom_test_shell.cc

js_shell_SOURCES	= js_shell.cc \
			  init_custom_objects_stub.cc

wrapper_test_shell_SOURCES = js_shell.cc \
			     wrapper_test_shell.cc \
			     $(top_srcdir)/ggadget/tests/scriptables.cc

cross_context_test_SOURCES = cross_context_test.cc

TESTS_ENVIRONMENT	= $(SHELL)
TESTS			= dom_test_shell.sh \
			  js_shell.sh \
			  wrapper_test_shell.sh \
			  cross_context_test.sh

.PHONY: dom_test_shell.sh \
	js_shell.sh \
	wrapper_test_shell.sh \
	cross_context_test.sh

test_scripts_dir = $(abs_top_srcdir)/ggadget/js/test_scripts

cross_context_test.sh:
	(echo $(LIBTOOL) --mode=execute $(MEMCHECK_COMMAND) \
	      $(abs_builddir)/cross_context_test > $@)

dom_test_shell.sh:
	(echo $(LIBTOOL) --mode=execute $(MEMCHECK_COMMAND) \
	      $(abs_builddir)/dom_test_shell \
	      $(abs_top_srcdir)/unittest/js_unittest.js \
	      $(test_scripts_dir)/dom1_test.js > $@)

js_shell.sh:
	(echo $(LIBTOOL) --mode=execute $(MEMCHECK_COMMAND) \
	      $(abs_builddir)/js_shell \
	      $(abs_top_srcdir)/unittest/js_unittest.js \
	      $(test_scripts_dir)/json_test.js > $@)

wrapper_test_shell.sh:
	(echo $(LIBTOOL) --mode=execute $(MEMCHECK_COMMAND) \
	      $(abs_builddir)/wrapper_test_shell \
	      $(abs_top_srcdir)/unittest/js_unittest.js \
	      $(test_scripts_dir)/wrapper_test.js > $@)

