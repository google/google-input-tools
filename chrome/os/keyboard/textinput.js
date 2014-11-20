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
 * @fileoverview Definition of text input for vk.
 */

goog.provide('i18n.input.chrome.vk.TextInput');



/**
 * The abstract text input.
 *
 * @param {InputContext} context The input box context.
 * @constructor
 */
i18n.input.chrome.vk.TextInput = function(context) {
  /**
   * The input box context.
   *
   * @type {InputContext}
   * @protected
   */
  this.context = context;
};


/**
 * The text before cursor.
 *
 * @type {string}
 */
i18n.input.chrome.vk.TextInput.prototype.textBeforeCursor = '';


/**
 * The engine ID.
 *
 * @type {string}
 */
i18n.input.chrome.vk.TextInput.prototype.engineID = '';


/**
 * The input context.
 *
 * @type {InputContext}
 */
i18n.input.chrome.vk.TextInput.prototype.inputContext = null;


/**
 * Sets the input context.
 *
 * @param {InputContext} context The input box context.
 */
i18n.input.chrome.vk.TextInput.prototype.setContext = function(context) {
  this.context = context;
};


/**
 * Commits the transform result to input box.
 *
 * @param {string} text The text to be inserted into input box.
 * @param {number} back The number of characters to be deleted before insert.
 * @return {boolean} Whether successfully committed.
 */
i18n.input.chrome.vk.TextInput.prototype.commitText = goog.abstractMethod;


/**
 * Resets the text input.
 */
i18n.input.chrome.vk.TextInput.prototype.reset = goog.nullFunction;
