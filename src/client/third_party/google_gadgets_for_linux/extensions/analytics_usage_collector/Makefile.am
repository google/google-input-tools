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

extension_LTLIBRARIES	= analytics-usage-collector.la
extensiondir		= $(GGL_MODULE_DIR)

analytics_usage_collector_la_SOURCES = analytics_usage_collector.cc

analytics_usage_collector_la_CXXFLAGS = \
			  $(DEFAULT_COMPILE_FLAGS)

analytics_usage_collector_la_CPPFLAGS = \
			  $(PREDEFINED_MACROS)

analytics_usage_collector_la_LIBADD = \
			  $(top_builddir)/ggadget/libggadget@GGL_EPOCH@.la

analytics_usage_collector_la_LDFLAGS = \
			  -no-undefined \
			  -avoid-version \
			  -module
