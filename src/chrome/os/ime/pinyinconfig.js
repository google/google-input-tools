// Copyright 2013 The ChromeOS IME Authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS-IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * @fileoverview Defines the pinyin model configs.
 */
goog.provide('goog.ime.chrome.os.PinyinConfig');

goog.require('goog.ime.chrome.os.ChineseConfig');



/**
 * The input method config.
 *
 * @constructor
 * @extends {goog.ime.chrome.os.ChineseConfig}
 */
goog.ime.chrome.os.PinyinConfig = function() {
  goog.base(this);
  this.punctuationReg = /[^a-z0-9 \r]/i;
  this.editorCharReg = /[a-z\']/;
  this.pageupCharReg = /[=.]/;
  this.pagedownCharReg = /[\-,]/;
};
goog.inherits(goog.ime.chrome.os.PinyinConfig,
    goog.ime.chrome.os.ChineseConfig);
