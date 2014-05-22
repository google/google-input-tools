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

EXTRA_DIST		= js_unittest.js CMakeLists.txt

noinst_HEADERS		= gtest_internal_inl.h \
			  gtest_internal.h \
			  gtest.h \
			  gtest_pred_impl.h \
			  gtest_prod.h \
			  gtest_spi.h

check_LTLIBRARIES	= libgtest.la

libgtest_la_SOURCES	= gtest.cc

libgtest_la_CXXFLAGS	= $(DEFAULT_COMPILE_FLAGS)
libgtest_la_CPPFLAGS	= $(PREDEFINED_MACROS)

if GGL_HOST_WINDOWS
  libgtest_la_CPPFLAGS	+= -D_MSC_VER
else
if GGL_HOST_MACOSX
  libgtest_la_CPPFLAGS += -D__APPLE__
else
  libgtest_la_CPPFLAGS += -DOS_LINUX
endif
endif
