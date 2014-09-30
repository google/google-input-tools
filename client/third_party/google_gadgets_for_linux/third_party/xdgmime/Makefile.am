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

if GGL_ENABLE_XDGMIME

AM_CPPFLAGS = $(PREDEFINED_MACROS) -DXDG_PREFIX=_ggl_xdg
AM_CFLAGS = $(DEFAULT_COMPILE_FLAGS)

noinst_LTLIBRARIES = libxdgmime.la

noinst_HEADERS = \
	xdgmime.h \
	xdgmimealias.h \
	xdgmimecache.h \
	xdgmimeglob.h \
	xdgmimeicon.h \
	xdgmimeint.h \
	xdgmimemagic.h \
	xdgmimeparent.h

libxdgmime_la_SOURCES = \
	xdgmime.c \
	xdgmimealias.c \
	xdgmimecache.c \
	xdgmimeglob.c \
	xdgmimeicon.c \
	xdgmimeint.c \
	xdgmimemagic.c \
	xdgmimeparent.c

noinst_PROGRAMS = test-mime

test_mime_SOURCES = test-mime.c
test_mime_LDADD = libxdgmime.la

endif
