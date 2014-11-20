// Copyright 2013 The ChromeOS IME Authors. All Rights Reserved.
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

/**
 * @fileoverview local storage handlers for pinyin.
 */
goog.provide('goog.ime.chrome.os.PinyinLocalStorageHandler');

goog.require('goog.ime.chrome.os.LocalStorageHandler');
goog.require('goog.ime.offline.InputToolCode');



/**
 * The local storage handler, which gets and sets key-value pairs to local
 * storage for pinyin input method.
 *
 * @constructor
 * @extends {goog.ime.chrome.os.LocalStorageHandler}
 */
goog.ime.chrome.os.PinyinLocalStorageHandler = function() {
  /**
   * The default fuzzy expansion pairs for pinyin.
   *
   * @type {!Object.<string, boolean>}
   * @private
   */
  this.fuzzyPinyinExpansionPairs_ = {
    'an_ang': undefined,
    'c_ch': undefined,
    'en_eng': undefined,
    'f_h': undefined,
    'ian_iang': undefined,
    'in_ing': undefined,
    'k_g': undefined,
    'l_n': undefined,
    'r_l': undefined,
    's_sh': undefined,
    'uan_uang': undefined,
    'z_zh': undefined
  };

  /**
   * The keys for the local storage.
   *
   * @enum {string}
   * @private
   */
  this.keys_ = {
    FUZZY_PINYIN_ENABLED: 'pinyin-fpe',
    USER_DICT_ENABLED: 'pinyin-ud',
    FUZZY_EXPANSIONS: 'pinyin-fe',
    TOP_PAGE: 'pinyin-tp',
    BOTTOM_PAGE: 'pinyin-bp',
    INIT_LANG: 'pinyin-il',
    INIT_SBC: 'pinyin-is',
    INIT_PUNC: 'pinyin-ip'
  };
};
goog.inherits(goog.ime.chrome.os.PinyinLocalStorageHandler,
    goog.ime.chrome.os.LocalStorageHandler);


/**
 * Whether the fuzzy pinyin is enabled.
 *
 * @return {boolean} True if it is enabled.
 */
goog.ime.chrome.os.PinyinLocalStorageHandler.prototype.
    getFuzzyPinyinEnabled = function() {
  var key = this.keys_.FUZZY_PINYIN_ENABLED;
  return !!this.getLocalStorageItem(key, false);
};


/**
 * Whether the user dictioanry is enabled.
 *
 * @return {boolean} True if it is enabled.
 */
goog.ime.chrome.os.PinyinLocalStorageHandler.prototype.
    getUserDictEnabled = function() {
  var key = this.keys_.USER_DICT_ENABLED;
  return !!this.getLocalStorageItem(key, true);
};


/**
 * Gets the fuzzy pinyin expansions, it returns a map whose keys are the
 * expansion pairs, and values are true or false to indicate whether the
 * expansion should be used.
 *
 * @return {Object.<string, boolean>} The fuzzy expansions.
 */
goog.ime.chrome.os.PinyinLocalStorageHandler.prototype.
    getFuzzyExpansions = function() {
  var key = this.keys_.FUZZY_EXPANSIONS;
  return /** @type {Object.<string, boolean>} */ (this.getLocalStorageItem(
      key, this.fuzzyPinyinExpansionPairs_));
};


/**
 * Whether the - and = keys are used to page down and page up.
 *
 * @return {boolean} True if it is enabled.
 */
goog.ime.chrome.os.PinyinLocalStorageHandler.prototype.
    getTopPageEnabled = function() {
  var key = this.keys_.TOP_PAGE;
  return !!this.getLocalStorageItem(key, true);
};


/**
 * Whether the , and . keys are used to page down and page up.
 *
 * @return {boolean} True if it is enabled.
 */
goog.ime.chrome.os.PinyinLocalStorageHandler.prototype.
    getBottomPageEnabled = function() {
  var key = this.keys_.BOTTOM_PAGE;
  return !!this.getLocalStorageItem(key, true);
};


/**
 * Whether the inital language is Chinese.
 *
 * @return {boolean} True if it is enabled.
 */
goog.ime.chrome.os.PinyinLocalStorageHandler.prototype.
    getInitLang = function() {
  var key = this.keys_.INIT_LANG;
  return !!this.getLocalStorageItem(key, true);
};


/**
 * Whether the inital character width is wide.
 *
 * @return {boolean} True if it is enabled.
 */
goog.ime.chrome.os.PinyinLocalStorageHandler.prototype.
    getInitSBC = function() {
  var key = this.keys_.INIT_SBC;
  return !!this.getLocalStorageItem(key, false);
};


/**
 * Whether the inital punctaution is wide.
 *
 * @return {boolean} True if it is enabled.
 */
goog.ime.chrome.os.PinyinLocalStorageHandler.prototype.
    getInitPunc = function() {
  var key = this.keys_.INIT_PUNC;
  return !!this.getLocalStorageItem(key, true);
};


/**
 * Sets fuzzy pinyin enabled value.
 *
 * @param {boolean} value Whether it is enabled.
 */
goog.ime.chrome.os.PinyinLocalStorageHandler.prototype.
    setFuzzyPinyinEnabled = function(value) {
  var key = this.keys_.FUZZY_PINYIN_ENABLED;
  this.setLocalStorageItem(key, value);
};


/**
 * Sets user dictioanry enabled value.
 *
 * @param {boolean} value Whether it is enabled.
 */
goog.ime.chrome.os.PinyinLocalStorageHandler.prototype.
    setUserDictEnabled = function(value) {
  var key = this.keys_.USER_DICT_ENABLED;
  this.setLocalStorageItem(key, value);
};


/**
 * Sets the fuzzy pinyin expansions, it returns a map whose keys are the
 * expansion pairs, and values are true or false to indicate whether the
 * expansion should be used.
 *
 * @param {Object.<string, boolean>} value The fuzzy pinyin expansions.
 */
goog.ime.chrome.os.PinyinLocalStorageHandler.prototype.
    setFuzzyExpansions = function(value) {
  var key = this.keys_.FUZZY_EXPANSIONS;
  this.setLocalStorageItem(key, value);
};


/**
 * Sets whether the - and = are used to page down and page up.
 *
 * @param {boolean} value Whether it is enabled.
 */
goog.ime.chrome.os.PinyinLocalStorageHandler.prototype.
    setTopPageEnabled = function(value) {
  var key = this.keys_.TOP_PAGE;
  this.setLocalStorageItem(key, value);
};


/**
 * Sets whether the , and . are used to page down and page up.
 *
 * @param {boolean} value Whether it is enabled.
 */
goog.ime.chrome.os.PinyinLocalStorageHandler.prototype.
    setBottomPageEnabled = function(value) {
  var key = this.keys_.BOTTOM_PAGE;
  this.setLocalStorageItem(key, value);
};


/**
 * Sets whether the inital language is Chinese.
 *
 * @param {boolean} value Whether it is enabled.
 */
goog.ime.chrome.os.PinyinLocalStorageHandler.prototype.
    setInitLang = function(value) {
  var key = this.keys_.INIT_LANG;
  this.setLocalStorageItem(key, value);
};


/**
 * Sets whether the inital character width is wide.
 *
 * @param {boolean} value Whether it is enabled.
 */
goog.ime.chrome.os.PinyinLocalStorageHandler.prototype.
    setInitSBC = function(value) {
  var key = this.keys_.INIT_SBC;
  this.setLocalStorageItem(key, value);
};


/**
 * Sets whether the inital punctuation width is wide.
 *
 * @param {boolean} value Whether it is enabled.
 */
goog.ime.chrome.os.PinyinLocalStorageHandler.prototype.
    setInitPunc = function(value) {
  var key = this.keys_.INIT_PUNC;
  this.setLocalStorageItem(key, value);
};


/** @override */
goog.ime.chrome.os.PinyinLocalStorageHandler.prototype.
    updateControllerSettings = function(controller) {
  var inputToolCode = goog.ime.offline.InputToolCode.
      INPUTMETHOD_PINYIN_CHINESE_SIMPLIFIED;

  var fuzzyPinyinEnabled = this.getFuzzyPinyinEnabled();
  var fuzzyExpansions = this.getFuzzyExpansions();
  controller.setFuzzyExpansions(
      inputToolCode, fuzzyPinyinEnabled, fuzzyExpansions);

  var userDictEnabled = this.getUserDictEnabled();
  controller.setUserDictEnabled(inputToolCode, userDictEnabled);

  var pageupChars = '';
  var pagedownChars = '';
  if (this.getTopPageEnabled()) {
    pageupChars += '=';
    pagedownChars += '\\\-';
  }
  if (this.getBottomPageEnabled()) {
    pageupChars += '.';
    pagedownChars += ',';
  }
  controller.setPageMoveChars(inputToolCode, pageupChars, pagedownChars);

  var initLang = this.getInitLang();
  var initSBC = this.getInitSBC();
  var initPunc = this.getInitPunc();
  controller.setInputToolStates(inputToolCode, initLang, initSBC, initPunc);
};

