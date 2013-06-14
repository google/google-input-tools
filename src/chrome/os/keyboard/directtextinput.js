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
 * @fileoverview Definition of direct text input.
 */

goog.provide('goog.ime.chrome.vk.DirectTextInput');

goog.require('goog.ime.chrome.vk.DeferredApi');
goog.require('goog.ime.chrome.vk.TextInput');



/**
 * The direct-style text input, which will commit text directly into the input
 * box.
 *
 * @param {InputContext} context The input box context.
 * @constructor
 * @extends {goog.ime.chrome.vk.TextInput}
 */
goog.ime.chrome.vk.DirectTextInput = function(context) {
  goog.base(this, context);
};
goog.inherits(goog.ime.chrome.vk.DirectTextInput, goog.ime.chrome.vk.TextInput);


/** @override */
goog.ime.chrome.vk.DirectTextInput.prototype.commitText = function(text, back) {
  if (back == 1 && text == '') {
    // Returns false to take default action which will trigger
    // surroundingTextChanged event. So doesn't need to update
    // this.textBeforeCursor at here.
    return false;
  }
  if (back > 0) {
    goog.ime.chrome.vk.DeferredApi.deleteSurroundingText(
        this.engineID, this.context.contextID, back, text);
  } else {
    goog.ime.chrome.vk.DeferredApi.commitText(this.context.contextID, text);
  }
  // Sometimes ChromeOS API won't trigger surroundingTextChanged event.
  // So make sure the recorded surrounding text is update to date after the
  // transform.
  this.textBeforeCursor = this.textBeforeCursor.slice(0, -back) + text;
  return true;
};
