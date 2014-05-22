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

DIGITAL_ALARM_CLOCK_GADGET_FILES= en/strings.xml \
		  images/base_brown-left.png \
		  images/base_brown-middle.png \
		  images/base_brown-right.png \
		  images/bell.png \
		  images/bell_ringing.png \
		  images/browse.png \
		  images/browse_d.png \
		  images/button_default.png \
		  images/button_down.png \
		  images/button_hover.png \
		  images/checkbox.png \
		  images/checkbox_chk.png \
		  images/digit_0.png \
		  images/digit_1.png \
		  images/digit_2.png \
		  images/digit_3.png \
		  images/digit_4.png \
		  images/digit_5.png \
		  images/digit_6.png \
		  images/digit_7.png \
		  images/digit_8.png \
		  images/digit_9.png \
		  images/digits_dots.png \
		  images/flashing_lights.png \
		  images/map.png \
		  images/map_daylight.png \
		  images/map_pointers.png \
		  images/radiobutton.png \
		  images/radiobutton_chk.png \
		  alarm.wav \
		  gadget.gmanifest \
		  icon24_daylightclock.png \
		  icon32_daylightclock.png \
		  main.js \
		  main.xml \
		  options.js \
		  options.xml \
		  setup.js \
		  time_display.js

EXTRA_DIST = $(DIGITAL_ALARM_CLOCK_GADGET_FILES)
CLEANFILES = *.gg

if GGL_HAVE_ZIP
digital_alarm_clock_gadget_DATA = digital-alarm-clock.gg
digital_alarm_clock_gadgetdir=$(GGL_RESOURCE_DIR)

digital-alarm-clock.gg: $(DIGITAL_ALARM_CLOCK_GADGET_FILES)
	(cd $(abs_srcdir); $(ZIP) -r -u $(abs_builddir)/$@ $(DIGITAL_ALARM_CLOCK_GADGET_FILES))
else
nobase_digital_alarm_clock_gadget_DATA = $(DIGITAL_ALARM_CLOCK_GADGET_FILES)
digital_alarm_clock_gadgetdir = $(GGL_RESOURCE_DIR)/digital-alarm-clock
endif
