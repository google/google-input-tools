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
goog.provide('i18n.input.chrome.inputview.Model');

goog.require('goog.events.EventTarget');
goog.require('goog.net.jsloader');
goog.require('i18n.input.chrome.inputview.ConditionName');
goog.require('i18n.input.chrome.inputview.Settings');
goog.require('i18n.input.chrome.inputview.StateManager');
goog.require('i18n.input.chrome.inputview.events.ConfigLoadedEvent');
goog.require('i18n.input.chrome.inputview.events.LayoutLoadedEvent');



/**
 * The model.
 * @constructor
 * @extends {goog.events.EventTarget}
 */
i18n.input.chrome.inputview.Model = function() {
  goog.base(this);

  /**
   * The state manager.
   *
   * @type {!i18n.input.chrome.inputview.StateManager}
   */
  this.stateManager = new i18n.input.chrome.inputview.StateManager();

  /**
   * The configuration.
   *
   * @type {!i18n.input.chrome.inputview.Settings}
   */
  this.settings = new i18n.input.chrome.inputview.Settings();

  goog.exportSymbol('google.ime.chrome.inputview.onLayoutLoaded',
      goog.bind(this.onLayoutLoaded_, this));
  goog.exportSymbol('google.ime.chrome.inputview.onConfigLoaded',
      goog.bind(this.onConfigLoaded_, this));
};
goog.inherits(i18n.input.chrome.inputview.Model, goog.events.EventTarget);


/**
 * The path to the layouts directory.
 *
 * @type {string}
 * @private
 */
i18n.input.chrome.inputview.Model.LAYOUTS_PATH_ =
    '/inputview_layouts/';


/**
 * The path to the content directory.
 *
 * @type {string}
 * @private
 */
i18n.input.chrome.inputview.Model.CONTENT_PATH_ =
    '/config/';


/**
 * Callback when configuration is loaded.
 *
 * @param {!Object} data The configuration data.
 * @private
 */
i18n.input.chrome.inputview.Model.prototype.onConfigLoaded_ = function(data) {
  this.dispatchEvent(new i18n.input.chrome.inputview.events.ConfigLoadedEvent(
      data));
};


/**
 * Callback when layout is loaded.
 *
 * @param {!Object} data The layout data.
 * @private
 */
i18n.input.chrome.inputview.Model.prototype.onLayoutLoaded_ = function(data) {
  this.dispatchEvent(new i18n.input.chrome.inputview.events.LayoutLoadedEvent(
      data));
};


/**
 * Loads a layout.
 *
 * @param {string} layout The layout name.
 */
i18n.input.chrome.inputview.Model.prototype.loadLayout = function(layout) {
  var url = i18n.input.chrome.inputview.Model.LAYOUTS_PATH_ + layout + '.js';
  goog.net.jsloader.load(url);
};


/**
 * Loads the configuration for the keyboard code.
 *
 * @param {string} keyboardCode The keyboard code.
 */
i18n.input.chrome.inputview.Model.prototype.loadConfig = function(
    keyboardCode) {
  // Strips out all the suffixes in the keyboard code.
  var configId = keyboardCode.replace(/\..*$/, '');
  var url = i18n.input.chrome.inputview.Model.CONTENT_PATH_ + configId +
      '.js';
  goog.net.jsloader.load(url);
};

