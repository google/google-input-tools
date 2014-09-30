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

GADGET_BROWSER_FILES= \
		en/strings.xml \
		fr/strings.xml \
		zh-CN/strings.xml \
		gadget.gmanifest \
		images/add_button_default.png \
		images/add_button_down.png \
		images/add_button_hover.png \
		images/bg_bottom_left.png \
		images/bg_bottom_middle.png \
		images/bg_bottom_right.png \
		images/bg_middle_left.png \
		images/bg_middle_right.png \
		images/bg_top_left.png \
		images/bg_top_middle.png \
		images/bg_top_right.png \
		images/category_active.png \
		images/category_hover.png \
		images/close_button_down.png \
		images/close_button_over.png \
		images/close_button_up.png \
		images/control_border.png \
		images/default_thumbnail.jpg \
		images/next_default.png \
		images/next_down.png \
		images/next_hover.png \
		images/previous_default.png \
		images/previous_down.png \
		images/previous_hover.png \
		images/resize_corner.png \
		images/scrollbar_thumb.png \
		images/scrollbar_track.png \
		images/search_lens_default.png \
		images/search_lens_down.png \
		images/search_lens_hover.png \
		images/status_added_default.png \
		images/status_adding_default.png \
		images/status_failed_default.png \
		images/thumbnails_default_mask.png \
		images/thumbnails_hover.png \
		images/thumbnails_hover_mask.png \
		images/thumbnails_shadow.png \
		main.js \
		main.xml \
		plugins.js \
		thumbnail.js

EXTRA_DIST = $(GADGET_BROWSER_FILES)
CLEANFILES = *.gg

if GGL_HAVE_ZIP
gadget_browser_DATA = google-gadget-browser.gg
gadget_browserdir=$(GGL_RESOURCE_DIR)

google-gadget-browser.gg: $(GADGET_BROWSER_FILES)
	(cd $(abs_srcdir); $(ZIP) -r -u $(abs_builddir)/$@ $(GADGET_BROWSER_FILES))
else
nobase_gadget_browser_DATA = $(GADGET_BROWSER_FILES)
gadget_browserdir = $(GGL_RESOURCE_DIR)/google-gadget-browser
endif
