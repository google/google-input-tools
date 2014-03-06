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
 * @fileoverview Handler detecting hover and makes according css changes.
 *
 */

goog.provide('i18n.input.common.HoverHandler');

goog.require('goog.Disposable');
goog.require('goog.dom.classes');
goog.require('goog.events.EventHandler');
goog.require('goog.events.EventType');



/**
 * A class simulates the :hover effect, because :hover is not working
 * efficiently in many browsers.
 *
 * @constructor
 * @extends {goog.Disposable}
 */
i18n.input.common.HoverHandler = function() {

  /**
   * Event handler.
   *
   * @type {goog.events.EventHandler}
   * @private
   */
  this.eventHandler_ = new goog.events.EventHandler(this);
};
goog.inherits(i18n.input.common.HoverHandler, goog.Disposable);


/**
 * Adds hover listener.
 *
 * @param {Element} element Element to add hover listener.
 * @param {!string} className Class name for the element. When the element is
 *     hoverred, we will add className + '-hover' to it.
 */
i18n.input.common.HoverHandler.prototype.addHover =
    function(element, className) {
  if (element) {
    this.eventHandler_.listen(element, goog.events.EventType.MOUSEOVER,
        goog.bind(this.addHoverClass_, this, element, className));
    this.eventHandler_.listen(element, goog.events.EventType.MOUSEOUT,
        goog.bind(this.removeHoverClass_, this, element, className));
  }
};


/**
 * Adds hover css class.
 *
 * @param {!Element} element Element to add hover listener.
 * @param {!string} className Class name for the element. When the element is
 *     hoverred, we will add className + '-hover' to it.
 * @private
 */
i18n.input.common.HoverHandler.prototype.addHoverClass_ = function(element,
    className) {
  goog.dom.classes.add(element, className);
};


/**
 * Removes hover css class.
 *
 * @param {!Element} element Element to add hover listener.
 * @param {!string} className Class name for the element. When the element is
 *     hoverred, we will add className + '-hover' to it.
 * @private
 */
i18n.input.common.HoverHandler.prototype.removeHoverClass_ = function(element,
    className) {
  goog.dom.classes.remove(element, className);
};


/**
 * Removes hover listener.
 *
 * @param {!Element} element Element to add hover listener.
 * @param {!string} className Class name for the element. When the element is
 *     hoverred, we will add className + '-hover' to it.
 */
i18n.input.common.HoverHandler.prototype.removeHover = function(element,
    className) {
  if (element) {
    this.eventHandler_.unlisten(element, goog.events.EventType.MOUSEOVER,
        goog.bind(this.addHoverClass_, null, element, className));
    this.eventHandler_.unlisten(element, goog.events.EventType.MOUSEOUT,
        goog.bind(this.removeHoverClass_, null, element, className));
  }
};


/** @override */
i18n.input.common.HoverHandler.prototype.disposeInternal = function() {
  goog.dispose(this.eventHandler_);
};
