# Copyright 2014 The ChromeOS IME Authors. All Rights Reserved.
# limitations under the License.
# See the License for the specific language governing permissions and
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# distributed under the License is distributed on an "AS-IS" BASIS,
# Unless required by applicable law or agreed to in writing, software
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# You may obtain a copy of the License at
# you may not use this file except in compliance with the License.
# Licensed under the Apache License, Version 2.0 (the "License");
#

SHELL:=/bin/bash
PROJECT:=inputview
CLOSURE_COMPILE:=java -jar $(CLOSURE_COMPILER)
BASE_DIR:=../..

all: inputview.js config layout css images

inputview.js:
	@echo compiling inputview.js...
	@$(CLOSURE_COMPILE) --js=$(CLOSURE_LIB)/closure/goog/**.js \
	--js=$(CLOSURE_LIB)/third_party/closure/goog/**.js \
	--js=$(BASE_DIR)/inputview/**.js \
	--js=$(BASE_DIR)/message/**.js \
	--js=$(BASE_DIR)/keyboard/**.js \
	--js=$(BASE_DIR)/datasource.js \
	--js=$(BASE_DIR)/sounds/**.js \
	--js=!**.js \
	--closure_entry_point=i18n.input.chrome.inputview.Controller \
	--closure_entry_point=i18n.input.chrome.inputview.content.compact.letter \
	--closure_entry_point=i18n.input.chrome.inputview.content.compact.util \
	--closure_entry_point=i18n.input.chrome.inputview.content.compact.symbol \
	--closure_entry_point=i18n.input.chrome.inputview.content.compact.more \
	--closure_entry_point=i18n.input.chrome.inputview.content.compact.numberpad \
	--closure_entry_point=i18n.input.chrome.inputview.content.ContextlayoutUtil \
	--closure_entry_point=i18n.input.chrome.inputview.content.util \
	--closure_entry_point=i18n.input.chrome.inputview.EmojiType \
	--closure_entry_point=i18n.input.chrome.inputview.layouts.CompactSpaceRow \
	--closure_entry_point=i18n.input.chrome.inputview.layouts.RowsOf101 \
	--closure_entry_point=i18n.input.chrome.inputview.layouts.RowsOf102 \
	--closure_entry_point=i18n.input.chrome.inputview.layouts.RowsOfCompact \
	--closure_entry_point=i18n.input.chrome.inputview.layouts.RowsOfJP \
	--closure_entry_point=i18n.input.chrome.inputview.layouts.SpaceRow \
	--closure_entry_point=i18n.input.chrome.inputview.layouts.util \
	--closure_entry_point=i18n.input.hwt.util \
	--externs=$(BASE_DIR)/api_externs.js \
	--externs=$(BASE_DIR)/inputview_externs.js \
	--externs=$(BASE_DIR)/xkb_externs.js \
	--compilation_level=WHITESPACE_ONLY \
	--only_closure_dependencies \
	--formatting=PRETTY_PRINT \
	--js_output_file=inputview.js


# ---------------------------------------------------
# Keyboard configurations

config: config/be.js config/br.js config/by.js config/ca.js config/ch.js \
	config/cz.js config/de.js config/dk.js config/ee.js config/es.js \
	config/fi.js config/fr.js config/ge.js config/gr.js config/hr.js \
	config/hu.js config/hwt.js config/ie.js config/il.js config/is.js \
	config/it.js config/jp.js config/latam.js config/lt.js config/mn.js \
	config/no.js config/pl.js config/pt.js config/ro.js config/rs.js \
	config/ru.js config/se.js config/si.js config/sk.js config/tr.js \
	config/ua.js config/us.js config/emoji.js config/bg-phonetic.js \
	config/ca-eng.js config/ca-multix.js config/ch-fr.js config/cz-qwerty.js \
	config/de-neo.js config/es-cat.js config/gb-dvorak.js config/gb-extd.js \
	config/lv-apostrophe.js config/pinyin-zh-CN.js config/ru-phonetic.js \
	config/us-altgr-intl.js config/us-colemak.js config/us-dvorak.js \
	config/us-intl.js

# Entry points for requirements must be included in inputview.js.
config/%.js: $(BASE_DIR)/inputview/config/%_data.js
	@echo copying $@ ...
	@mkdir -p config
	@cp $^ $@

config/pinyin-zh-CN.js: $(BASE_DIR)/inputview/config/pinyin_zh_CN_data.js
	@echo copying $@ ...
	@mkdir -p config
	@cp $^ $@

# ---------------------------------------------------
# Keyboard layouts

layout: inputview_layouts/101kbd.js inputview_layouts/handwriting.js \
	inputview_layouts/compactkbd-qwerty.js inputview_layouts/emoji.js \
	inputview_layouts/102kbd.js inputview_layouts/jpkbd.js

# Include any requirements as closure entry points in build of inputview.js.
inputview_layouts/%.js: $(BASE_DIR)/inputview/layouts/*.js
	@echo copying $@ ...
	@mkdir -p inputview_layouts
	@echo $@ | sed -e 's/^[^\/]*\///g' -e 's/\-/_/g' -e 's/\.js//g' | \
	xargs -I file cp $(BASE_DIR)/inputview/layouts/file_layout.js $@

#----------------------------------------------------
# resources

css: $(BASE_DIR)/inputview/common.css
	@echo copying stylesheets ...
	@cp $(BASE_DIR)/inputview/common.css inputview.css

images: $(BASE_DIR)/inputview/images/*.png
	@echo copying image resources ...
	@mkdir -p images
	@cp -rf $(BASE_DIR)/inputview/images/* images
