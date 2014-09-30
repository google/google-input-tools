// Copyright 2014 The Cloud Input Tools Authors. All Rights Reserved.
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
 * @fileoverview Defines the constrained dragger.
 */

goog.provide('i18n.input.common.ConstrainedDragger');

goog.require('goog.dom');
goog.require('goog.dom.ViewportSizeMonitor');
goog.require('goog.events.EventHandler');
goog.require('goog.events.EventType');
goog.require('goog.math.Box');
goog.require('goog.math.Rect');
goog.require('goog.style');
goog.require('i18n.input.common.Dragger');



/**
 * The inputable implementation for elements.
 *
 * @param {!Element} target The element that will be dragged.
 * @param {Element=} opt_handle An optional handle to control the drag, if null
 *     the target is used.
 * @constructor
 * @extends {i18n.input.common.Dragger}
 */
i18n.input.common.ConstrainedDragger = function(target, opt_handle) {
  goog.base(this, target, opt_handle);

  /**
   * The viewport size monitor.
   *
   * @type {!goog.dom.ViewportSizeMonitor}
   * @private
   */
  this.viewportSizeMonitor_ = new goog.dom.ViewportSizeMonitor(
      goog.dom.getDomHelper(target).getWindow());

  var eventHandler = new goog.events.EventHandler(this);
  eventHandler.listen(this.viewportSizeMonitor_,
      goog.events.EventType.RESIZE, this.handleResize_);
  this.registerDisposable(eventHandler);
};
goog.inherits(i18n.input.common.ConstrainedDragger, i18n.input.common.Dragger);


/**
 * The padding of the dragging boundary.
 *
 * @type {number}
 * @private
 */
i18n.input.common.ConstrainedDragger.DRAG_PADDING_ = 2;


/**
 * Handles the window resize event.
 *
 * @private
 */
i18n.input.common.ConstrainedDragger.prototype.handleResize_ = function() {
  if (this.target.style.display.toLowerCase() != 'none') {
    this.repositionTarget();
  }
};


/**
 * Gets the dragging boundary.
 *
 * @return {goog.math.Box} The dragging boundary.
 */
i18n.input.common.ConstrainedDragger.prototype.getBoundary = function() {
  var padding = i18n.input.common.ConstrainedDragger.DRAG_PADDING_;
  var size = goog.style.getSize(this.target);
  var bound = this.viewportSizeMonitor_.getSize();
  bound.width -= padding + size.width;
  bound.height -= padding + size.height;

  return new goog.math.Box(padding, bound.width, bound.height, padding);
};


/**
 * Repositions the target element into boundary.
 *
 * @param {!goog.math.Coordinate=} opt_pos The coordinate to reposition. If not
 *     specified, this method will fit the current target into view.
 */
i18n.input.common.ConstrainedDragger.prototype.repositionTarget =
    function(opt_pos) {
  var bound = this.getBoundary();
  this.setLimits(goog.math.Rect.createFromBox(bound));

  var pos = opt_pos || goog.style.getClientPosition(this.target);
  pos.x = Math.min(pos.x, bound.right);
  pos.y = Math.min(pos.y, bound.bottom);
  pos.x = Math.max(pos.x, bound.left);
  pos.y = Math.max(pos.y, bound.top);

  goog.style.setPosition(this.target, pos);
};


/** @override */
i18n.input.common.ConstrainedDragger.prototype.disposeInternal = function() {
  goog.dispose(this.viewportSizeMonitor_);

  goog.base(this, 'disposeInternal');
};
