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
 * @fileoverview local storage handler to set/get variables from local storage.
 */
goog.provide('goog.ime.chrome.os.LocalStorageHandler');



/**
 * The local storage handler, which gets and sets key-value pairs to local
 * storage.
 *
 * @constructor
 */
goog.ime.chrome.os.LocalStorageHandler = function() {
};


/**
 * Gets an item from local storage.
 *
 * @param {!string} key The item key.
 * @param {*} defaultValue The default value.
 * @return {*} The value from local storage.
 * @protected
 */
goog.ime.chrome.os.LocalStorageHandler.prototype.
    getLocalStorageItem = function(key, defaultValue) {
  var value = localStorage.getItem(key);
  if (value == undefined) {
    return defaultValue;
  }
  return JSON.parse(value);
};


/**
 * Sets an item in local storage.
 *
 * @param {!string} key The item key.
 * @param {*} value The item value.
 * @protected
 */
goog.ime.chrome.os.LocalStorageHandler.prototype.
    setLocalStorageItem = function(key, value) {
  localStorage.setItem(key, JSON.stringify(value));
};


/**
 * Updates the settings for a controller.
 *
 * @param {goog.ime.chrome.os.Controller} controller
 */
goog.ime.chrome.os.LocalStorageHandler.prototype.
    updateControllerSettings = function(controller) {
};
