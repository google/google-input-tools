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

DESIGNER_FILES= \
		en/strings.xml \
		fr/strings.xml \
		images/border_bottom_left.png \
		images/border_bottom_middle.png \
		images/border_bottom_right.png \
		images/border_middle_left.png \
		images/border_middle_right.png \
		images/border_top_left.png \
		images/border_top_middle.png \
		images/border_top_right.png \
		images/close_button_down.png \
		images/close_button_over.png \
		images/close_button_up.png \
		images/control_border.png \
		images/folder.png \
		images/folder_over.png \
		images/pin.png \
		images/resize_corner.png \
		images/selection_border.png \
		images/tab_active.png \
		images/tab_background.png \
		images/tab_inactive.png \
		images/tool_a.png \
		images/tool_button.png \
		images/tool_button_active.png \
		images/tool_button_active_over.png \
		images/tool_button_down.png \
		images/tool_button_over.png \
		images/tool_checkbox.png \
		images/tool_combobox.png \
		images/tool_contentarea.png \
		images/tool_div.png \
		images/tool_edit.png \
		images/tool_img.png \
		images/tool_label.png \
		images/tool_listbox.png \
		images/tool_progressbar.png \
		images/tool_radio.png \
		images/tool_scrollbar.png \
		images/view_background.png \
		choose_color_image.js \
		choose_color_image.xml \
		designer.js \
		gadget.gmanifest \
		icon.png \
		main.xml \
		metadata.js \
		properties.js \
		tree.js \
		view_model.js

EXTRA_DIST = $(DESIGNER_FILES)
CLEANFILES = *.gg

if GGL_HAVE_ZIP
designer_gadget_DATA = designer.gg
designer_gadgetdir = $(GGL_RESOURCE_DIR)

designer.gg: $(DESIGNER_FILES)
	(cd $(abs_srcdir); $(ZIP) -r -u $(abs_builddir)/$@ $(DESIGNER_FILES))

install-data-hook:
	(cd $(DESTDIR)$(GGL_RESOURCE_DIR)/designer; \
	 $(ZIP) -r -u ../designer.gg \
		designer-script-utils.so \
		blank-gadget.gg )
	rm -fr $(DESTDIR)$(GGL_RESOURCE_DIR)/designer
else
nobase_designer_DATA = $(DESIGNER_FILES)
designerdir = $(GGL_RESOURCE_DIR)/designer
endif
