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
goog.provide('i18n.input.chrome.LatinCandidateWindow');

goog.require('goog.a11y.aria');
goog.require('goog.a11y.aria.State');
goog.require('goog.array');
goog.require('goog.dom');
goog.require('goog.dom.TagName');
goog.require('goog.dom.classlist');
goog.require('goog.events.Event');
goog.require('goog.events.EventType');
goog.require('goog.style');
goog.require('i18n.input.chrome.CandidateWindow');
goog.require('i18n.input.chrome.floatingwindow.Css');
goog.require('i18n.input.chrome.hmm.EventType');

goog.scope(function() {
var CandidateWindow = i18n.input.chrome.CandidateWindow;
var Css = i18n.input.chrome.floatingwindow.Css;
var EventType = i18n.input.chrome.hmm.EventType;



/**
 * The floating candidate window for Latin IME.
 *
 *  @param {!chrome.app.window.AppWindow} parentWindow The parent app window.
 * @constructor
 * @extends {i18n.input.chrome.CandidateWindow}
 */
i18n.input.chrome.LatinCandidateWindow = function(parentWindow) {
  goog.base(this, parentWindow, true);

  /**
   * The total list of candidates. In the compact mode, only part of the
   * candidates are shown.
   *
   * @private {!Array.<!Object>}
   */
  this.candidates_ = [];


  /** @private {!BoundSize} */
  this.compositionBounds_ = /** @type {!BoundSize} */ (
      {x: 0, y: 0, w: 0, h: 0});
};
var LatinCandidateWindow = i18n.input.chrome.LatinCandidateWindow;
goog.inherits(LatinCandidateWindow, CandidateWindow);


/**
 * The input source.
 *
 * @type {string}
 */
LatinCandidateWindow.prototype.source = '';


/**
 * The number of candidates shown in the compact mode.
 * @type {number}
 */
LatinCandidateWindow.prototype.candidatesCountInCompact = 3;


/**
 * The 'show more' element.
 *
 * @private {!Element}
 */
LatinCandidateWindow.prototype.expandElement_;


/**
 * Whether the candidate window is expanded or not.
 *
 * @type {boolean}
 */
LatinCandidateWindow.prototype.expanded = false;


/** @override */
LatinCandidateWindow.prototype.createDom = function() {
  goog.base(this, 'createDom');

  this.expandElement_ = this.getDomHelper().createDom(
      goog.dom.TagName.DIV, Css.EXPAND);
  goog.a11y.aria.setState(this.expandElement_, goog.a11y.aria.State.LABEL,
      chrome.i18n.getMessage('EXPAND'));
  this.getDomHelper().replaceNode(this.expandElement_, this.auxilaryElement);
};


/** @override */
LatinCandidateWindow.prototype.enterDocument = function() {
  goog.base(this, 'enterDocument');

  this.getHandler().
      listen(this.expandElement_,
          goog.events.EventType.CLICK, this.onExpandDivClicked_).
      listen(this.expandElement_,
          goog.events.EventType.MOUSEOVER,
          this.readItem);

};


/** @override */
LatinCandidateWindow.prototype.setCandidates = function(candidates) {
  this.changeCandidatePosition(this.env.currentBounds,
      this.env.currentBoundsList);
  if (goog.array.equals(candidates, this.candidates_,
      function(cand1, cand2) {
        return cand1.candidate == cand2.candidate;
      })) {
    return;
  }

  this.candidates_ = candidates;
  // Sets the source to calculate the expand element width.

  if (this.source) {
    this.setCandidateElements([]);
    // Make the whole menu transparent.
    goog.dom.classlist.remove(this.getElement(), Css.SHOW);
    goog.dom.classlist.add(this.expandElement_, Css.INLINE_BLOCK);
    // Show ignore correct element when it's a correction.
    this.setCorrectElmentShown(true);
  } else {
    this.setCandidateElements(
        candidates.slice(0, this.candidatesCountInCompact));

    // Restores the menu Css.
    goog.dom.classlist.add(this.getElement(), Css.SHOW);
    goog.dom.classlist.remove(this.expandElement_, Css.INLINE_BLOCK);
    // Hide ignore correct element when it's a prediction.
    this.setCorrectElmentShown(false);
  }
  this.highlightIndex = -1;

  if (this.getElement().contains(this.auxilaryElement)) {
    this.getDomHelper().replaceNode(this.expandElement_, this.auxilaryElement);
  }
  this.adjustWindowSize();
};


/** @override */
LatinCandidateWindow.prototype.moveDownHighlight = function() {
  var candidateElements = goog.dom.getChildren(this.candidateListElement);
  if (candidateElements.length == 0 && this.candidates_.length > 0) {
    this.expandCandidates_();
    return 0;
  }

  if (!this.expanded) {
    if (this.highlightField == CandidateWindow.Field.CANDIDATES &&
        this.highlightIndex == candidateElements.length - 1 &&
        this.highlightIndex < this.candidates_.length) {
      this.expandCandidates_();
    }
  }

  return goog.base(this, 'moveDownHighlight');
};


/** @override */
LatinCandidateWindow.prototype.changeCandidatePosition = function(
    bounds, boundsList) {
  this.compositionBounds_ = this.getCompositionBounds(bounds, boundsList);
  if (this.source) {
    goog.style.setWidth(this.expandElement_, this.compositionBounds_.w);
  } else {
    this.expandElement_.style.width = '';
  }
  this.adjustWindowPosition(this.compositionBounds_);
};


/**
 * Handles the mousedown event on expand div.
 *
 * @param {!goog.events.BrowserEvent} e The mousedown event.
 * @private
 */
LatinCandidateWindow.prototype.onExpandDivClicked_ = function(e) {
  this.expandCandidates_();
  if (this.source && this.highlightIndex < 0) {
    this.highlightIndex = 0;
  }
  this.setCursorPosition(this.highlightIndex);
};


/**
 * Expands the candidates.
 *
 * @private
 */
LatinCandidateWindow.prototype.expandCandidates_ = function() {
  this.expanded = true;
  this.setCandidateElements(this.candidates_);
  this.getDomHelper().replaceNode(this.auxilaryElement, this.expandElement_);
  // Restores the menu Css.
  goog.dom.classlist.add(this.getElement(), Css.SHOW);
  this.adjustWindowSize();
  this.adjustWindowPosition(this.compositionBounds_);
  this.dispatchEvent(new goog.events.Event(EventType.EXPAND));
};


/** @override */
LatinCandidateWindow.prototype.setVisible = function(isVisible) {
  if (!isVisible) {
    this.expanded = false;
  }
  return goog.base(this, 'setVisible', isVisible);
};
});  // goog.scope
