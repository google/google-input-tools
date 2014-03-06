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
 * @fileoverview Defines the Inputable interface.
 */


goog.provide('i18n.input.common.Inputable');



/**
 * Inputable interface defines the abstracted operations to an input box which
 * are used by ITA plugins or the keyboard controller.
 *
 * @interface
 */
i18n.input.common.Inputable = function() {};


/**
 * Gets the text before the current cursor.
 *
 * @param {number} maxLength The max length of text to get before cursor.
 * @return {string} The text before cursor.
 */
i18n.input.common.Inputable.prototype.getTextBeforeCursor = goog.abstractMethod;


/**
 * Commits the given text to the input box. Either inert at the cursor or
 * replace the current selection.
 *
 * @param {string} text The text to commit to the input box.
 * @param {number=} opt_back The number of chars to be removed before the
 *     cursor.
 */
i18n.input.common.Inputable.prototype.commitText = goog.abstractMethod;


/**
 * Gets the cursor position.
 *
 * @return {goog.positioning.AbstractPosition} The position of the cursor.
 */
i18n.input.common.Inputable.prototype.getCursorPosition = goog.abstractMethod;


/**
 * Sets focus to the input box.
 */
i18n.input.common.Inputable.prototype.setFocus = goog.abstractMethod;


/**
 * Gets whether the selection range in the input box is collapsed.
 *
 * @return {boolean} Whether the selection range is collapsed.
 */
i18n.input.common.Inputable.prototype.isRangeCollapsed = goog.abstractMethod;


/**
 * Creates a token with a range whose end point is current cursor and start
 * point is a separator. If currently has no cursor (a.k.a, selection
 * range), returns the token for the selection.
 *
 * @return {i18n.input.common.Token} The created token.
 */
i18n.input.common.Inputable.prototype.createToken = goog.abstractMethod;
