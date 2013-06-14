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
 * @fileoverview Defines the model configs.
 */
goog.provide('goog.ime.chrome.os.Config');

goog.require('goog.ime.chrome.os.KeyboardLayouts');



/**
 * The input method config.
 *
 * @constructor
 */
goog.ime.chrome.os.Config = function() {
  /**
   * The input tool states.
   *
   * @type {Object.<string, goog.ime.chrome.os.State>}
   */
  this.states = {};

  /**
   * The fuzzy expansion paris.
   *
   * @type {Array.<string>}
   */
  this.fuzzyExpansions = [];
};


/**
 * The punctuationReg.
 *
 * @type {!RegExp}
 */
goog.ime.chrome.os.Config.prototype.punctuationReg = /[^a-z0-9 \r]/i;


/**
 * The regexp for editor chars.
 *
 * @type {!RegExp}
 */
goog.ime.chrome.os.Config.prototype.editorCharReg = /[a-z]/i;


/**
 * The pageup chars.
 *
 * @type {!RegExp}
 */
goog.ime.chrome.os.Config.prototype.pageupCharReg = /xyz/g;


/**
 * The pagedown chars.
 *
 * @type {!RegExp}
 */
goog.ime.chrome.os.Config.prototype.pagedownCharReg = /xyz/g;


/**
 * The page size.
 *
 * @type {number}
 */
goog.ime.chrome.os.Config.prototype.pageSize = 5;


/**
 * The maximum allowed input length.
 *
 * @type {number}
 */
goog.ime.chrome.os.Config.prototype.maxInputLen = 40;


/**
 * The request number.
 *
 * @type {number}
 */
goog.ime.chrome.os.Config.prototype.requestNum = 50;


/**
 * Whether automatically highlight the fetched candidates.
 *
 * @type {boolean}
 */
goog.ime.chrome.os.Config.prototype.autoHighlight = true;


/**
 * Whether enables the user dictionary.
 *
 * @type {boolean}
 */
goog.ime.chrome.os.Config.prototype.enableUserDict = true;


/**
 * The keyboard layout.
 *
 * @type {string}
 */
goog.ime.chrome.os.Config.prototype.layout =
    goog.ime.chrome.os.KeyboardLayouts.STANDARD;


/**
 * The select keys.
 *
 * @type {string}
 */
goog.ime.chrome.os.Config.prototype.selectKeys = '1234567890';


/**
 * Transform before the popup editor opened.
 *
 * @param {string} c The incoming char.
 * @return {string} The transformed char.
 */
goog.ime.chrome.os.Config.prototype.preTransform = function(c) {
  return '';
};


/**
 * Transform when the popup editor opened.
 *
 * @param {string} context The text before the input.
 * @param {string} c The incoming char.
 * @return {string} The transformed result.
 */
goog.ime.chrome.os.Config.prototype.transform = function(context, c) {
  return c;
};


/**
 * Transform when the popup editor is going to close.
 *
 * @param {string} c The incoming char.
 * @return {string} The transformed result.
 */
goog.ime.chrome.os.Config.prototype.postTransform = function(c) {
  return c;
};


/**
 * Transform the character on the editor text.
 *
 * @param {string} text The editor text.
 * @return {string} The transformed result.
 */
goog.ime.chrome.os.Config.prototype.transformView = function(text) {
  return text;
};
