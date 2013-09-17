// Copyright 2013 The ChromeOS VK Authors. All Rights Reserved.
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
 * @fileoverview Virtual Keyboard controller for Chrome OS.
 */

goog.provide('goog.ime.chrome.vk.Controller');

goog.require('goog.Disposable');
goog.require('goog.events.EventHandler');
goog.require('goog.events.KeyCodes');
goog.require('goog.ime.chrome.vk.ComposeTextInput');
goog.require('goog.ime.chrome.vk.DeferredCallManager');
goog.require('goog.ime.chrome.vk.DirectTextInput');
goog.require('goog.ime.chrome.vk.EventType');
goog.require('goog.ime.chrome.vk.KeyCode');
goog.require('goog.ime.chrome.vk.Model');



/**
 * Creates the Controller object.
 *
 * @constructor
 * @extends {goog.Disposable}
 */
goog.ime.chrome.vk.Controller = function() {
  goog.base(this);

  /**
   * The keyboard module object.
   *
   * @type {!goog.ime.chrome.vk.Model}
   * @private
   */
  this.model_ = new goog.ime.chrome.vk.Model();

  /**
   * The event handler.
   *
   * @type {!goog.events.EventHandler}
   * @private
   */
  this.eventHandler_ = new goog.events.EventHandler(this);

  this.eventHandler_.listen(this.model_,
      goog.ime.chrome.vk.EventType.LAYOUT_LOADED, this.onLayoutLoaded_);

  var self = this;
  chrome.runtime.onConnect.addListener(function(port) {
    self.port_ = port;
    if (self.layout_) {
      port.postMessage({
        'type': 'layout',
        'layout': self.layout_.view
      });
    }
    port.onMessage.addListener(goog.bind(self.handleMessage_, self));
  });
  chrome.windows.onRemoved.addListener(function(winId) {
    if (self.port_) {
      self.port_.disconnect();
      self.winId_ = 0;
      self.port_ = null;
    }
  });
};
goog.inherits(goog.ime.chrome.vk.Controller, goog.Disposable);


/**
 * @type {Port}
 * @private
 */
goog.ime.chrome.vk.Controller.prototype.port_ = null;


/**
 * The text input.
 *
 * @type {goog.ime.chrome.vk.TextInput}
 * @private
 */
goog.ime.chrome.vk.Controller.prototype.textInput_ = null;


/**
 * The input context.
 *
 * @type {InputContext}
 * @private
 */
goog.ime.chrome.vk.Controller.prototype.context_ = null;


/**
 * The vk state controled by key modifiers. And it consists of the bit flags
 * goog.ime.chrome.vk.Controller.StateBit.
 *
 * @type {number}
 * @private
 */
goog.ime.chrome.vk.Controller.prototype.state_ = 0;


/**
 * The layout object.
 *
 * @type {goog.ime.chrome.vk.ParsedLayout}
 * @private
 */
goog.ime.chrome.vk.Controller.prototype.layout_ = null;


/**
 * The engine ID.
 *
 * @type {string}
 * @private
 */
goog.ime.chrome.vk.Controller.prototype.engineID_ = '';


/**
 * The view's window id.
 *
 * @type {number}
 * @private
 */
goog.ime.chrome.vk.Controller.prototype.winId_ = 0;


/**
 * The state bits.
 *
 * @enum {number}
 * @const
 */
goog.ime.chrome.vk.Controller.StateBit = {
  SHIFT: 1,
  ALTGR: 2,
  CAPS: 4,
  SHIFT_CLICK: 256,
  ALTGR_CLICK: 512
};


/**
 * Handles the messages from view.
 *
 * @param {Object} message The message from view.
 * @private
 */
goog.ime.chrome.vk.Controller.prototype.handleMessage_ = function(
    message) {
  var type = message['type'];
  if (type != 'click' || !this.port_) {
    return;
  }
  var keyCode = message[type];
  var codes = goog.events.KeyCodes;
  var states = goog.ime.chrome.vk.Controller.StateBit;
  var orgState = this.state_;
  switch (keyCode) {
    case codes.SHIFT:
      // If shift is on, copies shift to shift click, and clear shift state.
      if (this.state_ & states.SHIFT) {
        this.state_ |= states.SHIFT_CLICK;
        this.state_ &= ~states.SHIFT;
      }
      this.state_ ^= states.SHIFT_CLICK;
      break;
    case 273: // Key code 273 means ALTGR.
      // If altgr is on, copies altgr to altgr click, and clear altgr state.
      if ((this.state_ & states.ALT) && (this.state_ & states.CTRL)) {
        this.state_ |= states.ALTGR_CLICK;
        this.state_ &= ~(states.ALT | states.CTRL);
      }
      this.state_ ^= states.ALTGR_CLICK;
      break;
    case codes.CAPS_LOCK:
      this.state_ ^= states.CAPS;
      break;
    default:
      this.commitChars_(keyCode);
      goog.ime.chrome.vk.DeferredCallManager.getInstance().execAll();
      break;
  }
  if (this.state_ != orgState) {
    this.port_.postMessage({
      'type': 'state',
      'state': this.getUiState_()
    });
  }
};


/**
 * Sets the ALTGR/SHIFT/CAPS state.
 *
 * @param {goog.ime.chrome.vk.Controller.StateBit} stateBit .
 * @param {boolean} set Whether to set the state. False to unset.
 */
goog.ime.chrome.vk.Controller.prototype.setState = function(
    stateBit, set) {
  var orgState = this.state_;
  if (set) {
    this.state_ |= stateBit;
  } else {
    this.state_ &= ~stateBit;
  }
  if (orgState != this.state_) {
    if (this.port_) {
      this.port_.postMessage({
        'type': 'state',
        'state': this.getUiState_()
      });
    }
  }
};


/**
 * Gets the state.
 *
 * @param {goog.ime.chrome.vk.Controller.StateBit} stateBit .
 * @return {boolean} Whether state is set.
 */
goog.ime.chrome.vk.Controller.prototype.getState = function(stateBit) {
  return !!(this.state_ & stateBit);
};


/**
 * Creates text input instance.
 *
 * @private
 */
goog.ime.chrome.vk.Controller.prototype.createTextInput_ = function() {
  if (this.context_ && this.layout_) {
    if (this.model_.hasTransforms()) {
      this.textInput_ = new goog.ime.chrome.vk.ComposeTextInput(
          this.context_, this.model_);
    } else {
      this.textInput_ = new goog.ime.chrome.vk.DirectTextInput(this.context_);
    }
    this.textInput_.engineID = this.engineID_;
  }
};


/**
 * Sets the input box context.
 *
 * @param {InputContext} context The input box context.
 */
goog.ime.chrome.vk.Controller.prototype.setContext = function(context) {
  this.context_ = context;
  this.createTextInput_();
};


/**
 * Sets the text before cursor.
 *
 * @param {string} text The text before cursor.
 */
goog.ime.chrome.vk.Controller.prototype.handleSurroundingTextChanged =
    function(text) {
  if (this.textInput_) {
    if (this.textInput_ instanceof goog.ime.chrome.vk.DirectTextInput) {
      this.textInput_.textBeforeCursor = text.slice(-20);
    }
  }
};


/**
 * Activates the vk via input tool.
 *
 * @param {string} engineId The engine ID which contains layout code.
 */
goog.ime.chrome.vk.Controller.prototype.activate = function(engineId) {
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
goog.ime.chrome.vk.Controller.prototype.deactivate = function() {
  this.engineID_ = '';
  this.layout_ = null;
  this.state_ = 0;
  this.setState(goog.ime.chrome.vk.Controller.StateBit.ALTGR, false);

  if (this.winId_ != 0) {
    chrome.windows.remove(this.winId_);
    this.winId_ = 0;
  }
};


/**
 * Creates the window for keyboard view.
 */
goog.ime.chrome.vk.Controller.prototype.createView = function() {
  if (this.winId_ > 0) {
    return;
  }
  var self = this;
  chrome.windows.create({
    'url': 'view.html',
    'width': 535,
    'height': 230,
    'type': 'panel'
  }, function(win) {
    self.winId_ = win.id;
  });
};


/**
 * Handler for layout loaded.
 *
 * @param {!goog.ime.chrome.vk.LayoutEvent} e The event of layout loaded.
 * @private
 */
goog.ime.chrome.vk.Controller.prototype.onLayoutLoaded_ = function(e) {
  this.layout_ = /** @type {!goog.ime.chrome.vk.ParsedLayout} */ (e.layoutView);
  this.model_.activateLayout(this.layout_.id);
  this.createTextInput_();
  if (this.port_ && this.layout_) {
    this.port_.postMessage({
      'type': 'layout',
      'layout': this.layout_.view
    });
  }
};


/**
 * Resets the text input.
 */
goog.ime.chrome.vk.Controller.prototype.resetTextInput = function() {
  if (this.textInput_) {
    this.textInput_.reset();
  }
};


/**
 * Handles the onReset event.
 */
goog.ime.chrome.vk.Controller.prototype.handleReset = function() {
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
goog.ime.chrome.vk.Controller.prototype.getUiState_ = function() {
  var uiState = '';
  var states = goog.ime.chrome.vk.Controller.StateBit;
  if (this.state_ & states.SHIFT ||
      this.state_ & states.SHIFT_CLICK) {
    uiState += 's';
  }
  if (this.state_ & states.ALTGR ||
      this.state_ & states.ALTGR_CLICK) {
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
goog.ime.chrome.vk.Controller.prototype.updateState_ = function(e) {
  var codes = goog.events.KeyCodes;
  var states = goog.ime.chrome.vk.Controller.StateBit;

  // Initial state needs to carry the current capslock and click status.
  var state = (this.state_ & (states.CAPS | states.SHIFT_CLICK |
                              states.ALTGR_CLICK));
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
  if (e.keyCode == codes.ALT || e.altKey) {
    state |= states.ALT;
  }
  if (e.keyCode == codes.CTRL || e.ctrlKey) {
    state |= states.CTRL;
  }

  if (this.state_ != state) {
    this.state_ = state;
    if (this.port_) {
      this.port_.postMessage({
        'type': 'state',
        'state': this.getUiState_()
      });
    }
  }
};


/**
 * Handles the keydown events.
 *
 * @param {goog.events.BrowserEvent} e The key event.
 * @return {boolean} False to continue handling, True otherwise.
 */
goog.ime.chrome.vk.Controller.prototype.processEvent = function(e) {
  if (!this.textInput_ || !this.layout_) {
    return false;
  }

  this.updateState_(e);

  // Check hot key.
  var states = goog.ime.chrome.vk.Controller.StateBit;
  if (e.ctrlKey || e.altKey && !(this.state_ & states.ALTGR)) {
    return false;
  }

  if (this.port_) {
    this.port_.postMessage({
      'type': 'click',
      'click': e.keyCode
    });
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
goog.ime.chrome.vk.Controller.prototype.commitChars_ = function(keyCode) {
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
    var keyCodes = is102 ? goog.ime.chrome.vk.KeyCode.CODES102 :
        goog.ime.chrome.vk.KeyCode.CODES101;
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
goog.ime.chrome.vk.Controller.prototype.disposeInternal = function() {
  goog.dispose(this.model_);
  goog.dispose(this.eventHandler_);

  goog.base(this, 'disposeInternal');
};
