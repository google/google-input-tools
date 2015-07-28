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
goog.provide('i18n.input.chrome.MarkWindow');

goog.require('goog.Timer');
goog.require('goog.dom.classlist');
goog.require('goog.math.Coordinate');
goog.require('goog.style');
goog.require('i18n.input.chrome.FloatingWindow');
goog.require('i18n.input.chrome.floatingwindow.Css');

goog.scope(function() {
var Css = i18n.input.chrome.floatingwindow.Css;
var FloatingWindow = i18n.input.chrome.FloatingWindow;



/**
 * The mark window in Chrome OS.
 *
 * @param {!chrome.app.window.AppWindow} parentWindow The parent app window.
 * @constructor
 * @extends {i18n.input.chrome.FloatingWindow}
 */
i18n.input.chrome.MarkWindow = function(parentWindow) {
  i18n.input.chrome.MarkWindow.base(this, 'constructor', parentWindow, true);

  /** @private {!Function} */
  this.compositionBoundsChangedHandler_ =
      this.changeCandidatePosition.bind(this);

  if (chrome.inputMethodPrivate &&
      chrome.inputMethodPrivate.onCompositionBoundsChanged) {
    chrome.inputMethodPrivate.onCompositionBoundsChanged.addListener(
        this.compositionBoundsChangedHandler_);
  }
};
var MarkWindow = i18n.input.chrome.MarkWindow;
goog.inherits(MarkWindow, FloatingWindow);


/**
 * Mark window display max time in ms.
 *
 * @private {number}
 */
MarkWindow.SHOW_MAX_TIME_ = 750;


/**
 * A space width in px.
 *
 * @private {number}
 */
MarkWindow.SPACE_WIDTH_ = 4;


/**
 * Whether the mark window is pinned or not.
 *
 * @private {boolean}
 */
MarkWindow.prototype.pinned_ = false;


/** @private {boolean} */
MarkWindow.prototype.hasExtraSpace_ = false;


/** @private {BoundSize} */
MarkWindow.prototype.position_ = null;


/** @private {number} */
MarkWindow.prototype.lastWidth_ = 0;


/** @private {number} */
MarkWindow.prototype.delayTimer_ = 0;


/** @private {boolean} */
MarkWindow.prototype.isReverted_ = false;


/** @private {!BoundSize} */
MarkWindow.prototype.lastShowedPosition_;


/** @override */
MarkWindow.prototype.createDom = function() {
  goog.base(this, 'createDom');
  var elem = this.getElement();
  goog.dom.classlist.add(elem, Css.MARK_WINDOW);
  goog.dom.classlist.add(
      this.parentWindow.contentWindow.document.body, Css.BODY);
};


/**
 * Changes the mark window position.
 *
 * @param {!BoundSize} bounds The bounds of the composition text.
 * @param {!Array.<!BoundSize>} boundsList The list of bounds of each
 *     composition character.
 * @protected
 */
MarkWindow.prototype.changeCandidatePosition = function(
    bounds, boundsList) {
  var position = this.getCompositionBounds(bounds, boundsList);
  if (this.pinned_) {
    this.show(position);
  }
  this.position_ = position;
};


/**
 * Shows window on last position.
 *
 * @param {!BoundSize=} opt_position .
 */
MarkWindow.prototype.show = function(opt_position) {
  this.pinned_ = false;
  var position = opt_position || this.position_;
  // When position is different from previous position, means
  // correction text is in different line with original text.
  // In this case, we can't get the correct position to show
  // mark window, so don't show it.
  if (!position || position.y != this.position_.y) {
    this.isReverted_ = false;
    return;
  }
  goog.Timer.clear(this.delayTimer_);
  var width;
  if (this.isReverted_) {
    this.position_ = position;
    this.position_.w = width = this.lastWidth_;
  } else {
    width = Math.max(this.position_.w, position.x - this.position_.x);
    if (this.hasExtraSpace_) {
      // Remove the space width.
      width -= MarkWindow.SPACE_WIDTH_;
    }
  }

  this.lastWidth_ = this.position_.w;
  // Only show mark window on revert.
  if (this.isReverted_) {
    goog.style.setWidth(this.getElement(), width);
    this.resize(width, 2);
    this.adjustWindowPosition(this.position_);
    this.lastShowedPosition_ = this.position_;
    this.setVisible(true);
    this.delayTimer_ = goog.Timer.callOnce(
        this.setVisible.bind(this, false), MarkWindow.SHOW_MAX_TIME_);
  }
  this.isReverted_ = false;
};


/**
 * Adjusts the mark window position according to the composition bounds,
 * window bounds and the candidate window size.
 *
 * @param {!BoundSize} bounds The composition bounds.
 * @protected
 */
MarkWindow.prototype.adjustWindowPosition = function(bounds) {
  var winWidth = window.screen.width;
  var winHeight = window.screen.height;
  var candidateBounds = this.size();
  var leftPos = bounds.x + candidateBounds.width > winWidth ?
      winWidth - candidateBounds.width : bounds.x;
  var topPos = bounds.y + bounds.h + candidateBounds.height > winHeight ?
      bounds.y - candidateBounds.height : bounds.y + bounds.h;

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
MarkWindow.prototype.getCompositionBounds = function(bounds, boundsList) {
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
 * Set the pinned property.
 *
 * @param {boolean} pinned .
 * @param {boolean=} opt_hasExtraSpace .
 */
MarkWindow.prototype.setPinned = function(pinned, opt_hasExtraSpace) {
  this.pinned_ = pinned;
  this.hasExtraSpace_ = !!opt_hasExtraSpace;
};


/**
 * Sets the revert flag.
 *
 * @param {boolean} isReverted .
 */
MarkWindow.prototype.setReverted = function(isReverted) {
  this.isReverted_ = isReverted;
};


/**
 * Gets the last showed position.
 *
 * @return {!BoundSize} The position.
 */
MarkWindow.prototype.getLastShowedPosition = function() {
  return this.lastShowedPosition_;
};
});  // goog.scope

