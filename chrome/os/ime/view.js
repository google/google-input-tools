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
 * @fileoverview View for os decoder chrome os extension.
 */

goog.provide('goog.ime.chrome.os.View');

goog.require('goog.ime.chrome.os.ConfigFactory');



/**
 * The view for os chrome os extension.
 *
 * @param {!goog.ime.chrome.os.Model} model The model.
 * @constructor
 */
goog.ime.chrome.os.View = function(model) {
  /**
   * The model.
   *
   * @type {!goog.ime.chrome.os.Model}
   * @protected
   */
  this.model = model;

  /**
   * The config factory
   *
   * @type {!goog.ime.chrome.os.ConfigFactory}
   * @protected
   */
  this.configFactory = goog.ime.chrome.os.ConfigFactory.getInstance();
};


/**
 * The context.
 *
 * @type {Object}
 * @private
 */
goog.ime.chrome.os.View.prototype.context_ = null;


/**
 * The current input tool.
 *
 * @type {string}
 * @private
 */
goog.ime.chrome.os.View.prototype.inputToolCode_ = '';


/**
 * Sets the context.
 *
 * @param {Object} context The input context.
 */
goog.ime.chrome.os.View.prototype.setContext = function(context) {
  this.context_ = context;
};


/**
 * Sets the input tool.
 */
goog.ime.chrome.os.View.prototype.updateInputTool = function() {
  this.inputToolCode_ = this.configFactory.getInputTool();
  this.updateItems();
};


/**
 * Updates the menu items.
 */
goog.ime.chrome.os.View.prototype.updateItems = function() {
  if (!this.inputToolCode_) {
    return;
  }

  var states = this.configFactory.getCurrentConfig().states;
  var menuItems = [];
  for (var key in states) {
    menuItems.push({
      'id': key,
      'label': states[key].desc,
      'checked': states[key].value,
      'enabled': true,
      'visible': true
    });

    chrome.input.ime.setMenuItems(
        {'engineID': this.inputToolCode_,
          'items': menuItems});
  }
};


/**
 * To show the editor.
 */
goog.ime.chrome.os.View.prototype.show = function() {
  if (this.inputToolCode_) {
    chrome.input.ime.setCandidateWindowProperties(
        {'engineID': this.inputToolCode_,
          'properties': {'visible': true}});
  }
};


/**
 * To hide the editor.
 */
goog.ime.chrome.os.View.prototype.hide = function() {
  if (this.inputToolCode_) {
    chrome.input.ime.setCandidateWindowProperties(
        {'engineID': this.inputToolCode_,
          'properties': {'visible': false}});
  }

  if (this.context_) {
    chrome.input.ime.clearComposition({
      'contextID': this.context_.contextID
    }, function(b) {});

    chrome.input.ime.setCandidates({
      'contextID': this.context_.contextID,
      'candidates': []});
  }
};


/**
 * To refresh the editor.
 */
goog.ime.chrome.os.View.prototype.refresh = function() {
  if (!this.context_) {
    return;
  }
  var segments = this.model.segments;
  var segmentsBeforeCursor = segments.slice(0, this.model.cursorPos);
  var segmentsAfterCursor = segments.slice(this.model.cursorPos);
  var prefix = segments.slice(
      this.model.commitPos, this.model.cursorPos).join('');
  var composing_text = segmentsBeforeCursor.join(' ') +
      this.model.source.slice(prefix.length);
  var pos = composing_text.length;
  if (segmentsAfterCursor.length > 0) {
    composing_text += ' ' + segmentsAfterCursor.join(' ');
  }
  composing_text = this.configFactory.getCurrentConfig().transformView(
      composing_text);
  chrome.input.ime.setComposition(
      {
        'contextID': this.context_.contextID,
        'text': composing_text,
        'cursor': pos});
  this.showCandidates();
};


/**
 * To refresh candidates.
 */
goog.ime.chrome.os.View.prototype.showCandidates = function() {
  var pageIndex = this.model.getPageIndex();
  var pageSize = this.configFactory.getCurrentConfig().pageSize;
  var from = pageIndex * pageSize;
  var to = from + pageSize;
  if (to > this.model.candidates.length) {
    to = this.model.candidates.length;
  }
  var displayItems = [];
  for (var i = from; i < to; i++) {
    var candidate = this.model.candidates[i];
    var label = (i + 1 - from).toString();
    displayItems.push({
      'candidate': candidate.target,
      'label': label,
      'id': i - from});
  }

  chrome.input.ime.setCandidates({
    'contextID': this.context_.contextID,
    'candidates': displayItems});
  if (to > from) {
    var hasHighlight = (this.model.highlightIndex >= 0);
    chrome.input.ime.setCandidateWindowProperties({
      'engineID': this.inputToolCode_,
      'properties': {
        'visible': true,
        'cursorVisible': hasHighlight,
        'vertical': true,
        'pageSize': to - from}});
    if (hasHighlight) {
      chrome.input.ime.setCursorPosition({
        'contextID': this.context_.contextID,
        'candidateID': this.model.highlightIndex % pageSize});
    }
  } else {
    chrome.input.ime.setCandidateWindowProperties({
      'engineID': this.inputToolCode_,
      'properties': {'visible': false}});
  }
};
