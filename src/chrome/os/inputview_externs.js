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

/**
 * @param {number} num .
 * @param {Function} callback .
 * @constructor
 */
xkb.DataSource = function(num, callback) {};


/**
 * @param {string} language .
 */
xkb.DataSource.prototype.setLanguage = function(language) {};


/**
 * @param {string} query .
 * @param {!Object=} opt_spatialData .
 */
xkb.DataSource.prototype.sendCompletionRequest = function(query,
    opt_spatialData) {};


/**
 * @param {string} query .
 */
xkb.DataSource.prototype.sendPredictionRequest = function(query) {};


/**
 * @param {number} level .
 */
xkb.DataSource.prototype.setCorrectionLevel = function(level) {};


/**
 * @return {boolean} .
 */
xkb.DataSource.prototype.isReady = function() {return false};


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
inputview.getInputMethods = function(callback) {};


/**
 * @param {string} inputMethodId .
 */
inputview.switchToInputMethod = function(inputMethodId) {};


/**
 * Opens the settings page.
 */
inputview.openSettings = function() {};

