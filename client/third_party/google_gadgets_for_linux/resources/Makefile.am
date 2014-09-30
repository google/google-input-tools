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

RESOURCE_FILES= builtin_gadgets.xml \
		button_down.png \
		button_over.png \
		button_up.png \
		checkbox_checked_down.png \
		checkbox_checked_over.png \
		checkbox_checked_up.png \
		checkbox_down.png \
		checkbox_over.png \
		checkbox_up.png \
		combo_arrow_up.png \
		combo_arrow_down.png \
		combo_arrow_over.png \
		common.js \
		focus_overlay.png \
		gadget_about.xml \
		ggl_about.xml \
		google-gadgets.png \
		google-gadgets-big.png \
		html_details_view.xml \
		messages-catalog.xml \
		pinned.png \
		progressbar_empty.png \
		progressbar_full.png \
		radio_checked_down.png \
		radio_checked_over.png \
		radio_checked_up.png \
		radio_down.png \
		radio_over.png \
		radio_up.png \
		scroll_background.png \
		scroll_background_h.png \
		scrollbar_d.png \
		scrollbar_d_h.png \
		scrollbar_grippy.png \
		scrollbar_grippy_h.png \
		scrollbar_o.png \
		scrollbar_o_h.png \
		scrollbar_u.png \
		scrollbar_u_h.png \
		scrolldown_d.png \
		scrolldown_o.png \
		scrolldown_u.png \
		scrollleft_d.png \
		scrollleft_o.png \
		scrollleft_u.png \
		scrollright_d.png \
		scrollright_o.png \
		scrollright_u.png \
		scrollup_d.png \
		scrollup_o.png \
		scrollup_u.png \
		sidebar_google.png \
		sidebar_add_up.png \
		sidebar_add_down.png \
		sidebar_add_over.png \
		sidebar_menu_up.png \
		sidebar_menu_down.png \
		sidebar_menu_over.png \
		sidebar_minimize_up.png \
		sidebar_minimize_down.png \
		sidebar_minimize_over.png \
		text_details_view.xml \
		unpinned.png \
		unpinned_over.png \
		vd_back_down.png \
		vd_back_normal.png \
		vd_back_over.png \
		vd_bottom_right_corner.png \
		vd_button_background.png \
		vd_close_down.png \
		vd_close_normal.png \
		vd_close_over.png \
		vd_details_button_bkgnd_click.png \
		vd_details_button_bkgnd_normal.png \
		vd_details_button_bkgnd_over.png \
		vd_details_button_negfb_normal.png \
		vd_details_button_negfb_over.png \
		vd_forward_down.png \
		vd_forward_normal.png \
		vd_forward_over.png \
		vd_framed_background.png \
		vd_framed_bottom.png \
		vd_framed_close_down.png \
		vd_framed_close_normal.png \
		vd_framed_close_over.png \
		vd_framed_middle.png \
		vd_framed_top.png \
		vd_main_background.png \
		vd_main_background_minimized.png \
		vd_main_background_transparent.png \
		vd_main_docked_border_h.png \
		vd_main_docked_border_v.png \
		vd_menu_down.png \
		vd_menu_normal.png \
		vd_menu_over.png \
		vd_popin_down.png \
		vd_popin_normal.png \
		vd_popin_over.png \
		vd_popout_down.png \
		vd_popout_normal.png \
		vd_popout_over.png \
		analog_clock_sm.jpg \
		digital_alarm_clock_sm.jpg \
		email_sm.jpg \
		igoogle_sm.jpg \
		photos_sm.jpg \
		rss_sm.jpg \
		scratchpad_sm.jpg \
		todo_sm.jpg \
		weather_sm.jpg \
		en/strings.xml \
		fr/strings.xml \
		zh-CN/strings.xml

EXTRA_DIST=$(RESOURCE_FILES)
CLEANFILES=*.gg

if GGL_HAVE_ZIP
resource_DATA = resources.gg
resourcedir=$(GGL_RESOURCE_DIR)

resources.gg: $(RESOURCE_FILES)
	(cd $(abs_srcdir); $(ZIP) -r -u $(abs_builddir)/$@ $(RESOURCE_FILES))
else
nobase_resource_DATA = $(RESOURCE_FILES)
resourcedir=$(GGL_RESOURCE_DIR)/resources
endif
