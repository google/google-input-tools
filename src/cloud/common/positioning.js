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
 * @fileoverview This file contains some common util functions for positioning.
 */

goog.provide('i18n.input.common.positioning');

goog.require('goog.dom');


/**
 * The pad between the menu and the anchor element.
 * TODO(fengyuan): Makes this interface configurable in statusbar.
 *
 * @type {number}
 * @private
 */
i18n.input.common.positioning.POSITION_PAD_ = 50;


/**
 * Positions element to the bottom right of the page.
 *
 * @param {Element} element Element to be positioned to the bottom
 *     right of the page.
 */
i18n.input.common.positioning.goToBottomRight = function(element) {
  if (element) {
    var viewPortSize = goog.dom.getDomHelper(element).getViewportSize();
    var width = element.offsetWidth;
    var height = element.offsetHeight;
    element.style.left = Math.floor(
        viewPortSize.width - width -
        i18n.input.common.positioning.POSITION_PAD_) + 'px';
    element.style.top = Math.floor(
        viewPortSize.height - height -
        i18n.input.common.positioning.POSITION_PAD_) + 'px';
  }
};
