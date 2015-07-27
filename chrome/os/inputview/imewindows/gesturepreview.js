// Copyright 2015 The ChromeOS IME Authors. All Rights Reserved.
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
goog.provide('i18n.input.chrome.inputview.GesturePreview');

goog.require('goog.dom');
goog.require('goog.dom.TagName');


goog.scope(function() {
var GesturePreview = i18n.input.chrome.inputview.GesturePreview;


/**
 * The ID of the preview text div.
 *
 * @type {string}
 * @private
 */
GesturePreview.PREVIEW_TEXT_ID_ = 'preview_text';


/**
 * Sets the gesture preview word which this window will display.
 *
 * @param {!string} word The word to display.
 * @private
 */
GesturePreview.setPreviewWord_ = function(word) {
  var textDiv = document.getElementById(GesturePreview.PREVIEW_TEXT_ID_);
  // If the text div is already created, just update the text and do not
  // re-create the dom structure.
  if (textDiv) {
    textDiv.textContent = word;
    return;
  }
};


// TODO: export public functions here
goog.exportSymbol(
    'gesturePreview.setPreviewWord', GesturePreview.setPreviewWord_);


});  // goog.scope
