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
goog.provide('i18n.input.chrome.CandidateWindow');

goog.require('goog.array');
goog.require('goog.dom');
goog.require('goog.dom.TagName');
goog.require('goog.dom.classlist');
goog.require('goog.events.EventType');
goog.require('goog.math.Coordinate');
goog.require('goog.style');
goog.require('i18n.input.chrome.ClickCandidateEvent');
goog.require('i18n.input.chrome.Env');
goog.require('i18n.input.chrome.EventType');
goog.require('i18n.input.chrome.FloatingWindow');
goog.require('i18n.input.chrome.floatingwindow.Css');

goog.scope(function() {
var ClickCandidateEvent = i18n.input.chrome.ClickCandidateEvent;
var Css = i18n.input.chrome.floatingwindow.Css;
var FloatingWindow = i18n.input.chrome.FloatingWindow;
var EventType = i18n.input.chrome.EventType;



/**
 * The floating candidate window in Chrome OS.
 *
 * TODO: After personal dictionary is consolidated, the USER_DICT option should
 *     be added back to this candidate window.
 *
 * @param {!chrome.app.window.AppWindow} parentWindow The parent app window.
 * @param {boolean=} opt_addSetting .
 * @constructor
 * @extends {i18n.input.chrome.FloatingWindow}
 */
i18n.input.chrome.CandidateWindow = function(parentWindow, opt_addSetting) {
  goog.base(this, parentWindow, true);

  /** @private {boolean} */
  this.hasSettingDiv_ = !!opt_addSetting;

  /** @protected */
  this.env = i18n.input.chrome.Env.getInstance();
  this.setVisible(false);
};
var CandidateWindow = i18n.input.chrome.CandidateWindow;
goog.inherits(CandidateWindow, FloatingWindow);


/**
 * The CSS names.
 *
 * @enum {string}
 */
CandidateWindow.CSS = {
  BODY: goog.getCssName('inputview-suggest-body'),
  BOLD: goog.getCssName('inputview-suggest-bold'),
  EXPAND: goog.getCssName('inputview-suggest-expand'),
  HIGHLIGHT: goog.getCssName('inputview-suggest-highlight'),
  INLINE_BLOCK: goog.getCssName('inputview-suggest-inline-block'),
  MENU: goog.getCssName('inputview-suggest-menu'),
  MENUITEM: goog.getCssName('inputview-suggest-menuitem'),
  NORMAL: goog.getCssName('inputview-suggest-normal'),
  SEPARATOR: goog.getCssName('inputview-suggest-separator'),
  SHOW: goog.getCssName('inputview-suggest-show')
};


/**
 * The fields of the candidate window.
 *
 * @enum {number}
 */
CandidateWindow.Field = {
  CANDIDATES: 0,
  SETTINGS: 1,
  IGNORE: 2
};
var Field = CandidateWindow.Field;


/**
 * The candidate list element.
 *
 * @protected {!Element}
 */
CandidateWindow.prototype.candidateListElement;


/**
 * The auxilary div element.
 *
 * @protected {!Element}
 */
CandidateWindow.prototype.auxilaryElement;


/**
 * The 'settings' element.
 *
 * @private {!Element}
 */
CandidateWindow.prototype.settingsElement_;


/**
 * The 'ignore' element.
 *
 * @protected {!Element}
 */
CandidateWindow.prototype.ignoreElement;


/**
 * The highlighted candidate index.
 *
 * @protected {number}
 */
CandidateWindow.prototype.highlightIndex = -1;


/**
 * The current highlight field.
 *
 * @protected {Field}
 */
CandidateWindow.prototype.highlightField = Field.CANDIDATES;


/** @override */
CandidateWindow.prototype.createDom = function() {
  goog.base(this, 'createDom');

  var container = this.getElement();
  goog.dom.classlist.addAll(container, [Css.MENU, Css.SHOW]);
  if (!container) {
    return;
  }

  var domHelper = this.getDomHelper();
  this.candidateListElement = domHelper.createDom(goog.dom.TagName.DIV);
  domHelper.append(container, this.candidateListElement);

  if (this.hasSettingDiv_) {
    this.auxilaryElement = domHelper.createDom(goog.dom.TagName.DIV);
    domHelper.append(this.auxilaryElement, domHelper.createDom(
        goog.dom.TagName.DIV, Css.SEPARATOR));

    this.ignoreElement = domHelper.createDom(goog.dom.TagName.DIV,
        Css.MENUITEM, chrome.i18n.getMessage('ADD_TO_DICTIONARY'));
    domHelper.append(this.auxilaryElement, this.ignoreElement);

    this.settingsElement_ = domHelper.createDom(goog.dom.TagName.DIV,
        Css.MENUITEM, chrome.i18n.getMessage('SETTINGS'));
    domHelper.append(this.auxilaryElement, this.settingsElement_);

    domHelper.append(container, this.auxilaryElement);
  }

  goog.dom.classlist.add(
      this.parentWindow.contentWindow.document.body, Css.BODY);
};


/** @override */
CandidateWindow.prototype.enterDocument = function() {
  goog.base(this, 'enterDocument');

  this.getHandler().
      listen(this.candidateListElement,
          goog.events.EventType.CLICK, this.onCandidateListClicked_);
  if (this.hasSettingDiv_) {
    this.getHandler().
        listen(this.ignoreElement, goog.events.EventType.CLICK,
            this.onIgnoreDivClicked_).
        listen(this.settingsElement_, goog.events.EventType.CLICK,
            this.onSettingsDivClicked_).
        listen(this.settingsElement_,
            goog.events.EventType.MOUSEOVER,
            this.readItem);
  }
};


/**
 * Changes the candidate window position.
 *
 * @param {!BoundSize} bounds The bounds of the composition text.
 * @param {!Array.<!BoundSize>} boundsList The list of bounds of each
 *     composition character.
 * @protected
 */
CandidateWindow.prototype.changeCandidatePosition = function(
    bounds, boundsList) {
  var compositionBounds = this.getCompositionBounds(bounds, boundsList);
  this.adjustWindowPosition(compositionBounds);
};


/**
 * Adjusts the candidate window position according to the composition bounds,
 * window bounds and the candidate window size.
 *
 * @param {!BoundSize} bounds The composition bounds.
 */
CandidateWindow.prototype.adjustWindowPosition = function(bounds) {
  var winWidth = window.screen.width;
  var winHeight = window.screen.height;
  var candidateBounds = this.size();
  var leftPos = bounds.x + candidateBounds.width > winWidth ?
      winWidth - candidateBounds.width : bounds.x;
  var topPos = bounds.y + bounds.h + candidateBounds.height > winHeight ?
      bounds.y - candidateBounds.height : bounds.y + bounds.h;
  if (this.candidateListElement.children.length > 0) {
    var padding = goog.style.getPaddingBox(
        this.candidateListElement.children[0]);
    leftPos -= padding.left;
  }
  this.reposition(new goog.math.Coordinate(leftPos, topPos));
};


/**
 * Calculates the total bounds of a list of bounds.
 *
 * @param {!BoundSize} bounds The initial bounds.
 * @param {!Array.<!BoundSize>} boundsList The list of bounds.
 * @return {!BoundSize} The total bounds contains the list of bounds.
 * @protected
 */
CandidateWindow.prototype.getCompositionBounds = function(bounds, boundsList) {
  var left = bounds.x;
  var top = bounds.y;
  var right = bounds.x + bounds.w;
  var bottom = bounds.y + bounds.h;

  if (boundsList) {
    for (var i = 1; i < boundsList.length; ++i) {
      left = Math.min(left, boundsList[i].x);
      top = Math.min(top, boundsList[i].y);
      right = Math.max(right, boundsList[i].x + boundsList[i].w);
      bottom = Math.max(bottom, boundsList[i].y + boundsList[i].h);
    }
  }

  return /** @type {!BoundSize} */ (
      {x: left, y: top, w: right - left, h: bottom - top});
};


/**
 * Sets candidates.
 *
 * @param {!Array.<!Object.<{candidate: string, label: number}>>} candidates
 *     The candiates with two fields:
 *     --candidate: The target;
 *     --label: The index.
 */
CandidateWindow.prototype.setCandidates = function(candidates) {
  this.setCandidateElements(candidates);
  this.adjustWindowSize();
};


/**
 * Handles the mousedown event on candidate list.
 *
 * @param {!goog.events.BrowserEvent} e The mousedown event.
 * @private
 */
CandidateWindow.prototype.onCandidateListClicked_ = function(e) {
  var index = goog.array.findIndex(
      this.candidateListElement.childNodes, function(element) {
        return element.contains(e.target);
      });
  if (index >= 0) {
    this.dispatchEvent(new ClickCandidateEvent(index));
  }
};


/**
 * Handles the click event on ignore correct div
 *
 * @private
 */
CandidateWindow.prototype.onIgnoreDivClicked_ = function() {
  this.dispatchEvent(EventType.IGNORE_CORRECT_EVENT);
};


/**
 * Handles the mousedown event on spread div.
 *
 * @private
 */
CandidateWindow.prototype.onSettingsDivClicked_ = function() {
  var url = 'chrome://settings/languages';
  if (chrome.inputMethodPrivate &&
      chrome.inputMethodPrivate.getCurrentInputMethod) {
    chrome.inputMethodPrivate.getCurrentInputMethod(function(inputMethod) {
      var matches = inputMethod.match(/_comp_ime_([a-z]{32})(.*)/);
      if (matches) {
        url = 'chrome-extension://';
        url += matches[1];
        url += '/hmm_options.html?code=';
        url += matches[2];
      }
      chrome.tabs.create({'url': url});
    });
  } else {
    chrome.tabs.create({'url': url});
  }
};


/**
 * Sets candidate elements.
 *
 * @param {!Array.<!Object.<!{candidate: string, label: number}>>} candidates
 *     The candidates with two fields:
 *     --candidate: The target;
 *     --label: The index.
 * @protected
 */
CandidateWindow.prototype.setCandidateElements = function(candidates) {
  var domHelper = this.getDomHelper();
  var candidateElements = domHelper.getChildren(this.candidateListElement);
  goog.array.forEach(candidateElements, function(elem) {
    this.getHandler().unlisten(elem,
        goog.events.EventType.MOUSEOVER,
        this.readItem);
  }, this);
  domHelper.removeChildren(this.candidateListElement);
  candidateElements = [];
  for (var i = 0; i < candidates.length; ++i) {
    var elem = this.createCandidateElement(candidates[i]);
    candidateElements.push(elem);
    this.getHandler().listen(elem,
        goog.events.EventType.MOUSEOVER,
        this.readItem);
  }
  domHelper.append(this.candidateListElement, candidateElements);
};


/**
 * Adjusts the window size to contain all the elements.
 *
 * @protected
 */
CandidateWindow.prototype.adjustWindowSize = function() {
  var size = goog.style.getSize(this.getElement());
  this.resize(size.width, size.height);
};


/**
 * Creates a candidate element from the candidate content.
 *
 * @param {!Object.<!{candidate: string, label: number}>} candidate
 *     The candidate object.
 * @return {!Element} The candidate element.
 * @protected
 */
CandidateWindow.prototype.createCandidateElement = function(candidate) {
  var label = candidate.label ? candidate.label + '. ' : '';
  return this.getDomHelper().createDom(goog.dom.TagName.DIV,
      Css.MENUITEM, label + candidate.candidate);
};


/**
 * Sets cursor position.
 *
 * @param {number} cursorPos The highlighted index.
 */
CandidateWindow.prototype.setCursorPosition = function(cursorPos) {
  var candidateElements = goog.dom.getChildren(this.candidateListElement);
  this.moveHighlightElement_(candidateElements[this.highlightIndex],
      candidateElements[cursorPos], cursorPos < candidateElements.length);
  this.highlightIndex = cursorPos;
  if (cursorPos < candidateElements.length) {
    this.highlightField = Field.CANDIDATES;
  }
};


/**
 * Moves the highlight field up.
 *
 * @return {number} -1 if the highlight field is not a candidate, other wise
 * returns the relative index of the candidate among the showing candidates.
 */
CandidateWindow.prototype.moveUpHighlight = function() {
  var candidateElements = goog.dom.getChildren(this.candidateListElement);
  switch (this.highlightField) {
    case Field.CANDIDATES:
      this.moveHighlightElement_(candidateElements[this.highlightIndex],
          candidateElements[this.highlightIndex - 1]);
      this.highlightIndex = this.highlightIndex > 0 ?
          this.highlightIndex - 1 : 0;
      return this.highlightIndex;
    case Field.IGNORE:
      this.moveHighlightElement_(this.ignoreElement,
          candidateElements[candidateElements.length - 1]);
      return candidateElements.length - 1;
    case Field.SETTINGS:
      if (goog.style.isElementShown(this.ignoreElement)) {
        this.highlightField = Field.IGNORE;
        this.moveHighlightElement_(this.settingsElement_, this.ignoreElement,
            true);
      } else {
        this.moveHighlightElement_(this.settingsElement_,
            candidateElements[candidateElements.length - 1]);
        return candidateElements.length - 1;
      }
      break;
    default:
      break;
  }
  return -1;
};


/**
 * Moves the highlight field down.
 *
 * @return {number} -1 if the highlight field is not a candidate, other wise
 * returns the relative index of the candidate among the showing candidates.
 */
CandidateWindow.prototype.moveDownHighlight = function() {
  switch (this.highlightField) {
    case Field.CANDIDATES:
      var candidateElements = goog.dom.getChildren(this.candidateListElement);
      if (this.highlightIndex < candidateElements.length - 1) {
        this.moveHighlightElement_(candidateElements[this.highlightIndex],
            candidateElements[this.highlightIndex + 1]);
        this.highlightIndex = this.highlightIndex + 1;
        return this.highlightIndex;
      }
      if (this.hasSettingDiv_) {
        if (goog.style.isElementShown(this.ignoreElement)) {
          this.highlightField = Field.IGNORE;
          this.moveHighlightElement_(candidateElements[this.highlightIndex],
              this.ignoreElement, true);
          this.highlightIndex = -1;
        } else {
          this.highlightField = Field.SETTINGS;
          this.moveHighlightElement_(candidateElements[this.highlightIndex],
              this.settingsElement_, true);
        }
      }
      break;
    case Field.IGNORE:
      this.highlightField = Field.SETTINGS;
      this.moveHighlightElement_(this.ignoreElement, this.settingsElement_,
          true);
    case Field.SETTINGS:
      break;
    default:
      break;
  }
  return -1;
};


/**
 * Handles the commit event. If the window doesn't has setting DIV, means it's
 * ignore window, will fire a click candidate event. If it has setting DIV,
 * only handle event on setting DIV.
 *
 * @return {boolean} True if the event is handled.
 */
CandidateWindow.prototype.handleCommitEvent = function() {
  switch (this.highlightField) {
    case Field.IGNORE:
      this.onIgnoreDivClicked_();
      return true;
    case Field.SETTINGS:
      this.onSettingsDivClicked_();
      return true;
    default:
      if (!this.hasSettingDiv_ && this.highlightIndex >= 0) {
        this.dispatchEvent(new ClickCandidateEvent(this.highlightIndex));
        return true;
      }
      break;
  }
  return false;
};


/** @override */
CandidateWindow.prototype.setVisible = function(isVisible) {
  if (!isVisible) {
    if (this.settingsElement_) {
      goog.dom.classlist.remove(this.ignoreElement, Css.HIGHLIGHT);
      goog.dom.classlist.remove(this.settingsElement_, Css.HIGHLIGHT);
    }
    this.highlightField = Field.CANDIDATES;
    this.highlightIndex = -1;
  }
  var ret = goog.base(this, 'setVisible', isVisible);
  if (isVisible) {
    this.changeCandidatePosition(this.env.currentBounds,
        this.env.currentBoundsList);
  }
  return ret;
};


/**
 * Reads the touched or highlighted item.
 *
 * @param {!goog.events.BrowserEvent|!Element} e .
 * @param {boolean=} opt_force .
 * @protected
 */
CandidateWindow.prototype.readItem = function(e, opt_force) {
  if (this.env.isChromeVoxOn &&
      (opt_force || !e.currentTarget.contains(e.relatedTarget))) {
    var elem = /** @type {!Element} */ (e.currentTarget || e);
    var text = goog.dom.getTextContent(elem) ||
        elem.getAttribute('aria-label');
    // Make compatible with old Chrome OS version.
    chrome.tts && chrome.tts.speak(text);
  }
};


/**
 * Move the highlight css.
 *
 * @param {Element} from .
 * @param {Element} to .
 * @param {boolean=} opt_read .
 * @private
 */
CandidateWindow.prototype.moveHighlightElement_ = function(from, to, opt_read) {
  if (from) {
    goog.dom.classlist.remove(from, Css.HIGHLIGHT);
  }

  if (to) {
    goog.dom.classlist.add(to, Css.HIGHLIGHT);
    if (opt_read) {
      this.readItem(to, true);
    }
  }
};


/**
 * Sets the correct element shown.
 *
 * @param {boolean} visible .
 */
CandidateWindow.prototype.setCorrectElmentShown = function(visible) {
  goog.style.setElementShown(this.ignoreElement, visible);
};

});  // goog.scope
