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
 * @fileoverview Defines the cangjie model configs.
 */
goog.provide('goog.ime.chrome.os.CangjieConfig');

goog.require('goog.ime.chrome.os.ChineseConfig');
goog.require('goog.ime.chrome.os.StateID');



/**
 * The input method config.
 *
 * @constructor
 * @extends {goog.ime.chrome.os.ChineseConfig}
 */
goog.ime.chrome.os.CangjieConfig = function() {
  goog.base(this);

  /**
   * The transform map.
   *
   * @type {!Object.<string, string>}
   * @private
   */
  this.transMap_ = {
    'a': '日',
    'b': '月',
    'c': '金',
    'd': '木',
    'e': '水',
    'f': '火',
    'g': '土',
    'h': '竹',
    'i': '戈',
    'j': '十',
    'k': '大',
    'l': '中',
    'm': '一',
    'n': '弓',
    'o': '人',
    'p': '心',
    'q': '手',
    'r': '口',
    's': '尸',
    't': '廿',
    'u': '山',
    'v': '女',
    'w': '田',
    'x': '難',
    'y': '卜',
    'z': '符'
  };

  this.punctuationReg = /[^a-z\'.* \r]/i;
  this.editorCharReg = /[a-z\'.*]/;
  this.pageupCharReg = /[=.]/;
  this.pagedownCharReg = /[\-,]/;
  this.maxInputLen = 5;
};
goog.inherits(goog.ime.chrome.os.CangjieConfig,
    goog.ime.chrome.os.ChineseConfig);


/** @override */
goog.ime.chrome.os.CangjieConfig.prototype.preTransform = function(ch) {
  if (ch == '*' &&
      this.states[goog.ime.chrome.os.StateID.LANG].value) {
    return '';
  }
  return goog.base(this, 'preTransform', ch);
};


/** @override */
goog.ime.chrome.os.CangjieConfig.prototype.transformView = function(text) {
  var self = this;
  return text.replace(/[a-z]/g, function(m) {
    return self.transMap_[m];
  });
};
