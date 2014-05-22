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

INCLUDES		= -I$(top_builddir) \
			  -I$(top_srcdir)

LDADD			= $(top_builddir)/unittest/libgtest.la \
			  $(top_builddir)/ggadget/libggadget@GGL_EPOCH@.la \
			  ../libggadget-linux.la

AM_CPPFLAGS		= $(PREDEFINED_MACROS)
AM_CXXFLAGS		= $(DEFAULT_COMPILE_FLAGS)

check_PROGRAMS		= perfmon_test \
			  memory_test \
			  process_test \
			  filesystem_test \
			  filesystem_file_test \
			  filesystem_folder_test \
			  filesystem_textstream_test \
			  filesystem_binarystream_test


memory_test_SOURCES = memory_test.cc
process_test_SOURCES = process_test.cc
filesystem_test_SOURCES = filesystem_test.cc
filesystem_file_test_SOURCES = filesystem_file_test.cc
filesystem_folder_test_SOURCES = filesystem_folder_test.cc
filesystem_textstream_test_SOURCES = filesystem_textstream_test.cc
filesystem_binarystream_test_SOURCES = filesystem_binarystream_test.cc
perfmon_test_SOURCES = perfmon_test.cc $(top_srcdir)/ggadget/tests/native_main_loop.cc

if GGL_BUILD_LIBGGADGET_DBUS
LDADD += $(top_builddir)/ggadget/dbus/libggadget-dbus@GGL_EPOCH@.la

check_PROGRAMS += machine_test \
		  power_test

machine_test_SOURCES = machine_test.cc
power_test_SOURCES = power_test.cc
endif

if GGL_HAVE_NETWORK_MANAGER
check_PROGRAMS += network_test
network_test_SOURCES = network_test.cc
endif

TESTS_ENVIRONMENT	= $(LIBTOOL) --mode=execute $(MEMCHECK_COMMAND)
TESTS 			= $(check_PROGRAMS)
