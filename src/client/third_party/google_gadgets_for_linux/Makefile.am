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

ACLOCAL_AMFLAGS = -I autotools

if GGL_BUILD_LIBLTDL
LIBLTDL_DIR = libltdl
endif

SUBDIRS = $(LIBLTDL_DIR) \
	  third_party \
	  unittest \
	  ggadget \
	  extensions \
	  resources \
	  xdg \
	  gadgets \
	  hosts

EXTRA_DIST = autotools cmake CMakeLists.txt Doxyfile

libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status --recheck

dist-hook:
	rm -rf `find $(distdir) -name '.svn'`
