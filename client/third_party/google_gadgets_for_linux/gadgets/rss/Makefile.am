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

RSS_GADGET_FILES= en/strings.xml \
		  fr/strings.xml \
		  zh-CN/strings.xml \
		  gadget.gmanifest \
		  main.js \
		  main.xml \
		  options.js \
		  options.xml \
		  shared.js \
		  rss_bkgnd.png \
		  small_rss_icon.png \
		  large_rss_icon.png \
		  button_down.png \
		  button_over.png \
		  button_up.png

EXTRA_DIST = $(RSS_GADGET_FILES)
CLEANFILES = *.gg

if GGL_HAVE_ZIP
rss_gadget_DATA = rss.gg
rss_gadgetdir=$(GGL_RESOURCE_DIR)

rss.gg: $(RSS_GADGET_FILES)
	(cd $(abs_srcdir); $(ZIP) -r -u $(abs_builddir)/$@ $(RSS_GADGET_FILES))
else
nobase_rss_gadget_DATA = $(RSS_GADGET_FILES)
rss_gadgetdir = $(GGL_RESOURCE_DIR)/rss
endif
