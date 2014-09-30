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

LDADD			= $(top_builddir)/unittest/libgtest.la \
			  $(top_builddir)/ggadget/js/libggadget-js@GGL_EPOCH@.la \
			  $(top_builddir)/ggadget/libggadget@GGL_EPOCH@.la

check_PROGRAMS		= jscript_massager_test \
			  massager

massager_SOURCES	= massager.cc

jscript_massager_test_SOURCES = jscript_massager_test.cc

TESTS			= jscript_massager_test.sh

.PHONY: jscript_massager_test.sh \
	massager.sh

jscript_massager_test.sh:
	(echo $(LIBTOOL) --mode=execute $(MEMCHECK_COMMAND) \
	      $(abs_builddir)/jscript_massager_test > $@)
	chmod +x $@

massager.sh:
	(echo $(LIBTOOL) --mode=execute $(MEMCHECK_COMMAND) \
	      $(abs_builddir)/massager > $@)
	chmod +x $@


