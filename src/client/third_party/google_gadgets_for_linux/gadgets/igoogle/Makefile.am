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

IGOOGLE_GADGET_FILES= en/strings.xml \
		      fr/strings.xml \
		      zh-CN/strings.xml \
		      gadget.gmanifest \
		      igoogle_32.gif \
		      main.js \
		      main.xml \
		      options.js \
		      options.xml \
		      shared.js \
		      bkgnd.png

EXTRA_DIST = $(IGOOGLE_GADGET_FILES)
CLEANFILES = *.gg

if GGL_HAVE_ZIP
igoogle_gadget_DATA = igoogle.gg
igoogle_gadgetdir=$(GGL_RESOURCE_DIR)

igoogle.gg: $(IGOOGLE_GADGET_FILES)
	(cd $(abs_srcdir); $(ZIP) -r -u $(abs_builddir)/$@ $(IGOOGLE_GADGET_FILES))
else
nobase_igoogle_gadget_DATA = $(IGOOGLE_GADGET_FILES)
igoogle_gadgetdir = $(GGL_RESOURCE_DIR)/igoogle
endif
