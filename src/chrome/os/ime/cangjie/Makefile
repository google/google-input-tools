// Copyright 2014 The ChromeOS IME Authors. All Rights Reserved.
// limitations under the License.
// See the License for the specific language governing permissions and
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// distributed under the License is distributed on an "AS-IS" BASIS,
// Unless required by applicable law or agreed to in writing, software
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// You may obtain a copy of the License at
// you may not use this file except in compliance with the License.
// Licensed under the Apache License, Version 2.0 (the "License");
//
# Copyright 2013 The ChromeOS IME Authors. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS-IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

#
# GNU Make based build file.  For details on GNU Make see:
# http://www.gnu.org/software/make/manual/make.html
#

SHELL:=/bin/bash
PROJECT:=chromeos-cangjie
CLOSURE_BUILDER:=$(CLOSURE_LIB)/closure/bin/build/closurebuilder.py
VER:=$(shell sed -n -e '/"version"/p' manifest.json | cut -f4 -d '"')
PACKAGE:=$(PROJECT)-$(VER).tar.gz

all: background data manifest pack

debug: background_dbg data manifest pack

dir:
	@mkdir -p files

background: dir
	@$(CLOSURE_BUILDER) --root=$(CLOSURE_LIB) --root=../ \
		--namespace="goog.ime.chrome.os.Background" \
		--output_mode=compiled \
		--compiler_jar=$(CLOSURE_COMPILER) \
		--compiler_flags="--externs=../../api_externs.js" \
		> files/cros_background.js

background_dbg: dir
	@$(CLOSURE_BUILDER) --root=$(CLOSURE_LIB) --root=../ \
		--namespace="goog.ime.chrome.os.Background" \
		--output_mode=script > files/cros_background.js

data: dir
	@cp data files/data.js

manifest: dir
	@cp manifest.json files/; \
	cp ../backgroundpage.html files/; \
	cp -r locales files/_locales

pack:
	@tar -zcf $(PACKAGE) -C files .

clean:
	@rm -rf files/ $(PACKAGE)
