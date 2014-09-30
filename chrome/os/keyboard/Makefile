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
# Copyright 2013 The ChromeOS VK Authors. All Rights Reserved.
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
PROJECT:=chromeos-keyboards
CLOSURE_BUILDER:=$(CLOSURE_LIB)/closure/bin/build/closurebuilder.py
VER:=$(shell sed -n -e '/"version"/p' manifest.json | cut -f4 -d '"')
PACKAGE:=$(PROJECT)-$(VER).tar.gz
FILES:=/tmp/cros_vk_files
BG:=$(FILES)/cros_vk_background.js
BGDBG:=$(FILES)/cros_vk_background_dbg.js
EXTERNS:=../api_externs.js

all: js lo manifest pack

debug: jsdbg lo manifest_dbg pack

dir:
	@mkdir -p $(FILES)/layouts

js: | $(BG)

jsdbg: | $(BGDBG)

$(BG): *.js $(EXTERNS) | dir
	@$(CLOSURE_BUILDER) --root=$(CLOSURE_LIB) --root=./ \
		--namespace="i18n.input.chrome.vk.Background" \
		--output_mode=compiled \
		--compiler_jar=$(CLOSURE_COMPILER) \
		--compiler_flags="--compilation_level=ADVANCED_OPTIMIZATIONS" \
		--compiler_flags="--externs=$(EXTERNS)" > $@

$(BGDBG): *.js $(EXTERNS) | dir
	@$(CLOSURE_BUILDER) --root=$(CLOSURE_LIB) --root=./ \
		--namespace="i18n.input.chrome.vk.Background" \
		--output_mode=script > $@

$(FILES)/layouts/%.js: layouts/%.js layouts/externs.js | dir
	@echo $<; \
	java -jar $(CLOSURE_COMPILER) \
		--compilation_level=ADVANCED_OPTIMIZATIONS \
		--externs=layouts/externs.js --js $< \
		--js_output_file $@

lo: $(addprefix $(FILES)/,$(filter-out layouts/externs.js,$(wildcard layouts/*.js)))

manifest:
	@cp manifest.json $(FILES)/; cp -r locales $(FILES)/_locales

manifest_dbg: | manifest
	@sed -i 's/\.js/_dbg\.js/' $(FILES)/manifest.json; \
	sed -i 's/\.js/_dbg\.js/' $(FILES)/*.html

pack:
	@mv $(FILES) $(PROJECT); tar -zcf $(PACKAGE) $(PROJECT); mv $(PROJECT) $(FILES)

clean:
	@rm -rf $(FILES)/ $(PACKAGE)
