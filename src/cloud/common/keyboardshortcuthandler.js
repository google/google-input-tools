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
 * @fileoverview The keyboard shortcut handler.
 */


goog.provide('i18n.input.common.KeyboardShortcutHandler');


goog.require('goog.events.EventHandler');
goog.require('goog.events.EventType');
goog.require('goog.events.KeyCodes');
goog.require('goog.ui.KeyboardShortcutEvent');
goog.require('goog.ui.KeyboardShortcutHandler');
goog.require('goog.userAgent');
goog.require('i18n.input.common.GlobalSettings');



/**
 * Component for handling keyboard shortcuts, closure basic class
 * goog.ui.KeyboardShortcutHandler can't handle "shift" or "ctrl+shift"
 * serial keyboard shortcuts. i18n.input.common.ShortcutHandler extends its
 * feature to any combined keyboard shortcuts of command key.
 *
 * @param {goog.events.EventTarget|EventTarget} keyTarget Event target that the
 *     key event listener is attached to, typically the applications root
 *     container.
 * @constructor
 * @extends {goog.ui.KeyboardShortcutHandler}
 */
i18n.input.common.KeyboardShortcutHandler = function(keyTarget) {

  /**
   * Saves additional key combination value.
   *
   * @type {!Object}
   * @private
   */
  this.keys_ = {};

  /**
   * Saves keycode and modifiers' combination value when last key down.
   *
   * @type {number}
   * @private
   */
  this.lastKeydownValue_ = 0;

  /**
   * The event handler.
   *
   * @type {!goog.events.EventHandler}
   * @private
   */
  this.eventHandler_ = new goog.events.EventHandler(this);

  goog.base(this, keyTarget);
};
goog.inherits(i18n.input.common.KeyboardShortcutHandler,
    goog.ui.KeyboardShortcutHandler);


/** @override */
i18n.input.common.KeyboardShortcutHandler.prototype.registerShortcut =
    function(identifier, var_args) {
  if (goog.isString(arguments[1])) {
    // Only support string type shortcut like "shift" or "ctrl+shift" in
    // extension but not "shift, ctrl+shift".
    var keyValue = this.getSpecialKeyValue_(arguments[1]);
    if (keyValue) {
      this.keys_[keyValue] = identifier;
      return;
    }
  }
  i18n.input.common.KeyboardShortcutHandler.superClass_.
      registerShortcut.apply(this, arguments);
};


/** @override */
i18n.input.common.KeyboardShortcutHandler.prototype.unregisterShortcut =
    function(var_args) {
  var shortcut = arguments[0];
  if (goog.isString(shortcut)) {
    // Only support string type shortcut like "shift" or "ctrl+shift" in
    // extension but not "shift, ctrl+shift".
    var keyValue = this.getSpecialKeyValue_(shortcut);
    if (keyValue && this.keys_[keyValue]) {
      this.keys_[keyValue] = null;
      return;
    }
  }
  i18n.input.common.KeyboardShortcutHandler.superClass_.
      unregisterShortcut.apply(this, arguments);
};


/** @override */
i18n.input.common.KeyboardShortcutHandler.prototype.isShortcutRegistered =
    function(var_args) {
  if (goog.isString(arguments[0])) {
    var keyValue = this.getSpecialKeyValue_(arguments[0]);
    if (keyValue && this.keys_[keyValue]) {
      return true;
    }
  }
  return i18n.input.common.KeyboardShortcutHandler.superClass_.
      isShortcutRegistered.apply(this, arguments);
};


/** @override **/
i18n.input.common.KeyboardShortcutHandler.prototype.initializeKeyListener =
    function(keyTarget) {
  i18n.input.common.KeyboardShortcutHandler.superClass_.
      initializeKeyListener.call(this, keyTarget);

  // Hacks for chrome extension. Adds listener on capture phrase to set
  // keyboard shortcut high priority.
  this.eventHandler_.listen(keyTarget, goog.events.EventType.KEYDOWN,
      this.handleSpecialKeyDown_,
      i18n.input.common.GlobalSettings.canListenInCaptureForIE8);
  this.eventHandler_.listen(keyTarget, goog.events.EventType.KEYPRESS,
      this.handleSpecialKeyPress_,
      i18n.input.common.GlobalSettings.canListenInCaptureForIE8);
  this.eventHandler_.listen(keyTarget, goog.events.EventType.KEYUP,
      this.handleSpecialKeyUp_,
      i18n.input.common.GlobalSettings.canListenInCaptureForIE8);
};


/**
 * Handles key down events, save the key code and modifiers combination value.
 *
 * @param {goog.events.BrowserEvent} e The event.
 * @private
 */
i18n.input.common.KeyboardShortcutHandler.prototype.handleSpecialKeyDown_ =
    function(e) {
  var keyCode = goog.userAgent.GECKO ?
      goog.events.KeyCodes.normalizeGeckoKeyCode(e.keyCode) :
      e.keyCode;

  var modifiers = this.getModifiers_(e);
  this.lastKeydownValue_ = keyCode & 255 | modifiers << 8;
  var shortcut = this.keys_[this.lastKeydownValue_];
  // Don't eat keydown event for modifier keys, because other code needs to
  // handle it. For example, virtual keyboard needs to handle SHIFT and ALTGR.
  // See b/8785756.
  if (shortcut && keyCode != goog.events.KeyCodes.SHIFT &&
      keyCode != goog.events.KeyCodes.CTRL &&
      keyCode != goog.events.KeyCodes.ALT) {
    e.preventDefault();
    e.stopPropagation();
  }
};


/**
 * Handles key press events, because even when keydown has been
 * preventDefault'ed, some browser (e.g. FireFox) still issue keypress.
 *
 * @param {goog.events.BrowserEvent} e The event.
 * @private
 */
i18n.input.common.KeyboardShortcutHandler.prototype.handleSpecialKeyPress_ =
    function(e) {
  var shortcut = this.keys_[this.lastKeydownValue_];
  if (shortcut) {
    e.preventDefault();
    e.stopPropagation();
  }
};


/**
 * Handles key up event, check whether the registered keys was pressed, fire
 * the events.
 *
 * @param {goog.events.BrowserEvent} e The event.
 * @private
 */
i18n.input.common.KeyboardShortcutHandler.prototype.handleSpecialKeyUp_ =
    function(e) {
  var keyCode = goog.userAgent.GECKO ?
      goog.events.KeyCodes.normalizeGeckoKeyCode(e.keyCode) :
      e.keyCode;

  var modifiers = this.getModifiers_(e);

  var keyValue = keyCode & 255 | modifiers << 8;
  if (keyValue == this.lastKeydownValue_) {
    var shortcut = this.keys_[keyValue];
    if (shortcut) {
      var types = goog.ui.KeyboardShortcutHandler.EventType;
      // Dispatch SHORTCUT_TRIGGERED event
      var target = /** @type {Node} */ (e.target);
      var triggerEvent = new goog.ui.KeyboardShortcutEvent(
          types.SHORTCUT_TRIGGERED, shortcut, target);
      this.dispatchEvent(triggerEvent);

      // Dispatch SHORTCUT_PREFIX_<identifier> event
      var prefixEvent = new goog.ui.KeyboardShortcutEvent(
          types.SHORTCUT_PREFIX + shortcut, shortcut, target);
      this.dispatchEvent(prefixEvent);
    }
  }
  // Clear stored last key value
  this.lastKeydownValue_ = 0;
};


/**
 * Gets the modifiers of browser event.
 *
 * @param {goog.events.BrowserEvent} e The Event.
 * @return {number} The modifiers combination value.
 * @private
 */
i18n.input.common.KeyboardShortcutHandler.prototype.getModifiers_ =
    function(e) {
  return ((e.shiftKey || e.keyCode == goog.events.KeyCodes.SHIFT) ?
      goog.ui.KeyboardShortcutHandler.Modifiers.SHIFT : 0) |
      ((e.ctrlKey || e.keyCode == goog.events.KeyCodes.CTRL) ?
      goog.ui.KeyboardShortcutHandler.Modifiers.CTRL : 0) |
      ((e.altKey || e.keyCode == goog.events.KeyCodes.ALT) ?
      goog.ui.KeyboardShortcutHandler.Modifiers.ALT : 0) |
      ((e.metaKey || e.keyCode == goog.events.KeyCodes.META) ?
      goog.ui.KeyboardShortcutHandler.Modifiers.META : 0);
};


/**
 * Gets the special short cut key value.
 *
 * @param {string} s String representation of shortcut.
 * @return {number|undefined} The key value for this shortcut if it exsits.
 * @private
 */
i18n.input.common.KeyboardShortcutHandler.prototype.getSpecialKeyValue_ =
    function(s) {
  var strokes = goog.ui.KeyboardShortcutHandler.parseStringShortcut(s);
  if (strokes.length == 1) {
    var stroke = strokes[0];
    var keyCode = stroke.keyCode;
    if (!keyCode &&
        goog.ui.KeyboardShortcutHandler.Modifiers.SHIFT & stroke.modifiers) {
      // Any shortcut without keyCode should be handled in keyup, so it won't
      // conflict with shortcuts with keyCode which is handled in keydown in
      // closure handler.
      if (goog.ui.KeyboardShortcutHandler.Modifiers.SHIFT &
          stroke.modifiers) {
        stroke.keyCode = goog.events.KeyCodes.SHIFT;
      } else if (goog.ui.KeyboardShortcutHandler.Modifiers.ALT &
          stroke.modifiers) {
        stroke.keyCode = goog.events.KeyCodes.ALT;
      } else if (goog.ui.KeyboardShortcutHandler.Modifiers.META &
          stroke.modifiers) {
        stroke.keyCode = goog.events.KeyCodes.META;
      } else {
        stroke.keyCode = goog.events.KeyCodes.CTRL;
      }
      return (stroke.keyCode & 255) | (stroke.modifiers << 8);
    }
  }
};


/** @override */
i18n.input.common.KeyboardShortcutHandler.prototype.disposeInternal =
    function() {
  goog.base(this, 'disposeInternal');

  goog.dispose(this.eventHandler_);
  delete this.eventHandler_;

  delete this.keys_;
};
