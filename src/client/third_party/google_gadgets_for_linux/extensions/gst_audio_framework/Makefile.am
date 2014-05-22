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

if GGL_BUILD_GST_AUDIO_FRAMEWORK

INCLUDES		= -I$(top_builddir) \
			  -I$(top_srcdir)

extension_LTLIBRARIES	= gst-audio-framework.la
extensiondir		= $(GGL_MODULE_DIR)

gst_audio_framework_la_SOURCES = gst_audio_framework.cc

gst_audio_framework_la_CXXFLAGS = \
			  $(DEFAULT_COMPILE_FLAGS)

gst_audio_framework_la_CPPFLAGS = \
			  $(GSTREAMER_CFLAGS) \
			  $(PREDEFINED_MACROS)

gst_audio_framework_la_LIBADD = \
			  $(GSTREAMER_LIBS) \
			  $(top_builddir)/ggadget/libggadget@GGL_EPOCH@.la

gst_audio_framework_la_LDFLAGS = \
			  -no-undefined \
			  -avoid-version \
			  -module

endif
