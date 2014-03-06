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
 * @fileoverview The controller class of virtual keyboard.
 *     It is singleton and creates KeyboardLayout & KeyboardView. It handles the
 *     UI events like button click, mouse dragging, etc., and also handles key
 *     down/press/up events passed from keyboard plugin.
 *     This is the Controller of MVC pattern.
 */


goog.provide('i18n.input.keyboard.Controller');


goog.require('goog.dom');
goog.require('goog.events.Event');
goog.require('goog.events.EventHandler');
goog.require('goog.events.EventTarget');
goog.require('goog.events.EventType');
goog.require('goog.events.KeyCodes');
goog.require('goog.math.Box');
goog.require('goog.style');
goog.require('goog.ui.Component');
goog.require('goog.userAgent');
goog.require('i18n.input.common.Metrics');
goog.require('i18n.input.common.Statistics');
goog.require('i18n.input.keyboard.EventType');
goog.require('i18n.input.keyboard.KeyCode');
goog.require('i18n.input.keyboard.Model');
goog.require('i18n.input.keyboard.View');



/**
 * Creates the Controller object.
 *
 * @param {string=} opt_keyboardHelpUrl The keyboard help page url.
 * @param {boolean=} opt_isIframeWrapper Whether UI is wrappered by iframe.
 * @constructor
 * @extends {goog.events.EventTarget}
 */
i18n.input.keyboard.Controller =
    function(opt_keyboardHelpUrl, opt_isIframeWrapper) {
  goog.base(this);

  /**
   * The keyboard module object.
   *
   * @type {!i18n.input.keyboard.Model}
   * @private
   */
  this.model_ = new i18n.input.keyboard.Model();

  /**
   * The dom helper.
   *
   * @type {goog.dom.DomHelper}
   * @private
   */
  this.domHelper_ = goog.dom.getDomHelper();

  /**
   * The last keydown handler's result. Controller doesn't handle
   * keypress event, but need to return the same return value as keydown
   * event. Considering 2 scenarios:
   * 1) when press 'a', onKeyDown returns true, so onKeyPress should return
   * true to prevent the browser commits additional 'a'.
   * 2) when press BS, onKeyDown returns false, so onKeyPress should return
   * false because the default browser can take default actions, which
   * may not be with keydown, but could be with keypress.
   * The object format is {<keycode>: <keydown result>}.
   * this.keyDownResults_[0] is the keydown result, no matter what key code
   * it is.
   *
   * @type {!Object.<boolean>}
   * @private
   */
  this.keyDownResults_ = {};

  /**
   * The event handler.
   *
   * @type {!goog.events.EventHandler}
   * @private
   */
  this.eventHandler_ = new goog.events.EventHandler(this);

  /**
   * The statistics session object.
   *
   * @type {!i18n.input.common.AbstractStatSession}
   * @private
   */
  this.statSession_ = i18n.input.common.Statistics.getInstance().getSession(
      i18n.input.common.Metrics.Type.VIRTUAL_KEYBOARD);

  /**
   * The separators for counting words.
   * It is necessary to judge whether a char is a separator in O(1) time.
   *
   * @type {Object.<string, boolean>}
   * @private
   */
  this.wordSeparators_ = {};

  /**
   * The map of layout id to layout title.
   *
   * @type {!Object.<string, string>}
   * @private
   */
  this.titleMap_ = {};

  /**
   * The keyboard help page url.
   *
   * @type {string}
   * @private
   */
  this.keyboardHelpUrl_ = opt_keyboardHelpUrl || '';

  /**
   * Whether view's UI is wrappered by iframe.
   *
   * @type {boolean}
   * @private
   */
  this.isIframeWraper_ = !!opt_isIframeWrapper;

  this.model_.setParentEventTarget(this);
  this.eventHandler_.listen(this.model_,
      i18n.input.keyboard.EventType.LAYOUT_ACTIVATED, this.onLayoutActivated_);

  // Initializes the word separator hash table.
  for (var i = 0, c;
       c = i18n.input.keyboard.Controller.WORD_SEPARATORS_.charAt(i); ++i) {
    this.wordSeparators_[c] = true;
  }

  // Installs the CSS styles.
  if (i18n.input.keyboard.Controller.Css) {
    goog.style.installStyles(
        i18n.input.keyboard.Controller.Css);
    i18n.input.keyboard.Controller.Css = ''; // Avoid duplicated install.
  }
};
goog.inherits(i18n.input.keyboard.Controller, goog.events.EventTarget);


/**
 * The inputable object.
 *
 * @type {i18n.input.common.Inputable}
 * @private
 */
i18n.input.keyboard.Controller.prototype.inputable_ = null;


/**
 * Whether listen to the events.
 *
 * @type {boolean}
 * @private
 */
i18n.input.keyboard.Controller.prototype.listening_ = true;


/**
 * The vk state controled by key modifiers. And it consists of the bit flags
 * i18n.input.keyboard.Controller.StateBit.
 *
 * @type {number}
 * @private
 */
i18n.input.keyboard.Controller.prototype.state_ = 0;


/**
 * The active view.
 *
 * @type {i18n.input.keyboard.View}
 * @private
 */
i18n.input.keyboard.Controller.prototype.activeView_ = null;


/**
 * The oem key map id. Support values are 'de' and 'fr'.
 *
 * @type {string}
 * @private
 */
i18n.input.keyboard.Controller.prototype.oemId_ = 'en';


/**
 * The recorded anchored position for repositionToAnchor.
 *
 * @type {goog.positioning.AnchoredPosition}
 * @private
 */
i18n.input.keyboard.Controller.prototype.anchoredPosition_ = null;


/**
 * The recorded anchored corner for repositionToAnchor.
 *
 * @type {?goog.positioning.Corner}
 * @private
 */
i18n.input.keyboard.Controller.prototype.anchoredCorner_ = null;


/**
 * The recorded anchored margin for repositionToAnchor.
 *
 * @type {goog.math.Box}
 * @private
 */
i18n.input.keyboard.Controller.prototype.anchoredMargin_ = null;


/**
 * The recorded visibility of the keyboard. It could be set before the
 * active view is set, so it needs to be recorded. The default value is
 * true.
 *
 * @type {boolean}
 * @private
 */
i18n.input.keyboard.Controller.prototype.visible_ = true;


/**
 * Whether pin SHIFT key button.
 *
 * @type {boolean}
 * @private
 */
i18n.input.keyboard.Controller.prototype.pinShift_ = false;


/**
 * Whether pin ALTGR key button.
 *
 * @type {boolean}
 * @private
 */
i18n.input.keyboard.Controller.prototype.pinAltGr_ = false;


/**
 * The word separators.
 *
 * @type {string}
 * @private
 * @const
 */
i18n.input.keyboard.Controller.WORD_SEPARATORS_ =
    ' \u00a0\u000a\u000d`~!@#$%^&*()_+-=[]{}\\|;:\'",./<>?';


/**
 * The CSS JS string.
 *
 * @type {string}
 */
i18n.input.keyboard.Controller.Css = '';


/**
 * The state bits.
 *
 * @enum {number}
 * @const
 */
i18n.input.keyboard.Controller.StateBit = {
  SHIFT: 1,
  ALT: 2,
  CTRL: 4,
  CAPS: 8,
  META: 16,
  SHIFT_CLICK: 256,
  ALTGR_CLICK: 512
};


/**
 * Gets/creates the Controller object. This is the getInstance method
 * in singleton pattern.
 *
 * @return {i18n.input.keyboard.Controller} The controller instance.
 */
i18n.input.keyboard.Controller.getInstance = function() {
  if (!i18n.input.keyboard.Controller.INSTANCE) {
    i18n.input.keyboard.Controller.INSTANCE = new
        i18n.input.keyboard.Controller();
  }
  return i18n.input.keyboard.Controller.INSTANCE;
};


/**
 * Sets the inputable object.
 *
 * @param {i18n.input.common.Inputable} inputable The input manager.
 */
i18n.input.keyboard.Controller.prototype.setInputable = function(
    inputable) {
  this.inputable_ = inputable;
};


/**
 * Sets the dom helper for keyboard.
 *
 * @param {goog.dom.DomHelper} domHelper The dom helper.
 */
i18n.input.keyboard.Controller.prototype.setDomHelper = function(
    domHelper) {
  this.domHelper_ = domHelper;
};


/**
 * Loads the layout.
 *
 * @param {string} layoutCode The layout code.
 */
i18n.input.keyboard.Controller.prototype.loadLayout = function(
    layoutCode) {
  this.model_.loadLayout(layoutCode);
};


/**
 * Activates the layout, which will notify KeyboardView to update the ui.
 *
 * @param {string} layoutCode The layout code.
 * @param {string=} opt_title The optional title for the layout. If not
 *     specified, use the title defined by layout itsefl.
 */
i18n.input.keyboard.Controller.prototype.activateLayout = function(
    layoutCode, opt_title) {
  if (opt_title) {
    this.titleMap_[layoutCode] = opt_title;
  }
  this.model_.activateLayout(layoutCode);
};


/**
 * Sets the listening of the keyboard.
 *
 * @param {boolean} listening True to listen the key event, false otherwise.
 */
i18n.input.keyboard.Controller.prototype.setListening = function(listening) {
  if (this.activeView_) {
    // Records the metrics info when listening state is changing.
    if (!this.listening_ && listening) {
      this.statSession_.begin(i18n.input.common.Metrics.Param.SHOW_TIME);
    } else if (this.listening_ && !listening) {
      this.statSession_.set(i18n.input.common.Metrics.Param.ACTION,
          i18n.input.common.Metrics.Action.CLOSE);
      this.statSession_.report();
    }
  }
  this.listening_ = listening;
};


/**
 * Sets the visibility of the keyboard.
 *
 * @param {boolean} visible True for show and false for hide.
 */
i18n.input.keyboard.Controller.prototype.setVisible = function(visible) {
  this.visible_ = visible;
  if (this.activeView_) {
    if (!visible) {
      this.state_ = 0; // Reset the state when hide the vk.
      this.activeView_.refreshState(this.getUiState_());
    }
    this.activeView_.setVisible(visible);
  }
};


/**
 * Sets the oem id.
 *
 * @param {string} oemId The oem id.
 */
i18n.input.keyboard.Controller.prototype.setOemId = function(oemId) {
  this.oemId_ = oemId;
};


/**
 * Enables/disables pin modifier key buttons, including SHIFT and ALTGR.
 *
 * @param {boolean} pinShift True to pin SHIFT key button, false otherwise.
 * @param {boolean} pinAltGr True to pin ALTGR key button, false otherwise.
 */
i18n.input.keyboard.Controller.prototype.enablePinModifierKeys = function(
    pinShift, pinAltGr) {
  this.pinShift_ = pinShift;
  this.pinAltGr_ = pinAltGr;
};


/**
 * Repositions the keyboard to the anchored position.
 *
 * @param {!goog.positioning.AnchoredPosition} anchoredPosition The anchored
 *     position.
 * @param {!goog.positioning.Corner} corner The corner of the keyboard.
 * @param {goog.math.Box=} opt_margin The margin of the alignment.
 */
i18n.input.keyboard.Controller.prototype.reposition = function(
    anchoredPosition, corner, opt_margin) {
  if (this.activeView_) {
    this.activeView_.repositionToAnchor(anchoredPosition, corner, opt_margin);
  } else {
    // If no view at present, record the necessary info so that the view could
    // be reposition'ed when it's activated.
    this.anchoredPosition_ = anchoredPosition;
    this.anchoredCorner_ = corner;
    this.anchoredMargin_ = opt_margin || new goog.math.Box(0, 0, 0, 0);
  }
};


/**
 * Gets the direction of the active layout.
 *
 * @return {string} The direction, Either 'rtl' or 'ltr'.
 */
i18n.input.keyboard.Controller.prototype.getDirection = function() {
  return (this.activeView_ && this.activeView_.isRTL()) ? 'rtl' : 'ltr';
};


/**
 * Handles the ui events.
 * Standard Behaviors:
 *   Visible Keys: once: d/p/u; repeat: d/p/d/p/.../u;
 *     d/u:keyCode=key,charCode=0;
 *     p:keyCode=charCode=char;
 *   Invisible Keys (BS/ARROW): once: d/u; repeat: d/d/.../u;
 *     Modifier Keys: once: d/u; repeat: N/A;
 *     Capslock Key: once: d/u; repeat: N/A;
 *   Exceptions:
 *     Firefox, Invisible Keys: once: d/p/u; repeat: d/p/d/p/.../u;
 *     Firefox/Chrome/Mac, Capslock Key: once: d; once again: u;
 *     IE8, charCode is always undefined.
 *     Firefox, p:charCode=char,keyCode=0.
 *     Firefox/Mac, d/u:charCode=keyCode=0 when SHIFT+[`-\;,./].
 *   Note: d means keydown, p means keypress, u means keyup.
 *
 * @param {goog.events.BrowserEvent} e The ui event.
 * @return {boolean} False to continue handling, True otherwise.
 * */
i18n.input.keyboard.Controller.prototype.handleEvent = function(e) {
  if (!this.listening_ || !this.activeView_) {
    return false;
  }

  var ret = false;

  switch (e.type) {
    case goog.events.EventType.KEYDOWN:
      ret = this.onKeyDown_(e);
      break;
    case goog.events.EventType.KEYPRESS:
      ret = this.onKeyPress_(e);
      break;
    case goog.events.EventType.KEYUP:
      ret = this.onKeyUp_(e);
      break;
  }

  return ret;
};


/**
 * Handler for close keyboard.
 *
 * @private
 */
i18n.input.keyboard.Controller.prototype.onKeyboardClosed_ =
    function() {
  this.setVisible(false);
};


/**
 * Handler for button click.
 *
 * @param {goog.events.Event} e The ACTION event from View.
 * @private
 */
i18n.input.keyboard.Controller.prototype.onKeyButtonClicked_ = function(e) {
  if (!this.activeView_) return;

  if (this.inputable_) {
    this.inputable_.setFocus();
  }

  var keyCode = e.target.keyCode;
  if (!keyCode) return;

  var codes = goog.events.KeyCodes;
  var states = i18n.input.keyboard.Controller.StateBit;
  switch (keyCode) {
    case codes.SHIFT:
      // If shift is on, copies shift to shift click, and clear shift state.
      if (this.state_ & states.SHIFT) {
        this.state_ |= states.SHIFT_CLICK;
        this.state_ &= ~states.SHIFT;
      }
      this.state_ ^= states.SHIFT_CLICK;
      this.activeView_.refreshState(this.getUiState_());
      break;
    case 273: // Key code 273 means ALTGR.
      // If altgr is on, copies altgr to altgr click, and clear altgr state.
      if ((this.state_ & states.ALT) && (this.state_ & states.CTRL)) {
        this.state_ |= states.ALTGR_CLICK;
        this.state_ &= ~(states.ALT | states.CTRL);
      }
      this.state_ ^= states.ALTGR_CLICK;
      this.activeView_.refreshState(this.getUiState_());
      break;
    case codes.CAPS_LOCK:
      this.state_ ^= states.CAPS;
      this.activeView_.refreshState(this.getUiState_());
      break;
    default:
      this.commitChars_(keyCode);
      break;
  }

  this.statSession_.increase(i18n.input.common.Metrics.Param.
      VK_KEY_CLICK_COUNT);
};


/**
 * Handler for layout activated.
 *
 * @param {!i18n.input.keyboard.LayoutEvent} e The event of layout activated.
 * @private
 */
i18n.input.keyboard.Controller.prototype.onLayoutActivated_ = function(e) {
  var view = e.layoutView;
  view.title = this.titleMap_[view.id] || view.title;
  var anchoredPosition = this.anchoredPosition_;
  var anchoredCorner = this.anchoredCorner_;
  var anchoredMargin = this.anchoredMargin_;

  // Resets recorded data so it's used only once.
  this.anchoredPosition_ = null;
  this.anchoredCorner_ = null;
  this.anchoredMargin_ = null;

  var isDragged = false;

  var lastViewPosition = null;
  if (this.activeView_) {
    this.visible_ = this.activeView_.isVisible();
    isDragged = this.activeView_.isDragged();
    // Uses the old view's position if no position was set via reposition().
    // If the old view is hidden, use the default position.
    if (this.activeView_.isVisible() && isDragged) {
      lastViewPosition = this.activeView_.getPosition();
    }
    this.activeView_.dispose();

    this.statSession_.set(i18n.input.common.Metrics.Param.ACTION,
        i18n.input.common.Metrics.Action.SWITCH);
    this.statSession_.report();
  }

  // Statistics.
  this.statSession_.setConst(i18n.input.common.Metrics.Param.VK_LAYOUT,
      view.id);
  this.statSession_.setInputToolCode('vkd_' + view.id);

  this.activeView_ = new i18n.input.keyboard.View(view, this.domHelper_,
      this.keyboardHelpUrl_, this.isIframeWraper_);
  this.activeView_.setParentEventTarget(this);
  this.activeView_.render();
  this.activeView_.setDragged(isDragged);
  // Inits the view as visible = false, later it will call this.setVisible() to
  // refresh its visibility. The reason to do this is to trigger the statistics
  // properly, because statistics will be recorded when visibility is changing.
  this.activeView_.setVisible(this.visible_);
  if (lastViewPosition) {
    this.activeView_.reposition(lastViewPosition);
  } else if (anchoredPosition && goog.isDefAndNotNull(anchoredCorner)) {
    this.activeView_.repositionToAnchor(
        anchoredPosition, anchoredCorner, anchoredMargin);
    // If the keyboard was repositioned by API, make its position sticky.
    this.activeView_.setDragged(true);
  } else {
    // Repositions to the default position.
    this.activeView_.reposition();
  }
  this.activeView_.refreshState(this.getUiState_());

  this.eventHandler_
      .listen(this.activeView_, goog.ui.Component.EventType.ACTION,
          this.onKeyButtonClicked_)
      .listen(this.activeView_,
          [i18n.input.keyboard.EventType.KEYBOARD_CLOSED,
           i18n.input.keyboard.EventType.KEYBOARD_MINIMIZED],
          this.onKeyboardClosed_);
};


/**
 * Handles the keydown events.
 *
 * @param {goog.events.BrowserEvent} e The keydown event.
 * @return {boolean} False to continue handling, True otherwise.
 * @private
 */
i18n.input.keyboard.Controller.prototype.onKeyDown_ = function(e) {
  var codes = goog.events.KeyCodes;
  var states = i18n.input.keyboard.Controller.StateBit;
  var code = this.normalizeKeyCode_(e.keyCode);

  this.updateState_(code, e);

  // Don't handle meta key or keys in meta state.
  if (this.state_ & states.META) {
    return this.keyDownReturn_(code, false);
  }

  var shift = !!(this.state_ & states.SHIFT);
  var ctrl = !!(this.state_ & states.CTRL);
  var alt = !!(this.state_ & states.ALT);
  var meta = !!(this.state_ & states.META);

  if (code == codes.CTRL || code == codes.ALT ||
      code == codes.SHIFT || code == codes.CAPS_LOCK) {
    // Returns false for SHIFT, ALT, CTRL, because hotkey needs it.
    // For CAPS_LOCK, returns true because it listens on both document and input
    // box, so we need to prevent duplicated CAPS_LOCK down events.
    return this.keyDownReturn_(code, code == codes.CAPS_LOCK);
  }

  // Check hot key.
  if (meta || ctrl != alt) {
    return this.keyDownReturn_(code, this.onHotKey_(code));
  }

  this.statSession_.increase(i18n.input.common.Metrics.Param.VK_KEY_KEY_COUNT);

  this.activeView_.visualizeClickButton(code);

  return this.keyDownReturn_(code, this.commitChars_(code));
};


/**
 * Handles the keypress events. It's functional only for Firefox on Mac.
 * Notice: Firefox/Mac, d/u:charCode=keyCode=0 when SHIFT+[`-\;,./].
 *
 * @param {goog.events.BrowserEvent} e The keypress event.
 * @return {boolean} False to continue handling, True otherwise.
 * @private
 */
i18n.input.keyboard.Controller.prototype.onKeyPress_ = function(e) {
  // Returns true if the last keydown returns true, because keypress is always
  // right after keydown event.
  if (this.keyDownResults_[0]) {
    return true;
  }
  // If Firefox/Mac/SHIFT, get key code from char code and then perform the
  // action.
  if (goog.userAgent.MAC && goog.userAgent.GECKO) {
    var code = i18n.input.keyboard.KeyCode.MOZ_SHIFT_CHAR_CODES[e.charCode];
    if (code) {
      return this.commitChars_(code);
    }
  }
  // Returns the last keydown handler's result, because keypress is always right
  // after keydown event.
  return this.keyDownResults_[0];
};


/**
 * Handles the keyup events. This function has no return value because the
 * handleEvent will return the last keydown result.
 *
 * @param {goog.events.BrowserEvent} e The keyup event.
 * @return {boolean} False to continue handling, True otherwise.
 * @private
 */
i18n.input.keyboard.Controller.prototype.onKeyUp_ = function(e) {
  var codes = goog.events.KeyCodes;
  var states = i18n.input.keyboard.Controller.StateBit;
  var code = this.normalizeKeyCode_(e.keyCode);
  var state = this.state_;
  if (code == codes.CAPS_LOCK &&
      (goog.userAgent.MAC && goog.userAgent.WEBKIT)) {
    // Only handles CAPSLOCK in keyup for Chrome on Mac.
    state &= ~states.CAPS;
  } else if (code == codes.SHIFT) {
    state &= ~states.SHIFT;
    state &= ~states.SHIFT_CLICK;
  } else if (code == codes.ALT) {
    state &= ~states.ALT;
    state &= ~states.ALTGR_CLICK;
  } else if (code == codes.CTRL) {
    state &= ~states.CTRL;
    state &= ~states.ALTGR_CLICK;
  } else if (code == codes.META) {
    state &= ~states.META;
  }
  if (this.state_ != state) {
    this.state_ = state;
    this.activeView_.refreshState(this.getUiState_());
  }

  // Always return the same result as last keydown.
  var ret = this.keyDownResults_[code];
  delete this.keyDownResults_[code];
  return ret;
};


/**
 * Updates the state given the key code.
 *
 * @param {number} code The key code.
 * @param {goog.events.BrowserEvent} e The keydown event.
 * @private
 */
i18n.input.keyboard.Controller.prototype.updateState_ = function(
    code, e) {
  var codes = goog.events.KeyCodes;
  var states = i18n.input.keyboard.Controller.StateBit;

  // Initial state needs to carry the current capslock and click status.
  var state = (this.state_ & (states.CAPS | states.SHIFT_CLICK |
                              states.ALTGR_CLICK));
  // Normalizes capslock.
  // Chrome on Mac don't need special handle for Capslock, because it behaves
  // like: press it once, only keydown fired; press it again, only keyup fired.
  if (code == codes.CAPS_LOCK) {
    if (!goog.userAgent.MAC || !goog.userAgent.WEBKIT) {
      state ^= states.CAPS;
    } else {
      state |= states.CAPS;
    }
  }
  // Refreshes the modifier status.
  if (code == codes.SHIFT || e.shiftKey) {
    state |= states.SHIFT;
  }
  if (code == codes.ALT || e.altKey) {
    state |= states.ALT;
  }
  if (code == codes.CTRL || e.ctrlKey) {
    state |= states.CTRL;
  }
  if (code == codes.META || e.metaKey) {
    state |= states.META;
  }

  if (this.state_ != state) {
    this.state_ = state;
    this.activeView_.refreshState(this.getUiState_());
  }
};


/**
 * Records the return value of the keydown handler.
 *
 * @param {number} code The key code for keydown handler.
 * @param {boolean} ret The return value of the keydown handler.
 * @return {boolean} The keydown handler's return value.
 * @private
 */
i18n.input.keyboard.Controller.prototype.keyDownReturn_ = function(
    code, ret) {
  return this.keyDownResults_[0] = this.keyDownResults_[code] = ret;
};


/**
 * Commits the chars given the key code.
 *
 * @param {number} keyCode The key code.
 * @return {boolean} False to continue hanlding, True otherwise.
 * @private
 */
i18n.input.keyboard.Controller.prototype.commitChars_ = function(keyCode) {
  var codes = goog.events.KeyCodes;
  var states = i18n.input.keyboard.Controller.StateBit;

  if (!this.inputable_) return false;

  var chars = this.activeView_.getCommitChars(keyCode);

  // Collects statistics for word count, when the commit chars is like  a word
  // separator. The detailed conditions are:
  // if key mapping exists, and the chars is contained in the separator list,
  // it should be a seperator;
  // if key mapping not exists, and the original keychar is not in a-z or 0-9,
  // the chars must be a separator;
  // otherwise, the chars is not a separator.
  if (chars && !this.wordSeparators_[chars]) {
    if (!this.wordStarted_) {
      this.wordStarted_ = true;
      this.statSession_.increase(i18n.input.common.Metrics.Param.VK_WORD_COUNT);
    }
  } else if (this.wordStarted_ && keyCode != codes.BACKSPACE) {
    this.wordStarted_ = false;
  }
  // Updates the state if shift/altgr was clicked.
  if (this.state_ & states.SHIFT_CLICK || this.state_ & states.ALTGR_CLICK) {
    var state = this.state_;
    if (!this.pinShift_) {
      state &= ~states.SHIFT_CLICK;
    }
    if (!this.pinAltGr_) {
      state &= ~states.ALTGR_CLICK;
    }
    if (state != this.state_) {
      this.state_ = state;
      this.activeView_.refreshState(this.getUiState_());
    }
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
    var text = this.inputable_.getTextBeforeCursor(20) || '';
    if (keyCode == codes.BACKSPACE) {
      this.model_.processBackspace(text); // Maybe change history.
    } else {
      trans = this.model_.translate(/** @type {string} */ (chars), text);
    }
  } else if (chars) { // Not BS & non-transform layout.
    trans = {back: 0, chars: chars};
  }

  this.dispatchEvent(new goog.events.Event(
      i18n.input.keyboard.EventType.COMMIT_START));

  this.inputable_.commitText(trans.chars, trans.back);

  this.dispatchEvent(new goog.events.Event(
      i18n.input.keyboard.EventType.COMMIT_END));

  return true;
};


/**
 * Normalizes the key code across different browsers for virtual keyboard.
 *
 * @param {number} keyCode The original key code.
 * @return {number} The normalized key code.
 * @private
 */
i18n.input.keyboard.Controller.prototype.normalizeKeyCode_ = function(
    keyCode) {
  var code = keyCode;
  var codes = goog.events.KeyCodes;
  // On Linux, hold SHIFT+CTRL and press ALT, the key code is different than
  // goog.events.KeyCodes.ALT.
  if (goog.userAgent.LINUX && ((code == codes.META && goog.userAgent.WEBKIT) ||
      (code == codes.MAC_FF_META && goog.userAgent.GECKO))) {
    code = codes.ALT;
  }
  code = i18n.input.keyboard.KeyCode.MOZ_CODES[code] || code;
  var keyMap = i18n.input.keyboard.KeyCode.OEM_CODES[this.oemId_];
  if (keyMap) {
    code = keyMap[String.fromCharCode(code)] || code;
  }
  return code;
};


/**
 * Handles the hot key.
 *
 * @param {number} keyCode The key code.
 * @return {boolean} False to continue handling, True otherwise.
 * @private
 */
i18n.input.keyboard.Controller.prototype.onHotKey_ = function(keyCode) {
  return false;
};


/**
 * Gets the UI state from this.state_.
 *
 * @return {string} The UI state.
 * @private
 */
i18n.input.keyboard.Controller.prototype.getUiState_ = function() {
  var uiState = '';
  var states = i18n.input.keyboard.Controller.StateBit;
  if (this.state_ & states.SHIFT ||
      this.state_ & states.SHIFT_CLICK) {
    uiState += 's';
  }
  if ((this.state_ & states.ALT &&
      this.state_ & states.CTRL) ||
      this.state_ & states.ALTGR_CLICK) {
    uiState += 'c';
  }
  if (this.state_ & states.CAPS) {
    uiState += 'l';
  }
  return uiState;
};


/** @override */
i18n.input.keyboard.Controller.prototype.disposeInternal = function() {
  goog.dispose(this.activeView_);
  goog.dispose(this.model_);
  goog.dispose(this.statSession_);
  goog.dispose(this.eventHandler_);

  goog.base(this, 'disposeInternal');
};
