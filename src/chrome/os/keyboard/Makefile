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

all: js lo manifest pack

debug: jsdbg lo manifest pack

dir:
	@mkdir -p files/layouts

js: dir
	@$(CLOSURE_BUILDER) --root=$(CLOSURE_LIB) --root=./ \
		--namespace="goog.ime.chrome.vk.Background" \
		--output_mode=compiled \
		--compiler_jar=$(CLOSURE_COMPILER) \
		--compiler_flags="--compilation_level=ADVANCED_OPTIMIZATIONS" \
		--compiler_flags="--externs=../api_externs.js" \
	> files/cros_vk_background.js

jsdbg: dir
	@$(CLOSURE_BUILDER) --root=$(CLOSURE_LIB) --root=./ \
		--namespace="goog.ime.chrome.vk.Background" \
		--output_mode=script > files/cros_vk_background.js

lo: dir
	@for file in $(shell ls layouts/*_layout.js); \
	do \
	  out=$${file:0:$${#file}-10}.js; \
	  echo $$out; \
	  java -jar $(CLOSURE_COMPILER) \
		--compilation_level=ADVANCED_OPTIMIZATIONS \
		--externs=layouts/externs.js --js $$file \
		--js_output_file files/$$out; \
	done

manifest: dir
	@cp manifest.json files/; cp -r locales files/_locales

pack:
	@tar -zcf $(PACKAGE) -C files .

clean:
	@rm -rf files/ $(PACKAGE)
