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
goog.provide('i18n.input.chrome.AbstractView');

goog.require('goog.events.EventTarget');
goog.require('goog.object');
goog.require('i18n.input.chrome.Env');
goog.require('i18n.input.chrome.message.Name');
goog.require('i18n.input.chrome.message.Type');

goog.scope(function() {
var Env = i18n.input.chrome.Env;
var Name = i18n.input.chrome.message.Name;
var Type = i18n.input.chrome.message.Type;



/**
 * The abstract view.
 *
 * @constructor
 * @extends {goog.events.EventTarget}
 */
i18n.input.chrome.AbstractView = function() {
  i18n.input.chrome.AbstractView.base(this, 'constructor');
  /** @protected {!Env} */
  this.env = Env.getInstance();
};
var AbstractView = i18n.input.chrome.AbstractView;
goog.inherits(AbstractView, goog.events.EventTarget);


/**
 * A flag being flipped when onCompsitionCanceled to avoid composition ops
 * when hiding. Defualt to true and only be set to false when
 * onCompsitionCanceled.
 *
 * @type {boolean}
 */
AbstractView.prototype.resetCompositionWhenHiding = true;


/**
 * Sets the property of the candidate window.
 *
 * @param {string} engineID .
 * @param {boolean} visible .
 * @param {boolean=} opt_cursorVisible .
 * @param {boolean=} opt_vertical .
 * @param {number=} opt_pageSize .
 */
AbstractView.prototype.setCandidateWindowProperties = function(engineID,
    visible, opt_cursorVisible, opt_vertical, opt_pageSize) {
  if (!this.env.isOnScreenKeyboardShown) {
    var properties = goog.object.create(Name.VISIBLE, visible);
    if (goog.isDef(opt_cursorVisible)) {
      properties[Name.CURSOR_VISIBLE] = !!opt_cursorVisible;
    }
    if (goog.isDef(opt_vertical)) {
      properties[Name.VERTICAL] = !!opt_vertical;
    }
    if (goog.isDef(opt_pageSize)) {
      properties[Name.PAGE_SIZE] = opt_pageSize;
    }
    chrome.input.ime.setCandidateWindowProperties(goog.object.create(
        Name.ENGINE_ID, engineID, Name.PROPERTIES, properties));
  } else if (!visible) {
    chrome.runtime.sendMessage(goog.object.create(
        Name.TYPE, Type.CANDIDATES_BACK,
        Name.MSG, goog.object.create(Name.CANDIDATES, [])));
  }
};


/**
 * Sets the composing text.
 *
 * @param {string} composingText The composing text.
 * @param {number} cursorPos The cursor position.
 */
AbstractView.prototype.setComposingText = function(composingText, cursorPos) {
  if (this.env.context) {
    chrome.input.ime.setComposition({
      'contextID': this.env.context.contextID,
      'text': composingText,
      'cursor': cursorPos});
  }
};


/**
 * Sets the candidates.
 *
 * @param {!Array.<!Object>} candidates .
 */
AbstractView.prototype.setCandidates = function(candidates) {
  if (!this.env.isOnScreenKeyboardShown) {
    if (this.env.context) {
      chrome.input.ime.setCandidates(goog.object.create(
          Name.CONTEXT_ID, this.env.context.contextID,
          Name.CANDIDATES, candidates));
    }
  } else {
    chrome.runtime.sendMessage(goog.object.create(
        Name.TYPE, Type.CANDIDATES_BACK,
        Name.MSG, goog.object.create(Name.CANDIDATES, candidates)));
  }
};


/**
 * Sets the cursor position.
 *
 * @param {number} cursorPosition .
 */
AbstractView.prototype.setCursorPosition = function(cursorPosition) {
  if (!this.env.isOnScreenKeyboardShown && this.env.context) {
    chrome.input.ime.setCursorPosition(goog.object.create(
        Name.CONTEXT_ID, this.env.context.contextID,
        Name.CANDIDATE_ID, cursorPosition));
  }
};

});  // goog.scope
