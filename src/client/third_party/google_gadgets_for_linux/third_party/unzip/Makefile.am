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

if GGL_HOST_WINDOWS
  WIN32_INC=iowin32.h
  WIN32_SRC=iowin32.c
endif

noinst_HEADERS		= crypt.h \
			  ioapi.h \
			  mztools.h \
			  unzip.h \
			  zip.h \
			  $(WIN32_INC)

noinst_LTLIBRARIES	= libunzip.la

libunzip_la_SOURCES	= ioapi.c \
			  mztools.c \
			  unzip.c \
			  zip.c \
			  $(WIN32_SRC)

libunzip_la_CPPFLAGS	= @ZLIB_CPPFLAGS@ $(PREDEFINED_MACROS)
libunzip_la_CFLAGS	= $(DEFAULT_COMPILE_FLAGS)
libunzip_la_LDFLAGS	= @ZLIB_LDFLAGS@
libunzip_la_LIBADD	= @ZLIB_LIBS@
