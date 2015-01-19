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
goog.provide('i18n.input.chrome.xkb.Correction');


/**
 * The correction item.
 *
 * @param {string} source .
 * @param {string} target .
 * @constructor
 */
i18n.input.chrome.xkb.Correction = function(source, target) {
  /** @type {string} */
  this.source = source;

  /** @type {string} */
  this.target = target;

  /**
   * True if in the process of reverting correction.
   *
   * @type {boolean}
   * */
  this.reverting = false;

  /**
   * True if the auto-correct is triggerred by enter.
   *
   * @type {boolean}
   */
  this.isTriggerredByEnter = false;
};


/**
 * True to cancel this correction.
 *
 * @param {string} text .
 * @return {boolean} .
 */
i18n.input.chrome.xkb.Correction.prototype.shouldCancel = function(text) {
  var cancel = !goog.string.endsWith(text.replace(/\u00a0/g, ' '), this.target);
  if (this.isTriggerredByEnter && cancel) {
    cancel = !goog.string.endsWith(text, this.target + '\n');
  }
  return cancel;
};


/**
 * Gets the offset to delete.
 *
 * @param {string} text .
 * @return {number} .
 */
i18n.input.chrome.xkb.Correction.prototype.getTargetLength = function(text) {
  if (this.isTriggerredByEnter && goog.string.endsWith(text, '\n')) {
    return this.target.length + 1;
  }
  return this.target.length;
};


/**
 * Gets the source of the correction.
 *
 * @param {string} text .
 * @return {string} .
 */
i18n.input.chrome.xkb.Correction.prototype.getSource = function(text) {
  if (this.isTriggerredByEnter && goog.string.endsWith(text, '\n')) {
    return this.source + '\n';
  }
  return this.source;
};

