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

ADD_SUBDIRECTORY(designer)

ADD_DIR_TO_ZIP(analog_clock ../output/bin/analog-clock.gg)
ADD_DIR_TO_ZIP(digital_alarm_clock ../output/bin/digital-alarm-clock.gg)
ADD_DIR_TO_ZIP(igoogle ../output/bin/igoogle.gg)
ADD_DIR_TO_ZIP(rss ../output/bin/rss.gg)
ADD_DIR_TO_ZIP(photos ../output/bin/photos.gg)
ADD_DIR_TO_ZIP(gadget_browser ../output/bin/google-gadget-browser.gg)

INSTALL_GADGET(analog-clock.gg)
INSTALL_GADGET(digital-alarm-clock.gg)
INSTALL_GADGET(igoogle.gg)
INSTALL_GADGET(rss.gg)
INSTALL_GADGET(photos.gg)
INSTALL_GADGET(google-gadget-browser.gg)
