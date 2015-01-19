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
goog.require('goog.events.EventHandler');
goog.require('goog.events.EventTarget');
goog.require('goog.events.EventType');
goog.require('goog.functions');
goog.require('goog.object');
goog.require('i18n.input.chrome.Env');
goog.require('i18n.input.chrome.hmm.StateID');
goog.require('i18n.input.chrome.inputview.events.KeyCodes');
goog.require('i18n.input.chrome.message.ContextType');
goog.require('i18n.input.chrome.message.Name');
goog.require('i18n.input.chrome.message.Type');
goog.require('i18n.input.chrome.voice.EventType');
goog.require('i18n.input.chrome.voice.VoiceModule');


goog.scope(function() {
var ContextType = i18n.input.chrome.message.ContextType;
var Env = i18n.input.chrome.Env;
var KeyCodes = i18n.input.chrome.inputview.events.KeyCodes;
var Name = i18n.input.chrome.message.Name;
var Type = i18n.input.chrome.message.Type;
var SizeSpec = i18n.input.chrome.inputview.SizeSpec;
var StateID = i18n.input.chrome.hmm.StateID;
var VoiceEventType = i18n.input.chrome.voice.EventType;
var VoiceModule = i18n.input.chrome.voice.VoiceModule;



/**
 * Abstract controller defines the abstracted operations to IME events which
 * specific IME want to integrated into Google Input Tools.
 *
 * @extends {goog.events.EventTarget}
 * @constructor
 */
i18n.input.chrome.AbstractController = function() {
  i18n.input.chrome.AbstractController.base(this, 'constructor');

  /** @protected {!Env} */
  this.env = Env.getInstance();

  /**
   * The voice module.
   *
   * @protected {!VoiceModule}
   */
  this.voiceModule = VoiceModule.getInstance();

  /** @protected {!goog.events.EventHandler} */
  this.eventHandler = new goog.events.EventHandler(this);

  this.eventHandler.listen(this.voiceModule,
      [VoiceEventType.VOICE_RECOG_START,
       VoiceEventType.VOICE_RECOG_END,
       VoiceEventType.VOICE_RECOG_ERROR,
       VoiceEventType.VOICE_RECOG_TIMEOUT],
      this.onVoiceState_);
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
 * The flag to controller whether enable simulating key event for commit text.
 *
 * @type {boolean}
 */
AbstractController.ENABLE_KEY_EVENT_SIMULATION = true;


/**
 * A flag to indicate whether IME is on switching.
 *
 * @type {boolean}
 */
AbstractController.prototype.isSwitching = false;


/**
 * True if this IME is running in standalone mode in which there is no
 * touchscreen keyboard attached.
 *
 * @type {boolean}
 */
AbstractController.prototype.standalone = true;


/**
 * The current keyData being processed by the controller.
 * This is for the sub-class'ed controllers to get the keyData in any methods,
 * particularly for determine whether it should send fake key events or commit
 * text. As for some apps (e.g. Google Spreadsheet) doesn't support direct
 * commit text well.
 *
 * @protected {ChromeKeyboardEvent}
 */
AbstractController.prototype.keyData = null;


/**
 * Sets the current screen type.
 *
 * @param {string} screenType .
 */
AbstractController.prototype.setScreenType = function(screenType) {
  this.env.screenType = screenType;
  this.updateSettings({});
};


/**
 * Activates an input tool.
 *
 * @param {string} engineId The input tool or engine ID.
 */
AbstractController.prototype.activate = function(engineId) {
  this.updateOptions(engineId);
  this.updateInputToolMenu();
  this.voiceModule.activate(engineId);
};


/**
 * Deactivates the current input tool.
 */
AbstractController.prototype.deactivate = function() {
  this.voiceModule.deactivate();
};


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
AbstractController.prototype.register = goog.functions.NULL;


/**
 * Unregister the current context.
 */
AbstractController.prototype.unregister = function() {
  this.voiceModule.unregister();
};


/**
 * Resets the current context.
 */
AbstractController.prototype.reset = goog.functions.NULL;


/**
 * Callback when composition is cancelled by system.
 */
AbstractController.prototype.onCompositionCanceled = function() {
  this.voiceModule.onCompositionCanceled();
};


/**
 * Callback for system "onSurroundingTextChanged" event.
 * @param {string} engineID
 * @param {!Object} surroundingInfo
 */
AbstractController.prototype.onSurroundingTextChanged = goog.functions.NULL;


/**
 * Handler for voice module state change.
 *
 * @param {goog.events.Event} e .
 * @private
 */
AbstractController.prototype.onVoiceState_ = function(e) {
  chrome.runtime.sendMessage(goog.object.create(
      Name.TYPE, Type.VOICE_STATE_CHANGE,
      Name.MSG, goog.object.create(
          Name.VOICE_STATE, e.type == VoiceEventType.VOICE_RECOG_START)));
};


/**
 * Gets whether the key event was simulated for commitText.
 *
 * @param {!ChromeKeyboardEvent} e the key event.
 * @return {boolean} .
 * @protected
 */
AbstractController.prototype.isSimulated = function(e) {
  return AbstractController.ENABLE_KEY_EVENT_SIMULATION && e.key.length == 1 &&
      !!e.extensionId;
};


/**
 * Handles key event.
 *
 * @param {!ChromeKeyboardEvent} e the key event.
 * @return {boolean|undefined} True if the event is handled successfully or not,
 *     Undefined means it's unsure at that moment.
 */
AbstractController.prototype.handleEvent = function(e) {
  // Press any key on physical keyboard, stop voice recognition.
  if (/^key/.test(e.type)) {
    this.voiceModule.reset();
  }
  this.keyData = e;
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

  keyData['fromInputView'] = true;
  this.keyData = /** @type {!ChromeKeyboardEvent} */ (keyData);
  if (this.isNonCharacterKeyEvent(this.keyData)) {
    this.handleNonCharacterKeyEvent(this.keyData);
  } else if (this.isPasswdBox()) {
    this.handleSendKeyEventInPasswordBox_(this.keyData);
  } else {
    this.handleCharacterKeyEvent(this.keyData);
  }
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
    this.commitText(keyData[Name.KEY], false);
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
AbstractController.prototype.switchInputToolState =
    function(stateId, opt_stateIdValue) {
  if (stateId == StateID.VOICE) {
    if (this.voiceModule.isVisible()) {
      this.voiceModule.deactivate();
    } else {
      this.voiceModule.activate(this.env.engineId);
    }
  }
  this.updateInputToolMenu();
};


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
  this.keyData = null;
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
      this.commitText(text, false);
      break;
    case Type.VISIBILITY_CHANGE:
      this.standalone = !message[Name.VISIBILITY];
      break;
    case Type.VOICE_VIEW_STATE_CHANGE:
      if (!this.standalone) {
        if (message[Name.MSG]) {
          this.voiceModule.start();
        } else {
          this.voiceModule.stop();
        }
      }
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
 * @param {boolean} onStage Whether is composition text onstage.
 *     This is used to check whether need to simulate key events instead of
 *     directly committing the text.
 * @protected
 */
AbstractController.prototype.commitText = function(text, onStage) {
  if (this.env.context && text) {
    // this.keyData.key.length == 1 means the recorded keyData is character key
    // instead of special key like Backspace, Enter, Esc, etc.
    if (AbstractController.ENABLE_KEY_EVENT_SIMULATION &&
        this.keyData && !onStage && text.length == 1 &&
        this.keyData.key.length == 1) {
      this.keyData['key'] = text;
      this.sendKeyEvent(this.keyData);
    } else {
      chrome.input.ime.commitText(
          goog.object.create(
              Name.CONTEXT_ID,
              this.env.context.contextID,
              Name.TEXT,
              text));
    }
  }
};


/**
 * Sends message to input view to update settings.
 *
 * @param {!Object} settings .
 * @protected
 */
AbstractController.prototype.updateSettings = function(settings) {
  settings[Name.CONTEXT_TYPE] =
      this.env.context ? this.env.context.type : ContextType.NONE;
  settings[Name.SCREEN] = this.env.screenType;
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


/**
 * Is a password box.
 *
 * @return {boolean} .
 * @protected
 */
AbstractController.prototype.isPasswdBox = function() {
  return !!this.env.context && (!this.env.context.type ||
      this.env.context.type == 'password');
};


/**
 * Is suggestions supported.
 * This is only for XKB/Latin controllers.
 *
 * @return {boolean}
 * @protected
 */
AbstractController.prototype.isSuggestionSupported = function() {
  var context = this.env.context;
  if (!context || !goog.array.contains(['text', 'search'], context.type)) {
    return false;
  }
  return !goog.isDef(context.autoCorrect) && !goog.isDef(context.spellCheck) ||
      !!context.autoCorrect && !!context.spellCheck;
};


/**
 * Gets the Input Tool menus
 *
 * @return {Array.<!Object.<string, *>>} The menus.
 * @protected
 */
AbstractController.prototype.getInputToolMenu = function() {
  // Disable voice menu for physical keyboard.
  return null;
};


/**
 * Update the Input Tool menu.
 *
 * @protected
 */
AbstractController.prototype.updateInputToolMenu = function() {
  var menus = this.getInputToolMenu();
  if (menus) {
    // Sets up voice option.
    chrome.input.ime.setMenuItems(goog.object.create(
        'engineID', this.env.engineId,
        'items', menus));
  }
};


/** @override */
AbstractController.prototype.disposeInternal = function() {
  goog.dispose(this.voiceModule);
  goog.dispose(this.eventHandler);
  goog.base(this, 'disposeInternal');
};
});  // goog.scope

