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

noinst_HEADERS		= editline.h \
			  unix.h

check_LTLIBRARIES	= libeditline.la

libeditline_la_SOURCES	= editline.c \
			  sysunix.c

libeditline_la_CPPFLAGS	= $(PREDEFINED_MACROS) \
			  -DSYS_UNIX \
			  -DHAVE_STDLIB \
			  -DUNIQUE_HISTORY \
			  -DUSE_DIRENT \
			  -DHIDE \
			  -DHAVE_TCGETATTR \
			  -DANSI_ARROWS

libeditline_la_CFLAGS	= $(DEFAULT_COMPILE_FLAGS) \
			  -Wno-implicit \
			  -Wno-pointer-sign
