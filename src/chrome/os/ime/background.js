// Copyright 2013 The ChromeOS IME Authors. All Rights Reserved.
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
 * @fileoverview The background script for os decoder chrome os extension.
 */

goog.provide('goog.ime.chrome.os.Background');

goog.require('goog.ime.chrome.os.Controller');
goog.require('goog.ime.chrome.os.LocalStorageHandlerFactory');



/**
 * The background class implements the script for the background page of chrome
 * os extension for os input tools.
 *
 * @constructor
 */
goog.ime.chrome.os.Background = function() {
  /**
   * The controller for chrome os extension.
   *
   * @type {goog.ime.chrome.os.Controller}
   * @private
   */
  this.controller_ = new goog.ime.chrome.os.Controller();

  /**
   * The local storage handler.
   *
   * @type {goog.ime.chrome.os.LocalStorageHandlerFactory}
   * @private
   */
  this.localStorageHandlerFactory_ =
      new goog.ime.chrome.os.LocalStorageHandlerFactory();

  // Sets up a listener which talks to the option page.
  chrome.extension.onRequest.addListener(goog.bind(this.processRequest, this));

  this.init_();
};


/**
 * Initializes the background scripts.
 *
 * @private
 */
goog.ime.chrome.os.Background.prototype.init_ = function() {
  this.updateSettingsFromLocalStorage_();

  var self = this;
  chrome.input.ime.onActivate.addListener(function(engineID) {
    self.controller_.activate(engineID);
  });

  chrome.input.ime.onDeactivated.addListener(function() {
    self.controller_.deactivate();
  });

  chrome.input.ime.onFocus.addListener(function(context) {
    self.controller_.register(context);
  });

  chrome.input.ime.onBlur.addListener(function(contextID) {
    self.controller_.unregister();
  });

  // Since onReset evnet is implemented in M29, it needs to keep the backward
  // compatibility here.
  var onReset = chrome.input.ime['onReset'];
  if (onReset) {
    onReset['addListener'](function(engineID) {
      self.controller_.reset();
    });
  }

  chrome.input.ime.onKeyEvent.addListener(function(engine, keyEvent) {
    return self.controller_.handleEvent(keyEvent);
  });

  chrome.input.ime.onCandidateClicked.addListener(function(
      engineID, candidateID, button) {
        self.controller_.processNumberKey({'key': candidateID + 1});
      });

  chrome.input.ime.onMenuItemActivated.addListener(function(
      engineID, stateID) {
        self.controller_.switchInputToolState(stateID);
      });

  if (chrome.inputMethodPrivate && chrome.inputMethodPrivate.startIme) {
    chrome.inputMethodPrivate.startIme();
  }
};


/**
 * Updates settings from local storage.
 *
 * @param {string=} opt_inputToolCode the input tool code whose settings are
 *     updated. If it is undefined, updates all the settings.
 * @private
 */
goog.ime.chrome.os.Background.prototype.updateSettingsFromLocalStorage_ =
    function(opt_inputToolCode) {
  if (opt_inputToolCode) {
    var localStorageHandler = this.localStorageHandlerFactory_.
        getLocalStorageHandler(opt_inputToolCode);
    localStorageHandler.updateControllerSettings(this.controller_);
  } else {
    var localStorageHandlers = this.localStorageHandlerFactory_.
        getAllLocalStorageHandlers();
    for (var i = 0; i < localStorageHandlers.length; ++i) {
      localStorageHandlers[i].updateControllerSettings(this.controller_);
    }
  }
};


/**
 * Processes incoming requests from option page.
 *
 * @param {Object} request Request from option page.
 *     - update_setting - updates the settings from local storage.
 */
goog.ime.chrome.os.Background.prototype.processRequest = function(
    request) {
  if (request['update']) {
    this.updateSettingsFromLocalStorage_(request['update']);
  }
};


(function() {
  new goog.ime.chrome.os.Background();
}) ();
