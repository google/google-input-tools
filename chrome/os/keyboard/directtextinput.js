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
 * @fileoverview Definition of direct text input.
 */

goog.provide('i18n.input.chrome.vk.DirectTextInput');

goog.require('i18n.input.chrome.vk.DeferredApi');
goog.require('i18n.input.chrome.vk.TextInput');



/**
 * The direct-style text input, which will commit text directly into the input
 * box.
 *
 * @param {InputContext} context The input box context.
 * @constructor
 * @extends {i18n.input.chrome.vk.TextInput}
 */
i18n.input.chrome.vk.DirectTextInput = function(context) {
  goog.base(this, context);
};
goog.inherits(i18n.input.chrome.vk.DirectTextInput,
              i18n.input.chrome.vk.TextInput);


/** @override */
i18n.input.chrome.vk.DirectTextInput.prototype.commitText = function(
    text, back) {
  if (back == 1 && text == '') {
    // Returns false to take default action which will trigger
    // surroundingTextChanged event. So doesn't need to update
    // this.textBeforeCursor at here.
    return false;
  }
  if (back > 0) {
    i18n.input.chrome.vk.DeferredApi.deleteSurroundingText(
        this.engineID, this.context.contextID, back, text);
  } else {
    i18n.input.chrome.vk.DeferredApi.commitText(this.context.contextID, text);
  }
  // Sometimes ChromeOS API won't trigger surroundingTextChanged event.
  // So make sure the recorded surrounding text is update to date after the
  // transform.
  this.textBeforeCursor = this.textBeforeCursor.slice(0, -back) + text;
  return true;
};
