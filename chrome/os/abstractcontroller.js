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
goog.provide('i18n.input.chrome.AbstractController');

goog.require('goog.array');
goog.require('goog.events.EventTarget');
goog.require('goog.events.EventType');
goog.require('goog.functions');
goog.require('goog.object');
goog.require('i18n.input.chrome.inputview.events.KeyCodes');
goog.require('i18n.input.chrome.message.Name');
goog.require('i18n.input.chrome.message.Type');


goog.scope(function() {
var Name = i18n.input.chrome.message.Name;
var Type = i18n.input.chrome.message.Type;
var KeyCodes = i18n.input.chrome.inputview.events.KeyCodes;



/**
 * Abstract controller defines the abstracted operations to IME events which
 * specific IME want to integrated into Google Input Tools.
 *
 * @extends {goog.events.EventTarget}
 * @constructor
 */
i18n.input.chrome.AbstractController = function() {
  goog.base(this);
};
var AbstractController = i18n.input.chrome.AbstractController;
goog.inherits(AbstractController, goog.events.EventTarget);


/**
 * The request id.
 *
 * @type {number}
 */
AbstractController.prototype.requestId = 0;


/**
 * The functional keys.
 *
 * @type {!Array.<string>}
 */
AbstractController.FUNCTIONAL_KEYS = [
  KeyCodes.ARROW_DOWN,
  KeyCodes.ARROW_LEFT,
  KeyCodes.ARROW_RIGHT,
  KeyCodes.ARROW_UP,
  KeyCodes.BACKSPACE,
  KeyCodes.CONVERT,
  KeyCodes.ENTER,
  KeyCodes.NON_CONVERT,
  KeyCodes.TAB
];


/**
 * The ime context.
 *
 * @type {InputContext}
 * @protected
 */
AbstractController.prototype.context = null;


/**
 * True if this IME is running in standalone mode in which there is no
 * touchscreen keyboard attached.
 *
 * @type {boolean}
 */
AbstractController.prototype.standalone = true;


/**
 * The current screen type.
 *
 * @protected {string}
 */
AbstractController.prototype.screenType = 'normal';


/**
 * Sets the current screen type.
 *
 * @param {string} screenType .
 */
AbstractController.prototype.setScreenType = function(screenType) {
  this.screenType = screenType;
  this.updateSettings({});
};


/**
 * Activates an input tool.
 *
 * @param {string} inputToolCode The input tool or engine ID.
 */
AbstractController.prototype.activate = function(inputToolCode) {
  this.updateOptions(inputToolCode);
};


/**
 * Deactivates the current input tool.
 */
AbstractController.prototype.deactivate = goog.functions.NULL;


/**
 * Callback method called when properties of the context is changed.
 * @param {!InputContext} context context information.
 */
AbstractController.prototype.onInputContextUpdate = goog.functions.NULL;


/**
 * Register a context.
 *
 * @param {!InputContext} context The context.
 */
AbstractController.prototype.register = function(context) {
  this.context = context;
};


/**
 * Unregister the current context.
 */
AbstractController.prototype.unregister = function() {
  this.context = null;
};


/**
 * Resets the current context.
 */
AbstractController.prototype.reset = goog.functions.NULL;


/**
 * Callback when composition is cancelled by system.
 */
AbstractController.prototype.onCompositionCanceled = goog.functions.NULL;


/**
 * Callback for system "onSurroundingTextChanged" event.
 * @param {string} engineID
 * @param {!Object} surroundingInfo
 */
AbstractController.prototype.onSurroundingTextChanged = goog.functions.NULL;


/**
 * Handles key event.
 *
 * @param {!ChromeKeyboardEvent} e the key event.
 * @return {boolean|undefined} True if the event is handled successfully or not,
 *     Undefined means it's unsure at that moment.
 */
AbstractController.prototype.handleEvent = function(e) {
  return false;
};


/**
 * True if this is a non-character key event.
 *
 * @param {!ChromeKeyboardEvent} keyData .
 * @return {boolean} .
 */
AbstractController.prototype.isNonCharacterKeyEvent = function(keyData) {
  var code = keyData[Name.CODE];
  var ctrlKey = keyData[Name.CTRL_KEY];
  var altKey = keyData[Name.ALT_KEY];
  return goog.array.contains(AbstractController.FUNCTIONAL_KEYS, code) ||
      ctrlKey || altKey;
};


/**
 * Handles the message - SEND_KEY_EVENT.
 *
 * @param {!ChromeKeyboardEvent | !Array.<!ChromeKeyboardEvent>} keyData .
 * @private
 */
AbstractController.prototype.handleSendKeyEventMessage_ = function(keyData) {
  if (goog.isArrayLike(keyData)) {
    for (var i = 0; i < keyData.length; i++) {
      this.handleSendKeyEventMessage_(keyData[i]);
    }
    return;
  }

  var kData = /** @type {!ChromeKeyboardEvent} */ (keyData);
  if (this.isNonCharacterKeyEvent(kData)) {
    this.handleNonCharacterKeyEvent(kData);
  } else if (this.isPasswordBox()) {
    this.handleSendKeyEventInPasswordBox_(kData);
  } else {
    this.handleCharacterKeyEvent(kData);
  }
};


/**
 * Whether current input  box is password box.
 *
 * @return {boolean}
 * @protected
 */
AbstractController.prototype.isPasswordBox = function() {
  return !!this.context && this.context.type == 'password';
};


/**
 * Handle send key event in password input box.
 *
 * @param {!ChromeKeyboardEvent} keyData .
 * @private
 */
AbstractController.prototype.handleSendKeyEventInPasswordBox_ =
    function(keyData) {
  if (keyData[Name.TYPE] == goog.events.EventType.KEYDOWN) {
    this.commitText(keyData[Name.KEY]);
  }
};


/**
 * Handles character key event.
 *
 * @param {!ChromeKeyboardEvent} keyData .
 */
AbstractController.prototype.handleCharacterKeyEvent = function(keyData) {
  var ret = this.handleEvent(keyData);
  if (ret === false) {
    this.handleSendKeyEventInPasswordBox_(keyData);
  }
};


/**
 * Handles non-character key event, it could be shortcut or functional key.
 *
 * @param {!ChromeKeyboardEvent} keyData .
 */
AbstractController.prototype.handleNonCharacterKeyEvent = function(
    keyData) {
  // Hacks for special JP layout.
  if (keyData[Name.CODE] == KeyCodes.NON_CONVERT) {
    keyData[Name.KEYCODE] = 0x1D;
  } else if (keyData[Name.CODE] == KeyCodes.CONVERT) {
    keyData[Name.KEYCODE] = 0x1C;
  }
  this.sendKeyEvent(keyData);
};


/**
 * Sends fake key event.
 *
 * @param {!ChromeKeyboardEvent} keyData .
 */
AbstractController.prototype.sendKeyEvent = function(keyData) {
  var kData = {};
  var properties = ['type', 'requestId', 'extensionId', 'key', 'code',
                    'keyCode', 'altKey', 'ctrlKey', 'shiftKey', 'capsLock'];
  goog.array.forEach(properties, function(property) {
    if (goog.isDef(keyData[property])) {
      kData[property] = keyData[property];
    }
  });
  kData[Name.REQUEST_ID] = String(this.requestId++);
  chrome.input.ime.sendKeyEvents(goog.object.create(
      Name.CONTEXT_ID, 0,
      Name.KEY_DATA, [kData]));
};


/**
 * Selects the candidate of the index.
 *
 * @param {number} id The id of candidate.
 */
AbstractController.prototype.selectCandidate = goog.functions.NULL;


/**
 * Switch the input tool state.
 *
 * @param {string} stateId The state ID.
 * @param {boolean=} opt_stateIdValue The value of state ID.
 */
AbstractController.prototype.switchInputToolState = goog.functions.NULL;


/**
 * Updates options for this input tool code.
 *
 * @param {string} inputToolCode The input method to update.
 */
AbstractController.prototype.updateOptions = goog.functions.NULL;


/**
 * Processes incoming message from option page or inputview window.
 *
 * @param {*} message Message from option page or inputview window.
 * @param {!MessageSender} sender Information about the script
 *     context that sent the message.
 * @param {function(*): void} sendResponse Function to call to send a response.
 * @return {boolean|undefined} {@code true} to keep the message channel open in
 *     order to send a response asynchronously.
 */
AbstractController.prototype.processMessage = function(message, sender,
    sendResponse) {
  var msgType = message[Name.TYPE];
  switch (msgType) {
    case Type.SEND_KEY_EVENT:
      this.handleSendKeyEventMessage_(message[Name.KEY_DATA]);
      break;
    case Type.SELECT_CANDIDATE:
      var candidate = message[Name.CANDIDATE];
      this.selectCandidate(candidate[Name.ID]);
      break;
    case Type.COMMIT_TEXT:
      var text = message[Name.TEXT];
      this.commitText(text);
      break;
    case Type.VISIBILITY_CHANGE:
      this.standalone = !message[Name.VISIBILITY];
      break;
  }
};


/**
 * True if this IME is running in standalone mode in which the IME doesn't get
 * attached to a touchscreen keyboard.
 *
 * @return {boolean} .
 */
AbstractController.prototype.isStandalone = function() {
  return this.standalone;
};


/**
 * Commits a string.
 *
 * @param {string} text The string to commit.
 * @protected
 */
AbstractController.prototype.commitText = function(text) {
  if (this.context) {
    chrome.input.ime.commitText(
        goog.object.create(
            Name.CONTEXT_ID,
            this.context.contextID,
            Name.TEXT,
            text));
  }
};


/**
 * Sends message to input view to update settings.
 *
 * @param {!Object} settings .
 * @protected
 */
AbstractController.prototype.updateSettings = function(settings) {
  if (this.context) {
    settings[Name.CONTEXT_TYPE] = this.context.type;
  }
  settings[Name.SCREEN] = this.screenType;
  chrome.runtime.sendMessage(goog.object.create(
      Name.TYPE, Type.UPDATE_SETTINGS,
      Name.MSG, settings));
};


/**
 * Gets whether the controller is ready for handling events.
 *
 * @return {boolean}
 */
AbstractController.prototype.isReady = goog.functions.TRUE;
});  // goog.scope
