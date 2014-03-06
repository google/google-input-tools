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
 * @fileoverview Drag Utilities allow target and handle cross iframe.
 */

goog.provide('i18n.input.common.Dragger');

goog.require('goog.asserts');
goog.require('goog.dom');
goog.require('goog.events.Event');
goog.require('goog.events.EventType');
goog.require('goog.fx.Dragger');
goog.require('goog.userAgent');



/**
 * Inherits from goog.fx.Dragger. It allow target and handle element cross
 * different iframe.
 *
 * @param {Element} target The element that will be dragged.
 * @param {Element=} opt_handle An optional handle to control the drag, if null
 *     the target is used.
 * @param {goog.math.Rect=} opt_limits Object containing left, top, width,
 *     and height.
 *
 * @extends {goog.fx.Dragger}
 * @constructor
 * @suppress {visibility}
 */
i18n.input.common.Dragger = function(target, opt_handle, opt_limits) {
  goog.base(this, target, opt_handle, opt_limits);
  // Not allow target and handle cross multiple iframe layers.
  if (opt_handle && this.document_ != goog.dom.getOwnerDocument(opt_handle)) {
    var win = goog.dom.getWindow(goog.dom.getOwnerDocument(opt_handle));
    goog.asserts.assert(goog.dom.getWindow(this.document_) == win.parent);
    /**
     * The iframe container element.
     *
     * @type {Element}
     * @private
     */
    this.frameElement_ = win.frameElement;
  }
};
goog.inherits(i18n.input.common.Dragger, goog.fx.Dragger);


/** @override */
i18n.input.common.Dragger.prototype.startDrag = function(e) {
  this.fixClientPositon_(e);
  goog.base(this, 'startDrag', e);
};


/**
 * @override
 * @suppress {visibility}
 */
i18n.input.common.Dragger.prototype.setupDragHandlers = function() {
  this.setupDragHandlersOnDocument_(this.document_);
  var doc = goog.dom.getOwnerDocument(this.handle);
  // If target and handle in different documents, need to set up handlers on
  // the additional document.
  if (doc != this.document_) {
    this.setupDragHandlersOnDocument_(doc);
  }

  if (this.scrollTarget_) {
    this.eventHandler_.listen(this.scrollTarget_, goog.events.EventType.SCROLL,
        this.onScroll_, !goog.fx.Dragger.HAS_SET_CAPTURE_);
  }
};


/**
 * Sets up event handlers on given document.
 *
 * @param {Document} doc The document to set up handlers.
 * @suppress {visibility}
 * @private
 */
i18n.input.common.Dragger.prototype.setupDragHandlersOnDocument_ =
    function(doc) {
  var docEl = doc.documentElement;
  // Use bubbling when we have setCapture since we got reports that IE has
  // problems with the capturing events in combination with setCapture.
  var useCapture = !goog.fx.Dragger.HAS_SET_CAPTURE_;

  this.eventHandler_.listen(doc,
      [goog.events.EventType.TOUCHMOVE, goog.events.EventType.MOUSEMOVE],
      this.handleMoveOverride_, useCapture);
  this.eventHandler_.listen(doc,
      [goog.events.EventType.TOUCHEND, goog.events.EventType.MOUSEUP],
      this.endDrag, useCapture);

  if (goog.fx.Dragger.HAS_SET_CAPTURE_) {
    docEl.setCapture(false);
    this.eventHandler_.listen(docEl,
                              goog.events.EventType.LOSECAPTURE,
                              this.endDrag);
  } else {
    // Make sure we stop the dragging if the window loses focus.
    // Don't use capture in this listener because we only want to end the drag
    // if the actual window loses focus. Since blur events do not bubble we use
    // a bubbling listener on the window.
    this.eventHandler_.listen(goog.dom.getWindow(doc),
                              goog.events.EventType.BLUR,
                              this.endDrag);
  }

  if (goog.userAgent.IE && this.ieDragStartCancellingOn_) {
    // Cancel IE's 'ondragstart' event.
    this.eventHandler_.listen(doc, goog.events.EventType.DRAGSTART,
                              goog.events.Event.preventDefault);
  }
};


/**
 * Event handler that is used on mouse / touch move to update the drag. Have to
 * change original name 'handleMove_' to 'handleMoveOverride_'. Without
 * '@override' annotation will cause a compliant error. However, with the
 * annotation will cause a lint error which not allow override a private
 * member.
 *
 * @param {goog.events.BrowserEvent} e Event object.
 * @suppress {visibility}
 * @private
 */
i18n.input.common.Dragger.prototype.handleMoveOverride_ = function(e) {
  this.fixClientPositon_(e);
  this.handleMove_(e);
};


/**
 * Fixes the event clientX/Y relative to the target element document.
 *
 * @param {goog.events.BrowserEvent} e The event.
 * @suppress {visibility}
 * @private
 */
i18n.input.common.Dragger.prototype.fixClientPositon_ = function(e) {
  if (goog.dom.getOwnerDocument(e.target) != this.document_ &&
      this.frameElement_) {
    e.clientX += this.frameElement_.offsetLeft;
    e.clientY += this.frameElement_.offsetTop;
  }
};
