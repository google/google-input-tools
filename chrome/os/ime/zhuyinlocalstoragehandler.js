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
 * @fileoverview local storage handlers for zhuyin.
 */
goog.provide('goog.ime.chrome.os.ZhuyinLocalStorageHandler');

goog.require('goog.ime.chrome.os.ConfigFactory');
goog.require('goog.ime.chrome.os.KeyboardLayouts');
goog.require('goog.ime.chrome.os.LocalStorageHandler');
goog.require('goog.ime.offline.InputToolCode');



/**
 * The local storage handler, which gets and sets key-value pairs to local
 * storage for zhuyin input method.
 *
 * @constructor
 * @extends {goog.ime.chrome.os.LocalStorageHandler}
 */
goog.ime.chrome.os.ZhuyinLocalStorageHandler = function() {
  /**
   * The keys for the local storage.
   *
   * @enum {string}
   * @private
   */
  this.keys_ = {
    LAYOUT: 'zhuyin-lt',
    SELECT_KEYS: 'zhuyin-sk',
    PAGE_SIZE: 'zhuyin-ps'
  };
};
goog.inherits(goog.ime.chrome.os.ZhuyinLocalStorageHandler,
    goog.ime.chrome.os.LocalStorageHandler);


/**
 * Gets the keyboard layout from the local storage.
 *
 * @return {string} The layout id.
 */
goog.ime.chrome.os.ZhuyinLocalStorageHandler.prototype.getLayout = function() {
  var key = this.keys_.LAYOUT;
  return /** @type {string} */ (this.getLocalStorageItem(
      key, goog.ime.chrome.os.KeyboardLayouts.STANDARD));
};


/**
 * Gets the select keys from the local storage.
 *
 * @return {string} The select keys.
 */
goog.ime.chrome.os.ZhuyinLocalStorageHandler.prototype.getSelectKeys =
    function() {
  var key = this.keys_.SELECT_KEYS;
  return /** @type {string} */ (this.getLocalStorageItem(
      key, '1234567890'));
};


/**
 * Gets the select keys from the local storage.
 *
 * @return {number} The page size.
 */
goog.ime.chrome.os.ZhuyinLocalStorageHandler.prototype.getPageSize =
    function() {
  var key = this.keys_.PAGE_SIZE;
  return /** @type {number} */ (this.getLocalStorageItem(
      key, 10));
};


/**
 * Sets the layout.
 *
 * @param {string} value The layout id.
 */
goog.ime.chrome.os.ZhuyinLocalStorageHandler.prototype.setLayout = function(
    value) {
  var key = this.keys_.LAYOUT;
  this.setLocalStorageItem(key, value);
};


/**
 * Sets the select keys.
 *
 * @param {string} value The select keys.
 */
goog.ime.chrome.os.ZhuyinLocalStorageHandler.prototype.setSelectKeys =
    function(
    value) {
  var key = this.keys_.SELECT_KEYS;
  this.setLocalStorageItem(key, value);
};


/**
 * Sets the page size.
 *
 * @param {number} value The page size.
 */
goog.ime.chrome.os.ZhuyinLocalStorageHandler.prototype.setPageSize = function(
    value) {
  var key = this.keys_.PAGE_SIZE;
  this.setLocalStorageItem(key, value);
};


/** @override */
goog.ime.chrome.os.ZhuyinLocalStorageHandler.prototype.
    updateControllerSettings = function(controller) {
  var inputToolCode = goog.ime.offline.InputToolCode.
      INPUTMETHOD_ZHUYIN_CHINESE_TRADITIONAL;

  var layout = this.getLayout();
  var selectKeys = this.getSelectKeys();
  var pageSize = Number(this.getPageSize());
  controller.setPageSettings(inputToolCode, layout, selectKeys, pageSize);

  var keyboardLayouts = goog.ime.chrome.os.KeyboardLayouts;
  var config = goog.ime.chrome.os.ConfigFactory.getInstance().
      getConfig(inputToolCode);
  if ((config.layout == keyboardLayouts.HSU ||
      config.layout == keyboardLayouts.ETEN26) &&
      config.selectKeys == '1234567890') {
    config.autoHighlight = true;
  } else {
    config.autoHighlight = false;
  }
};
