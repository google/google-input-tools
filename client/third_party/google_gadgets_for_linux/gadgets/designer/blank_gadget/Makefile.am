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

BLANK_GADGET_FILES = \
		en/strings.xml \
		stock_images/button_down.png \
		stock_images/button_over.png \
		stock_images/button_up.png \
		stock_images/checkbox_checked_down.png \
		stock_images/checkbox_checked_over.png \
		stock_images/checkbox_checked_up.png \
		stock_images/checkbox_down.png \
		stock_images/checkbox_over.png \
		stock_images/checkbox_up.png \
		stock_images/progressbar_empty.png \
		stock_images/progressbar_full.png \
		stock_images/radiobutton_checked.png \
		stock_images/radiobutton.png \
		gadget.gmanifest \
		icon_large.png \
		icon_small.png \
		main.js \
		main.xml

EXTRA_DIST = $(BLANK_GADGET_FILES)
CLEANFILES = *.gg

if GGL_HAVE_ZIP
blank_gadget_DATA = blank-gadget.gg
blank_gadgetdir = $(GGL_RESOURCE_DIR)/designer

blank-gadget.gg: $(BLANK_GADGET_FILES)
	(cd $(abs_srcdir); $(ZIP) -r -u $(abs_builddir)/$@ $(BLANK_GADGET_FILES))
else
nobase_blank_gadget_DATA = $(BLANK_GADGET_FILES)
blank_gadgetdir = $(GGL_RESOURCE_DIR)/designer/blank-gadget
endif
