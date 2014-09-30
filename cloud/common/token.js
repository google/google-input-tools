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
 * @fileoverview Description of this file.
 */

goog.provide('i18n.input.common.Token');



/**
 * The token interface defines the abstracted operations to an range of text in
 * an input box which are used by inline mode IME.
 *
 * @interface
 */
i18n.input.common.Token = function() {};


/**
 * Gets the text in this token.
 *
 * @return {string} The text in this token.
 */
i18n.input.common.Token.prototype.getText = goog.abstractMethod;


/**
 * Gets the position of this token.
 *
 * @return {goog.positioning.AbstractPosition} The position of bottom-start
 *     corner.
 */
i18n.input.common.Token.prototype.getPosition = goog.abstractMethod;


/**
 * Replaces the token's text to the given text.
 *
 * @param {string} text The new text.
 * @param {boolean=} opt_force Whether forcly replace the text even if the
 *     token's text equals the replacing text. This can help to refresh the
 *     caret to the end of token.
 */
i18n.input.common.Token.prototype.replace = goog.abstractMethod;


/**
 * Extends backwards from the cursor to a separator char in the input box.
 *
 * @param {!RegExp} separatorRegex The regexp for text separators.
 * @param {number} maxLength The max length of the characters to extend.
 * @param {boolean=} opt_forward Whether extend this token in both backward and
 *     forward directions. If not specified, extend backward only.
 */
i18n.input.common.Token.prototype.extend = goog.abstractMethod;


/**
 * Gets whether the token is still valid. It will try to match the text content
 * in the input box according to the token's range with the token's text.
 *
 * @return {boolean} Whether is valid.
 */
i18n.input.common.Token.prototype.isValid = goog.abstractMethod;


/**
 * Gets whether the token's start and end points are collapsed.
 *
 * @return {boolean} Whether is collapsed.
 */
i18n.input.common.Token.prototype.isCollapsed = goog.abstractMethod;


/**
 * Gets whether the token contains the browser cursor.
 *
 * @return {boolean} Whether the token contains the browser cursor.
 */
i18n.input.common.Token.prototype.containsCursor = goog.abstractMethod;


/**
 * Disposes this token.
 */
i18n.input.common.Token.prototype.dispose = goog.abstractMethod;
