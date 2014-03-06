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
 * @fileoverview The definition of layout event.
 *
 */

goog.provide('i18n.input.keyboard.LayoutEvent');

goog.require('goog.events.Event');



/**
 * The Layout Event.
 *
 * @param {i18n.input.keyboard.EventType} type The event type.
 * @param {Object} layoutView The layoutView object. See ParsedLayout for
 *     details.
 * @constructor
 * @extends {goog.events.Event}
 */
i18n.input.keyboard.LayoutEvent = function(type, layoutView) {
  goog.base(this, type);

  /**
   * The layout view object.
   *
   * @type {Object}
   */
  this.layoutView = layoutView;

  /**
   * The layout code.
   *
   * @type {?string}
   */
  this.layoutCode = layoutView ? layoutView.id : null;
};
goog.inherits(i18n.input.keyboard.LayoutEvent, goog.events.Event);
