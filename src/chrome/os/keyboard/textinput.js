// Copyright 2013 The ChromeOS VK Authors. All Rights Reserved.
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
 * @fileoverview Definition of text input for vk.
 */

goog.provide('goog.ime.chrome.vk.TextInput');



/**
 * The abstract text input.
 *
 * @param {InputContext} context The input box context.
 * @constructor
 */
goog.ime.chrome.vk.TextInput = function(context) {
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
goog.ime.chrome.vk.TextInput.prototype.textBeforeCursor = '';


/**
 * The engine ID.
 *
 * @type {string}
 */
goog.ime.chrome.vk.TextInput.prototype.engineID = '';


/**
 * The input context.
 *
 * @type {InputContext}
 */
goog.ime.chrome.vk.TextInput.prototype.inputContext = null;


/**
 * Sets the input context.
 *
 * @param {InputContext} context The input box context.
 */
goog.ime.chrome.vk.TextInput.prototype.setContext = function(context) {
  this.context = context;
};


/**
 * Commits the transform result to input box.
 *
 * @param {string} text The text to be inserted into input box.
 * @param {number} back The number of characters to be deleted before insert.
 * @return {boolean} Whether successfully committed.
 */
goog.ime.chrome.vk.TextInput.prototype.commitText = goog.abstractMethod;


/**
 * Resets the text input.
 */
goog.ime.chrome.vk.TextInput.prototype.reset = goog.nullFunction;
