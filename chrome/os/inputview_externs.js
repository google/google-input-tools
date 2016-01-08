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
var xkb = {};


/** @const */
chrome.metricsPrivate = {};


/**
 * @param {!Object} metrics The metrics object.
 * @param {number} value The count value to be recorded.
 */
chrome.metricsPrivate.recordValue = function(metrics, value) {};



/**
 * @param {number} num .
 * @param {Function} candidatesCallback .
 * @param {Function} gestureCallback .
 * @constructor
 */
xkb.Model = function(num, candidatesCallback, gestureCallback) {};


/**
 * @param {!Array.<!Object>} gestureData .
 * @param {string} precedingText .
 */
xkb.Model.prototype.decodeGesture = function(gestureData, precedingText) {};


/**
 * @param {string} language .
 */
xkb.Model.prototype.setLanguage = function(language) {};


/**
 * @param {string} query .
 * @param {string} context .
 * @param {!Object=} opt_spatialData .
 */
xkb.Model.prototype.sendCompletionRequest = function(query, context,
    opt_spatialData) {};


/**
 * @param {string} word .
 * @param {string} context .
 */
xkb.Model.prototype.sendCompletionRequestForWord = function(word,
    context) {};


/**
 * @param {?Object} keyboardLayout .
 */
xkb.Model.prototype.sendKeyboardLayout = function(keyboardLayout) {};


/**
 * @param {string} context .
 */
xkb.Model.prototype.sendPredictionRequest = function(context) {};


/**
 * @param {number} level .
 */
xkb.Model.prototype.setCorrectionLevel = function(level) {};


/**
 * @param {boolean} enabled .
 */
xkb.Model.prototype.setEnableUserDict = function(enabled) {};


/**
 * @return {boolean} .
 */
xkb.Model.prototype.isReady = function() {return false};


/** Clears the data source. */
xkb.Model.prototype.clear = function() {};


/**
 * @param {string} word .
 * @param {number} frequency .
 */
xkb.Model.prototype.changeWordFrequency = function(word, frequency) {};


/**
 * @param {!Object} keyData .
 */
xkb.handleSendKeyEvent = function(keyData) {};


var inputview = {};


/**
 * @param {!Function} callback .
 */
inputview.getKeyboardConfig = function(callback) {};


/**
 * @param {!Function} callback .
 */
inputview.getInputMethodConfig = function(callback) {};


/**
 * @param {!Function} callback .
 */
inputview.getInputMethods = function(callback) {};


/**
 * @param {!Function} callback .
 */
inputview.getDisplayInInches = function(callback) {};


/**
 * @param {string} inputMethodId .
 */
inputview.switchToInputMethod = function(inputMethodId) {};


/**
 * Opens the settings page.
 */
inputview.openSettings = function() {};


/**
 * Sets the virtual keyboard mode.
 * @param {string} mode .
 */
inputview.setMode = function(mode) {};


/**
 * @param {string} url URL to create.
 * @param {!Object=} opt_options The options for
 *     the new window.
 * @param {function(!chrome.app.window.AppWindow)=} opt_createWindowCallback
 *     Callback to be run.
 */
inputview.createWindow = function(
    url, opt_options, opt_createWindowCallback) {};


var accents = {};


/**
 * Gets the highlighted character.
 *
 * @return {string} The character.
 */
accents.highlightedAccent = function() {
  return '';
};


/**
 * Highlights the item according to the current coordinate of the finger.
 *
 * @param {number} x The x position of finger in screen coordinate system.
 * @param {number} y The y position of finger in screen coordinate system.
 */
accents.highlightItem = function(x, y) {};


/**
 * Sets the accents which this window should display.
 *
 * @param {!Array.<string>} accents .
 */
accents.setAccents = function(accents) {};


var gesturePreview = {};


/**
 * Sets the gesture preview word which this window will display.
 *
 * @param {!string} word The word to display.
 */
gesturePreview.setPreviewWord = function(word) {};
