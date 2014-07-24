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
# Copyright 2013 Google Inc. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

PROJECT:=chromeos-hangul
VERSION:=$(shell sed -n -e '/"version"/p' misc/manifest.json | cut -f4 -d '"')
PACKAGE:=$(PROJECT)-$(VERSION).tar.gz

NACL_TARGETS:=nacl/*.nexe
JS_TARGETS:=js/ime_build.js
JS_SOURCES:=js/ime.js js/keyboard.js
MISC_FILES:=misc/hangul.nmf misc/hanja.txt \
			misc/symbol.txt misc/icon128.png misc/manifest.json
BACKGROUND_FILE:=misc/backgroundpage.html
BACKGROUND_FILE_DEBUG:=misc/backgroundpage-debug.html

all: debug release

release: $(NACL_TARGETS) $(JS_TARGETS) $(MISC_FILES) $(BACKGROUND_FILE)
	mkdir -p release
	cp $(NACL_TARGETS) $(MISC_FILES) $(BACKGROUND_FILE) release
	cp $(JS_TARGETS) release/ime.js
	mkdir -p release/_locales/en
	cp locales/en/messages.json release/_locales/en

debug: $(NACL_TARGETS) $(JS_SOURCES) $(MISC_FILES) $(BACKGROUND_FILE_DEBUG)
	mkdir -p debug
	cp $(NACL_TARGETS) $(MISC_FILES) $(JS_SOURCES) debug
	cp $(BACKGROUND_FILE_DEBUG) debug/backgroundpage.html
	mkdir -p debug/_locales/en
	cp locales/en/messages.json debug/_locales/en

$(NACL_TARGETS): nacl/*.cc nacl/*.h
	make -C nacl

$(JS_TARGETS): $(JS_SOURCES)
	make -C js

clean:
	rm -rf debug release $(PACKAGE)
	make -C nacl clean
	make -C js clean

pack: release
	mv release $(PROJECT)-$(VERSION)
	tar -zcf $(PACKAGE) $(PROJECT)-$(VERSION)
	rm -rf $(PROJECT)-$(VERSION)
