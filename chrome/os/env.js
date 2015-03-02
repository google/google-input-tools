// Copyright 2015 The ChromeOS IME Authors. All Rights Reserved.
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
goog.provide('i18n.input.chrome.Env');

goog.require('i18n.input.chrome.inputview.FeatureTracker');

goog.scope(function() {
var FeatureTracker = i18n.input.chrome.inputview.FeatureTracker;


/**
 * The Environment class which holds some status' of ChromeOS for input methods.
 * e.g. the current input field context, the current input method engine ID,
 * the flags, etc.
 *
 * @constructor
 */
i18n.input.chrome.Env = function() {

  /**
   * Tracker for which FeatureName are enabled.
   *
   * @type {!FeatureTracker};
   */
  this.featureTracker = new FeatureTracker();

  if (chrome.accessibilityFeatures &&
      chrome.accessibilityFeatures.spokenFeedback) {
    chrome.accessibilityFeatures.spokenFeedback.get({}, (function(details) {
      this.isChromeVoxOn = details['value'];
    }).bind(this));

    chrome.accessibilityFeatures.spokenFeedback.onChange.addListener(
        function(details) {
          this.isChromeVoxOn = details['value'];
        }.bind(this));
  }

  if (window.inputview && inputview.getKeyboardConfig) {
    inputview.getKeyboardConfig((function(config) {
        this.featureTracker.initialize(config);
    }).bind(this));
  }
};
var Env = i18n.input.chrome.Env;
goog.addSingletonGetter(Env);


/** @type {string} */
Env.prototype.engineId = '';


/** @type {InputContext} */
Env.prototype.context = null;


/** @type {string} */
Env.prototype.screenType = 'normal';


/** @type {boolean} */
Env.prototype.isPhysicalKeyboardAutocorrectEnabled = false;


/** @type {string} */
Env.prototype.textBeforeCursor = '';


/** @type {Object.<{text: string, focus: number, anchor: number}>} */
Env.prototype.surroundingInfo = null;


/** @type {boolean} */
Env.prototype.isChromeVoxOn = false;

});  // goog.scope
