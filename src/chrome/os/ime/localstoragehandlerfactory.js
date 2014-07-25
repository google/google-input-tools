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
 * @fileoverview The local storage handler factory to generate local storage
 * handlers for different input methods.
 */
goog.provide('goog.ime.chrome.os.LocalStorageHandlerFactory');

goog.require('goog.ime.chrome.os.PinyinLocalStorageHandler');
goog.require('goog.ime.chrome.os.ZhuyinLocalStorageHandler');
goog.require('goog.ime.offline.InputToolCode');
goog.require('goog.object');



/**
 * The factory to generate local storage handlers.
 *
 * @constructor
 */
goog.ime.chrome.os.LocalStorageHandlerFactory = function() {
  /**
   * The map of input tool code to the local storage handler.
   *
   * @type {!Object.<string, !goog.ime.chrome.os.LocalStorageHandler>}
   * @private
   */
  this.map_ = {};

  this.init_();
};
goog.addSingletonGetter(goog.ime.chrome.os.LocalStorageHandlerFactory);


/**
 * Gets the local storage handler for a given input tool code.
 *
 * @param {!string} inputToolCode the input tool code.
 * @return {goog.ime.chrome.os.LocalStorageHandler} the local storage handler.
 */
goog.ime.chrome.os.LocalStorageHandlerFactory.prototype.
    getLocalStorageHandler = function(inputToolCode) {
  if (this.map_[inputToolCode]) {
    return this.map_[inputToolCode];
  }
  return null;
};


/**
 * Gets all the local storage handlers.
 *
 * @return {!Array.<!goog.ime.chrome.os.LocalStorageHandler>} a map
 * for all local storage handlers.
 */
goog.ime.chrome.os.LocalStorageHandlerFactory.prototype.
    getAllLocalStorageHandlers = function() {
  return goog.object.getValues(this.map_);
};


/**
 * Initializes the local storage handler factory.
 *
 * @private
 */
goog.ime.chrome.os.LocalStorageHandlerFactory.prototype.init_ = function() {
  var code = goog.ime.offline.InputToolCode;

  var pinyinHandler = new goog.ime.chrome.os.PinyinLocalStorageHandler();
  this.map_[code.INPUTMETHOD_PINYIN_CHINESE_SIMPLIFIED] = pinyinHandler;

  var traditionalPinyinHandler =
      new goog.ime.chrome.os.PinyinLocalStorageHandler();
  this.map_[code.INPUTMETHOD_PINYIN_CHINESE_TRADITIONAL] =
      traditionalPinyinHandler;

  var zhuyinHandler = new goog.ime.chrome.os.ZhuyinLocalStorageHandler();
  this.map_[code.INPUTMETHOD_ZHUYIN_CHINESE_TRADITIONAL] = zhuyinHandler;
};
