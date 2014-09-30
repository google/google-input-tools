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

PHOTOS_GADGET_FILES= en/strings.xml \
		     fr/strings.xml \
		     zh_CN/strings.xml \
		     images/bkgnd-b.png \
		     images/bkgnd-t.png \
		     images/lbutton_down.png \
		     images/left_1.png \
		     images/pause_1.png \
		     images/play_1.png \
		     images/rect.png \
		     images/right_3.png \
		     images/scrolldown_u.png \
		     images/scrollup_u.png \
		     images/bkgnd-l.png \
		     images/icon_large.png \
		     images/lbutton_over.png \
		     images/left_2.png \
		     images/pause_2.png \
		     images/play_2.png \
		     images/right_1.png \
		     images/scrolldown_d.png \
		     images/scrollup_d.png \
		     images/toolbar_bkgnd.png \
		     images/bkgnd-r.png \
		     images/icon_small.png \
		     images/lbutton_up.png \
		     images/left_3.png \
		     images/pause_3.png \
		     images/play_3.png \
		     images/right_2.png \
		     images/scrolldown_o.png \
		     images/scrollup_o.png \
		     gadget.gmanifest \
		     details.js \
		     feedalt.js \
		     fetchs.js \
		     main.js \
		     options.js \
		     shared.js \
		     slide_shower.js \
		     xml_selector.js \
		     details.xml  \
		     main.xml  \
		     options.xml

EXTRA_DIST = $(PHOTOS_GADGET_FILES)
CLEANFILES = *.gg

if GGL_HAVE_ZIP
photos_gadget_DATA = photos.gg
photos_gadgetdir=$(GGL_RESOURCE_DIR)

photos.gg: $(PHOTOS_GADGET_FILES)
	(cd $(abs_srcdir); $(ZIP) -r -u $(abs_builddir)/$@ $(PHOTOS_GADGET_FILES))
else
nobase_photos_gadget_DATA = $(PHOTOS_GADGET_FILES)
photos_gadgetdir = $(GGL_RESOURCE_DIR)/photos
endif
