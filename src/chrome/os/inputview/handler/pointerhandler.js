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
goog.provide('i18n.input.chrome.inputview.handler.PointerHandler');

goog.require('goog.events.EventHandler');
goog.require('goog.events.EventTarget');
goog.require('goog.events.EventType');
goog.require('goog.math.Coordinate');
goog.require('i18n.input.chrome.inputview.handler.PointerActionBundle');

goog.scope(function() {



/**
 * The pointer controller.
 *
 * @constructor
 * @extends {goog.events.EventTarget}
 */
i18n.input.chrome.inputview.handler.PointerHandler = function() {
  goog.base(this);

  /**
   * The pointer handlers.
   *
   * @type {!Object.<number, !i18n.input.chrome.inputview.handler.PointerActionBundle>}
   * @private
   */
  this.pointerActionBundles_ = {};

  /**
   * The event handler.
   *
   * @type {!goog.events.EventHandler}
   * @private
   */
  this.eventHandler_ = new goog.events.EventHandler(this);

  this.eventHandler_.
      listen(document, [goog.events.EventType.MOUSEDOWN,
        goog.events.EventType.TOUCHSTART], this.onPointerDown_, true).
      listen(document, [goog.events.EventType.MOUSEUP,
        goog.events.EventType.TOUCHEND], this.onPointerUp_, true).
      listen(document, goog.events.EventType.TOUCHMOVE, this.onTouchMove_,
          true);
};
goog.inherits(i18n.input.chrome.inputview.handler.PointerHandler,
    goog.events.EventTarget);
var PointerHandler = i18n.input.chrome.inputview.handler.PointerHandler;


/**
 * The canvas class name.
 * @const {string}
 * @private
 */
PointerHandler.CANVAS_CLASS_NAME_ = 'ita-hwt-canvas';


/**
 * Creates a new pointer handler.
 *
 * @param {!Node} target .
 * @return {!i18n.input.chrome.inputview.handler.PointerActionBundle} .
 * @private
 */
PointerHandler.prototype.createPointerHandler_ = function(target) {
  var uid = goog.getUid(target);
  if (!this.pointerActionBundles_[uid]) {
    this.pointerActionBundles_[uid] = new i18n.input.chrome.inputview.handler.
        PointerActionBundle(target, this);
  }
  return this.pointerActionBundles_[uid];
};


/**
 * Callback for mouse/touch down on the target.
 *
 * @param {!goog.events.BrowserEvent} e The event.
 * @private
 */
PointerHandler.prototype.onPointerDown_ = function(e) {
  if (!this.isOnCanvas_(e)) {
    var pointerHandler = this.createPointerHandler_(
        /** @type {!Node} */ (e.target));
    pointerHandler.handlePointerDown(e);
  }
};


/**
 * Callback for pointer out.
 *
 * @param {!goog.events.BrowserEvent} e The event.
 * @private
 */
PointerHandler.prototype.onPointerUp_ = function(e) {
  if (!this.isOnCanvas_(e)) {
    var uid = goog.getUid(e.target);
    var pointerHandler = this.pointerActionBundles_[uid];
    if (pointerHandler) {
      pointerHandler.handlePointerUp(e);
    }
  }
};


/**
 * Callback for touchmove.
 *
 * @param {!goog.events.BrowserEvent} e The event.
 * @private
 */
PointerHandler.prototype.onTouchMove_ = function(e) {
  if (!this.isOnCanvas_(e)) {
    var touches = e.getBrowserEvent()['touches'];
    if (!touches || touches.length == 0) {
      return;
    }
    for (var i = 0; i < touches.length; i++) {
      var uid = goog.getUid(touches[i].target);
      var pointerHandler = this.pointerActionBundles_[uid];
      if (pointerHandler) {
        pointerHandler.handleTouchMove(touches[i]);
      }
    }

    e.stopPropagation();
    e.preventDefault();
  }
};


/** @override */
PointerHandler.prototype.disposeInternal = function() {
  for (var bundle in this.pointerActionBundles_) {
    goog.dispose(bundle);
  }
  goog.dispose(this.eventHandler_);

  goog.base(this, 'disposeInternal');
};


/**
 * Checks whether the current event target element is on cavas element.
 *
 * @param {!goog.events.BrowserEvent} e .
 * @private
 */
PointerHandler.prototype.isOnCanvas_ = function(e) {
  return e.target.className == PointerHandler.CANVAS_CLASS_NAME_;
};
});  // goog.scope
