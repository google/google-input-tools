// Copyright 2013 The ChromeOS IME Authors. All Rights Reserved.
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

/**
 * @fileoverview Virtual Keyboard controller for Chrome OS.
 */

goog.provide('i18n.input.chrome.vk.Controller');

goog.require('goog.Disposable');
goog.require('goog.events.EventHandler');
goog.require('goog.events.KeyCodes');
goog.require('i18n.input.chrome.vk.ComposeTextInput');
goog.require('i18n.input.chrome.vk.DeferredCallManager');
goog.require('i18n.input.chrome.vk.DirectTextInput');
goog.require('i18n.input.chrome.vk.EventType');
goog.require('i18n.input.chrome.vk.KeyCode');
goog.require('i18n.input.chrome.vk.Model');



/**
 * Creates the Controller object.
 *
 * @constructor
 * @extends {goog.Disposable}
 */
i18n.input.chrome.vk.Controller = function() {
  goog.base(this);

  /**
   * The keyboard module object.
   *
   * @type {!i18n.input.chrome.vk.Model}
   * @private
   */
  this.model_ = new i18n.input.chrome.vk.Model();

  /**
   * The event handler.
   *
   * @type {!goog.events.EventHandler}
   * @private
   */
  this.eventHandler_ = new goog.events.EventHandler(this);

  this.eventHandler_.listen(this.model_,
      i18n.input.chrome.vk.EventType.LAYOUT_LOADED, this.onLayoutLoaded_);
};
goog.inherits(i18n.input.chrome.vk.Controller, goog.Disposable);


/**
 * The text input.
 *
 * @type {i18n.input.chrome.vk.TextInput}
 * @private
 */
i18n.input.chrome.vk.Controller.prototype.textInput_ = null;


/**
 * The input context.
 *
 * @type {InputContext}
 * @private
 */
i18n.input.chrome.vk.Controller.prototype.context_ = null;


/**
 * The vk state controled by key modifiers. And it consists of the bit flags
 * i18n.input.chrome.vk.Controller.StateBit.
 *
 * @type {number}
 * @private
 */
i18n.input.chrome.vk.Controller.prototype.state_ = 0;


/**
 * The layout object.
 *
 * @type {i18n.input.chrome.vk.ParsedLayout}
 * @private
 */
i18n.input.chrome.vk.Controller.prototype.layout_ = null;


/**
 * The engine ID.
 *
 * @type {string}
 * @private
 */
i18n.input.chrome.vk.Controller.prototype.engineID_ = '';


/**
 * The state bits.
 *
 * @enum {number}
 * @const
 */
i18n.input.chrome.vk.Controller.StateBit = {
  SHIFT: 1,
  ALTGR: 2,
  CAPS: 4
};


/**
 * Sets the ALTGR/SHIFT/CAPS state.
 *
 * @param {i18n.input.chrome.vk.Controller.StateBit} stateBit .
 * @param {boolean} set Whether to set the state. False to unset.
 */
i18n.input.chrome.vk.Controller.prototype.setState = function(
    stateBit, set) {
  if (set) {
    this.state_ |= stateBit;
  } else {
    this.state_ &= ~stateBit;
  }
};


/**
 * Gets the state.
 *
 * @param {i18n.input.chrome.vk.Controller.StateBit} stateBit .
 * @return {boolean} Whether state is set.
 */
i18n.input.chrome.vk.Controller.prototype.getState = function(stateBit) {
  return !!(this.state_ & stateBit);
};


/**
 * Creates text input instance.
 *
 * @private
 */
i18n.input.chrome.vk.Controller.prototype.createTextInput_ = function() {
  if (this.context_ && this.layout_) {
    if (this.model_.hasTransforms()) {
      this.textInput_ = new i18n.input.chrome.vk.ComposeTextInput(
          this.context_, this.model_);
    } else {
      this.textInput_ = new i18n.input.chrome.vk.DirectTextInput(this.context_);
    }
    this.textInput_.engineID = this.engineID_;
  }
};


/**
 * Sets the input box context.
 *
 * @param {InputContext} context The input box context.
 */
i18n.input.chrome.vk.Controller.prototype.setContext = function(context) {
  this.context_ = context;
  this.createTextInput_();
};


/**
 * Sets the text before cursor.
 *
 * @param {string} text The text before cursor.
 */
i18n.input.chrome.vk.Controller.prototype.handleSurroundingTextChanged =
    function(text) {
  if (this.textInput_) {
    if (this.textInput_ instanceof i18n.input.chrome.vk.DirectTextInput) {
      this.textInput_.textBeforeCursor = text.slice(-20);
    }
  }
};


/**
 * Activates the vk via input tool.
 *
 * @param {string} engineId The engine ID which contains layout code.
 */
i18n.input.chrome.vk.Controller.prototype.activate = function(engineId) {
  if (engineId) {
    var matches = engineId.match(/^vkd_([^\-]*)/);
    if (matches) {
      this.engineID_ = engineId;
      this.model_.loadLayout(matches[1]);
    }
  }
};


/**
 * Deactivates the keyboard.
 */
i18n.input.chrome.vk.Controller.prototype.deactivate = function() {
  this.engineID_ = '';
  this.layout_ = null;
  this.state_ = 0;
  this.setState(i18n.input.chrome.vk.Controller.StateBit.ALTGR, false);
};


/**
 * Handler for layout loaded.
 *
 * @param {!i18n.input.chrome.vk.LayoutEvent} e The event of layout loaded.
 * @private
 */
i18n.input.chrome.vk.Controller.prototype.onLayoutLoaded_ = function(e) {
  this.layout_ = /** @type {!i18n.input.chrome.vk.ParsedLayout} */ (
      e.layoutView);
  this.model_.activateLayout(this.layout_.id);
  this.createTextInput_();
};


/**
 * Resets the text input.
 */
i18n.input.chrome.vk.Controller.prototype.resetTextInput = function() {
  if (this.textInput_) {
    this.textInput_.reset();
  }
};


/**
 * Handles the onReset event.
 */
i18n.input.chrome.vk.Controller.prototype.handleReset = function() {
  if (this.textInput_) {
    this.textInput_.textBeforeCursor = '';
  }
};


/**
 * Gets the UI states for refreshing view.
 *
 * @return {string} The UI states represented by a string.
 * @private
 */
i18n.input.chrome.vk.Controller.prototype.getUiState_ = function() {
  var uiState = '';
  var states = i18n.input.chrome.vk.Controller.StateBit;
  if (this.state_ & states.SHIFT) {
    uiState += 's';
  }
  if (this.state_ & states.ALTGR) {
    uiState += 'c';
  }
  if (this.state_ & states.CAPS) {
    uiState += 'l';
  }
  return uiState;
};


/**
 * Updates the modifier state by the given key event.
 *
 * @param {goog.events.BrowserEvent} e The key event.
 * @private
 */
i18n.input.chrome.vk.Controller.prototype.updateState_ = function(e) {
  var codes = goog.events.KeyCodes;
  var states = i18n.input.chrome.vk.Controller.StateBit;

  // Initial state needs to carry the current capslock and click status.
  var state = this.state_ & states.CAPS;
  // Refreshes the modifier status.
  if (e.keyCode == codes.CAPS_LOCK) {
    state |= states.CAPS;
  } else if (e.capsLock != undefined) {
    if (e.capsLock) {
      state |= states.CAPS;
    } else {
      state &= ~states.CAPS;
    }
  } else {
    var c = String.fromCharCode(e.charCode);
    if (e.shiftKey && /[a-z]/.test(c) || !e.shiftKey && /[A-Z]/.test(c)) {
      state |= states.CAPS;
    }
  }
  if (e.keyCode == codes.SHIFT || e.shiftKey) {
    state |= states.SHIFT;
  }
  if (e.altKey && e.ctrlKey ||
      e.keyCode == codes.ALT && e.ctrlKey ||
      e.keyCode == codes.CTRL && e.altKey) {
    state |= states.ALTGR;
  }
  this.state_ = state;
};


/**
 * Handles the keydown events.
 *
 * @param {goog.events.BrowserEvent} e The key event.
 * @return {boolean} False to continue handling, True otherwise.
 */
i18n.input.chrome.vk.Controller.prototype.processEvent = function(e) {
  if (!this.textInput_ || !this.layout_) {
    return false;
  }

  this.updateState_(e);

  // Check hot key.
  var states = i18n.input.chrome.vk.Controller.StateBit;
  if (e.ctrlKey || e.altKey && !(this.state_ & states.ALTGR)) {
    return false;
  }
  return this.commitChars_(e.keyCode);
};


/**
 * Commits the chars given the key code.
 *
 * @param {number} keyCode The key code.
 * @return {boolean} False to continue hanlding, True otherwise.
 * @private
 */
i18n.input.chrome.vk.Controller.prototype.commitChars_ = function(keyCode) {
  var codes = goog.events.KeyCodes;
  var chars = null;
  var keyChar = String.fromCharCode(keyCode);
  var viewState = this.getUiState_();
  var mappings = this.layout_.view['mappings'];
  var is102 = this.layout_.view['is102'];
  if (!mappings[viewState]) {
    return false;
  }
  var item = mappings[viewState][keyChar];
  if (item) {
    chars = item[2];
  }
  if (!chars && keyCode == codes.SPACE) {
    chars = '\u0020';
  }
  if (!chars) {
    // Ignore the space bar for layout backward compatibility.
    var keyCodes = is102 ? i18n.input.chrome.vk.KeyCode.CODES102 :
        i18n.input.chrome.vk.KeyCode.CODES101;
    chars = keyCodes.indexOf(keyChar) >= 0 ? '' : null;
  }

  // Chars could be null or ''. Basically if chars=='', we should
  // commit nothing; and if chars==null, we should return false so that the
  // browser will take the default behavior.
  // But if chars==null and the key is BS, we fake the transform result
  // so that the input value could be changed accordingly.
  if (chars == '') {
    return true;
  } else if (chars == null && keyCode != codes.BACKSPACE) {
    return false;
  }
  var trans = {back: 1, chars: ''}; // Sets trans as BS by default.
  if (this.model_.hasTransforms()) {
    // Only gets text before caret when the layout has transforms defined.
    var text = this.textInput_.textBeforeCursor;
    if (keyCode == codes.BACKSPACE) {
      this.model_.processBackspace(text); // Maybe change history.
    } else {
      trans = this.model_.translate(/** @type {string} */ (chars), text);
    }
  } else if (chars) { // Not BS & non-transform layout.
    trans = {back: 0, chars: chars};
  }

  return this.textInput_.commitText(trans.chars, trans.back);
};


/** @override */
i18n.input.chrome.vk.Controller.prototype.disposeInternal = function() {
  goog.dispose(this.model_);
  goog.dispose(this.eventHandler_);

  goog.base(this, 'disposeInternal');
};
