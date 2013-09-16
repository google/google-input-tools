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
 * @fileoverview Definition of the keyboard view.
 */

goog.provide('goog.ime.chrome.vk.View');

goog.require('goog.Disposable');
goog.require('goog.dom');
goog.require('goog.dom.TagName');
goog.require('goog.dom.classes');
goog.require('goog.events.EventHandler');
goog.require('goog.events.EventType');
goog.require('goog.events.KeyCodes');
goog.require('goog.ime.chrome.vk.KeyCode');
goog.require('goog.style');



/**
 * The keyboard view which can build the vk UI.
 *
 * @param {Object} view The vk layout view object.
 *     See ParsedLayout.view for its format.
 * @constructor
 * @extends {goog.Disposable}
 */
goog.ime.chrome.vk.View = function(view) {
  goog.base(this);

  /**
   * The map of {keycode: button}. For keycode==0, the related button is the
   *     close button..
   *
   * @type {Object.<number, Element>}
   * @private
   */
  this.buttons_ = {};

  /**
   * The dom helper.
   *
   * @type {!goog.dom.DomHelper}
   * @private
   */
  this.dom_ = new goog.dom.getDomHelper();

  /**
   * The event handler.
   *
   * @type {!goog.events.EventHandler}
   * @private
   */
  this.eventHandler_ = new goog.events.EventHandler(this);
};
goog.inherits(goog.ime.chrome.vk.View, goog.Disposable);


/**
 * The active state, which is one of '','s','c','l','sc','cl','sl','scl'.
 *
 * @type {string}
 * @private
 */
goog.ime.chrome.vk.View.prototype.state_ = '';


/**
 * @type {Port}
 * @private
 */
goog.ime.chrome.vk.View.prototype.port_ = null;


/**
 * The vk layout view object. See ParsedLayout.view for its format.
 *
 * @type {Object}
 * @private
 */
goog.ime.chrome.vk.View.prototype.view_ = null;


/**
 * The container element of the UI.
 *
 * @type {Element}
 * @private
 */
goog.ime.chrome.vk.View.prototype.container_ = null;


/**
 * The close button.
 *
 * @type {Element}
 * @private
 */
goog.ime.chrome.vk.View.prototype.closeBtn_ = null;


/**
 * The content panel element which contains all the key buttons.
 *
 * @type {Element}
 * @private
 */
goog.ime.chrome.vk.View.prototype.contentPanel_ = null;


/**
 * The active button, which is for key button UI :active effect.
 *
 * @type {Element}
 * @private
 */
goog.ime.chrome.vk.View.prototype.activeBtn_ = null;


/**
 * The hovered button, which is for key button UI :hover effect.
 *
 * @type {Element}
 * @private
 */
goog.ime.chrome.vk.View.prototype.hoverBtn_ = null;


/**
 * The base url for the layout button images.
 *
 * @type {string}
 * @private
 */
goog.ime.chrome.vk.View.BTN_IMG_DIR_ = 'images/';


/**
 * The width of each key button in scale 1.
 *
 * @type {number}
 * @private
 */
goog.ime.chrome.vk.View.prototype.keyButtonWidth_ = 33;


/**
 * The millisecs for visualize click button lag.
 *
 * @type {number}
 * @private
 */
goog.ime.chrome.vk.View.VISUALIZE_CLICK_BUTTON_LAG_ = 250;


/**
 * CSS used for virtual keyboard.
 *
 * @enum {string}
 * @private
 */
goog.ime.chrome.vk.View.CSS_ = {
  ALTGR: goog.getCssName('vk-sf-c273'),
  BOX: goog.getCssName('vk-box'),
  BUTTON_BACKGROUND: goog.getCssName('vk-sf-b'),
  KEY_BUTTON: goog.getCssName('vk-btn'),
  KEY_BUTTON_ACTIVE: goog.getCssName('vk-sf-a'),
  KEY_BUTTON_HOVER: goog.getCssName('vk-sf-h'),
  KEY_BUTTON_SELECTED: goog.getCssName('vk-sf-s'),
  KEY_CAPTION: goog.getCssName('vk-cap'),
  KEY_CAPTION_IMAGE: goog.getCssName('vk-cap-i')
};


/**
 * Initializes the View. This will render the UI elements on page and register
 * the event handlers.
 */
goog.ime.chrome.vk.View.prototype.init = function() {
  // Listens the messages from background.
  this.port_ = chrome.runtime.connect();
  this.port_.onMessage.addListener(goog.bind(this.handleMessage_, this));

  // Disconnects the port when window is unloaded.
  this.eventHandler_.listen(goog.dom.getWindow(),
      goog.events.EventType.UNLOAD, function() {
        this.port_.disconnect();
        this.port_ = null;
        this.dispose();
      });
};


/**
 * Handles the messages from background page.
 *
 * @param {!Object} message The message from background.
 * @private
 */
goog.ime.chrome.vk.View.prototype.handleMessage_ = function(message) {
  var type = message['type'];
  var value = message[type];
  switch (type) {
    case 'layout':
      this.activateLayout(/** @type {Object} */ (value));
      break;
    case 'click':
      this.visualizeClickButton(/** @type {number} */ (value));
      break;
    case 'state':
      this.refreshState(/** @type {string} */ (value));
      break;
  }
};


/**
 * Sets class names to an element.
 *
 * @param {!Element} elem Element to set class names.
 * @param {!Array.<string>} classes Class names.
 * @private
 */
goog.ime.chrome.vk.View.prototype.setClasses_ = function(elem, classes) {
  if (elem) {
    for (var i = 0; i < classes.length; i++) {
      if (i == 0) {
        goog.dom.classes.set(elem, classes[0]);
      } else {
        goog.dom.classes.add(elem, classes[i]);
      }
    }
  }
};


/**
 * Activates a layout for view, this may cause creating window.
 *
 * @param {Object} view The vk layout view object.
 *     See ParsedLayout.view for its format.
 */
goog.ime.chrome.vk.View.prototype.activateLayout = function(view) {
  this.view_ = view;

  this.container_ = this.dom_.createElement('div');
  var doc = this.dom_.getDocument();
  doc.body.appendChild(this.container_);
  doc.title = view.title;

  goog.dom.classes.add(this.container_, 'notranslate');
  goog.dom.classes.add(this.container_, goog.ime.chrome.vk.View.CSS_.BOX);
  this.contentPanel_ = this.buildContentPane_();
  this.dom_.appendChild(this.container_, this.contentPanel_);

  // Resizes the key buttons. It needs to be done in enterDocument because it
  // needs to get the key button's unscaled width.
  this.resizeKeyButtons_();

  var eventsForButtonUI = [
    goog.events.EventType.MOUSEDOWN,
    goog.events.EventType.MOUSEOVER,
    goog.events.EventType.MOUSEOUT];

  // Listen MOUSEUP on document.
  var mouseUpFunc = goog.bind(this.onButtonMouseEvents_, this, null /* btn */);
  this.eventHandler_.listen(doc, goog.events.EventType.MOUSEUP, mouseUpFunc);

  // Setup handlers for button click.
  for (var code in this.buttons_) {
    var button = this.buttons_[Number(code)];
    code = parseInt(code, 10);
    // SHIFT/ALTGR has 2 buttons with the same key code.
    if (!goog.isArrayLike(button)) {
      button = [button];
    }

    for (var i = 0, btn; btn = button[i]; ++i) {
      // The handler for click & mouse events.
      this.eventHandler_.listen(
          btn, goog.events.EventType.CLICK,
          goog.bind(this.onButtonClick_, this, code));
      this.eventHandler_.listen(
          btn, eventsForButtonUI,
          goog.bind(this.onButtonMouseEvents_, this, btn));
    }
  }
};


/**
 * The button click event handler.
 *
 * @param {number} code The key code which indicates which button is clicked.
 * @private
 */
goog.ime.chrome.vk.View.prototype.onButtonClick_ = function(code) {
  if (this.port_) {
    this.port_.postMessage({'type': 'click', 'click': code});
  }
};


/**
 * The button mouse down/up event handler.
 *
 * @param {Element} btn The button element.
 * @param {goog.events.BrowserEvent} e The event.
 * @private
 */
goog.ime.chrome.vk.View.prototype.onButtonMouseEvents_ = function(btn, e) {
  var isActive = false, isSelected = false;
  if (btn) {
    isActive = goog.dom.classes.has(btn,
        goog.ime.chrome.vk.View.CSS_.KEY_BUTTON_ACTIVE);
    isSelected = goog.dom.classes.has(btn,
        goog.ime.chrome.vk.View.CSS_.KEY_BUTTON_SELECTED);
  }
  switch (e.type) {
    case goog.events.EventType.MOUSEOVER:
      this.hoverBtn_ = btn;
      if (!isActive && !isSelected) {
        this.setClasses_(btn,
            [goog.ime.chrome.vk.View.CSS_.KEY_BUTTON,
             goog.ime.chrome.vk.View.CSS_.KEY_BUTTON_HOVER]);
        goog.dom.classes.add(btn.firstChild,
            goog.ime.chrome.vk.View.CSS_.KEY_BUTTON_HOVER);
      }
      break;
    case goog.events.EventType.MOUSEOUT:
      this.hoverBtn_ = null;
      if (!isActive && !isSelected) {
        goog.dom.classes.set(btn, goog.ime.chrome.vk.View.CSS_.KEY_BUTTON);
        goog.dom.classes.remove(btn.firstChild,
            goog.ime.chrome.vk.View.CSS_.KEY_BUTTON_HOVER);
      }
      break;
    case goog.events.EventType.MOUSEDOWN:
      if (!isSelected) {
        this.setClasses_(btn,
            [goog.ime.chrome.vk.View.CSS_.KEY_BUTTON,
             goog.ime.chrome.vk.View.CSS_.KEY_BUTTON_ACTIVE]);
        this.activeBtn_ = btn;
      }
      break;
    case goog.events.EventType.MOUSEUP:
      if (this.activeBtn_) {
        goog.dom.classes.set(this.activeBtn_,
            goog.ime.chrome.vk.View.CSS_.KEY_BUTTON);
        goog.dom.classes.remove(
            this.activeBtn_.firstChild,
            goog.ime.chrome.vk.View.CSS_.KEY_BUTTON_HOVER);
        this.activeBtn_ = null;
      }
      if (this.hoverBtn_) {
        this.setClasses_(this.hoverBtn_,
            [goog.ime.chrome.vk.View.CSS_.KEY_BUTTON,
             goog.ime.chrome.vk.View.CSS_.KEY_BUTTON_HOVER]);
        goog.dom.classes.add(
            this.hoverBtn_.firstChild,
            goog.ime.chrome.vk.View.CSS_.KEY_BUTTON_HOVER);
      }
      break;
  }
};


/**
 * Refreshes the UI according to the given state.
 *
 * @param {string} state The vk new state.
 */
goog.ime.chrome.vk.View.prototype.refreshState = function(state) {
  if (!this.view_) return;

  // Only record the new state if the state is valid (key mapping is not empty).
  // Note: if new state's key mapping is empty, this.state_ will not be changed
  // so that:
  // 1) all the button's display chars remain the current state, except those
  // state buttons (e.g. SHIFT, ALTGR, etc., see below code using 'state'
  // instead of 'this.state_')
  // 2) get commit chars according to current/old state.
  // The purpose of above behaviors is for compatibility of the old virtual
  // keyboard: if the new state's key mapping is not defined, it uses the
  // previous state's key mappings.
  if (this.view_['mappings'][state]) {
    this.state_ = state;
  }

  var ss = {20: 'l', 16: 's', 273: 'c'};

  for (var code in this.buttons_) {
    var button = this.buttons_[Number(code)];
    code = parseInt(code, 10);
    var s = ss[code];
    if (s) {
      if (!goog.isArrayLike(button)) {
        button = [button];
      }
      for (var i = 0, btn; btn = button[i]; ++i) {
        goog.dom.classes.set(btn, goog.ime.chrome.vk.View.CSS_.KEY_BUTTON);
        if (state.indexOf(s) >= 0) {
          goog.dom.classes.add(btn,
              goog.ime.chrome.vk.View.CSS_.KEY_BUTTON_SELECTED);
        }
      }
    } else {
      var caption = this.buildCaption_(code);
      if (caption) {
        this.dom_.removeChildren(button);
        this.dom_.appendChild(button, caption);
      }
    }
  }

  // Makes sure the hovered button have correct style.
  if (this.hoverBtn_ && !goog.dom.classes.has(
      this.hoverBtn_, goog.ime.chrome.vk.View.CSS_.KEY_BUTTON_SELECTED)) {
    this.setClasses_(this.hoverBtn_,
        [goog.ime.chrome.vk.View.CSS_.KEY_BUTTON,
         goog.ime.chrome.vk.View.CSS_.KEY_BUTTON_HOVER]);
    goog.dom.classes.add(this.hoverBtn_.firstChild,
        goog.ime.chrome.vk.View.CSS_.KEY_BUTTON_HOVER);
  }
};


/**
 * Visualized clicks the button which maps to the given key code.
 *
 * @param {number} keyCode The key code.
 */
goog.ime.chrome.vk.View.prototype.visualizeClickButton = function(keyCode) {
  var button = this.buttons_[keyCode];
  if (button) {
    this.setClasses_(button,
        [goog.ime.chrome.vk.View.CSS_.KEY_BUTTON,
         goog.ime.chrome.vk.View.CSS_.KEY_BUTTON_ACTIVE]);
    window.setTimeout(function() {
      goog.dom.classes.set(button, goog.ime.chrome.vk.View.CSS_.KEY_BUTTON);
    }, goog.ime.chrome.vk.View.VISUALIZE_CLICK_BUTTON_LAG_);
  }
};


/**
 * Resizes the key buttons according to their scales.
 *
 * @private
 */
goog.ime.chrome.vk.View.prototype.resizeKeyButtons_ = function() {
  // buttonScales defines the width scale of each button, by default the scale
  // is 1.
  var buttonScales = this.view_['is102'] ? [
    {13: 2},                // BS: 2
    {0: 1.5, 13: 1.5},      // TAB: 1.5, ENTER: 1.5
    {0: 1.75, 13: 1.25},    // CAPS: 1.75, ENTER: 1.25
    {0: 1.25, 12: 2.75},    // SHIFT: 1.25, SHIFT: 2.75
    {0: 3, 1: 9, 2: 3}] : [ // ALTGR: 3, SPACE: 9, ALTGR: 3
    {13: 2},                // BS: 2
    {0: 1.5, 13: 1.5},      // TAB: 1.5, \: 1.5
    {0: 1.75, 12: 2.25},    // CAPS: 1.75, ENTER: 2.25
    {0: 2.25, 11: 2.75},    // SHIFT: 2.25, SHIFT: 2.75
    {0: 3, 1: 9, 2: 3}];    // ALTGR: 3, SPACE: 9, ALTGR: 3

  var width = 29, marginWidth = 4;
  var firstButton = this.contentPanel_.children[0].children[0];
  if (firstButton) { // The first button must be in scale 1.
    // Chrome has problem to use goog.style.getSize(). It returns 30px for
    // button with width=29px.
    // Firefox has problem to use goog.style.getComputedStyle(). It returns 25px
    // for button with width=29px.
    // IE has problem to use goog.style.getComputedStyle(). It returns empty
    // string.
    var widthStyle = goog.style.getComputedStyle(firstButton, 'width');
    width = widthStyle ?  Number(widthStyle.slice(0, -2)) :
        goog.style.getSize(firstButton).width;
    var box = goog.style.getMarginBox(firstButton);
    marginWidth = box.right + box.left;
    width += marginWidth;
  }

  for (var i = 0; i < buttonScales.length; i++) {
    for (var j in buttonScales[i]) {
      j = Number(j); // The key type for enumeration is 'string' by default.
      var button = this.contentPanel_.children[i].children[j];
      button.style.width = (width * buttonScales[i][j] - marginWidth) + 'px';
    }
  }
};


/**
 * Builds the content pane table which includes the key buttons.
 *
 * @return {Element} The table element of content pane.
 * @private
 */
goog.ime.chrome.vk.View.prototype.buildContentPane_ = function() {
  var is102 = this.view_['is102'];

  var buttonCounts = [14, 14,
    is102 ? 14 : 13,
    is102 ? 13 : 12,
    3];

  var codes = is102 ? goog.ime.chrome.vk.KeyCode.ALLCODES102 :
      goog.ime.chrome.vk.KeyCode.ALLCODES101;

  var codeIndex = 0;
  var table = this.dom_.createDom(goog.dom.TagName.DIV);
  table.dir = table.style.direction = 'ltr';
  for (var i = 0; i < 5; ++i) {
    var row = this.dom_.createDom(goog.dom.TagName.DIV);
    row.style.whiteSpace = 'nowrap';
    for (var j = 0; j < buttonCounts[i]; ++j) {
      var code = codes.charCodeAt(codeIndex++);
      var button = this.buildButton_(code);
      this.dom_.appendChild(row, button);
    }
    this.dom_.appendChild(table, row);
  }
  return table;
};


/**
 * Builds the key button.
 *
 * @param {number} code The key code for the key button.
 * @return {Element} The button element.
 * @private
 */
goog.ime.chrome.vk.View.prototype.buildButton_ = function(code) {
  var caption = this.buildCaption_(code);
  var button = this.dom_.createDom(goog.dom.TagName.BUTTON, {
    'id': 'K' + code,
    'type': 'button',
    'class': goog.ime.chrome.vk.View.CSS_.KEY_BUTTON,
    'style': 'visibility:' + (caption ? '' : 'hidden')
  });
  if (caption) {
    this.dom_.appendChild(button, caption);
  }

  this.buttons_[code] = this.buttons_[code] ?
      /** @type {?} */ ([this.buttons_[code], button]) : button;
  return button;
};


/**
 * @enum {string} Css used for caption in modifier states.
 * @private
 */
goog.ime.chrome.vk.View.MODIFIER_CSS_ = {
  8: goog.getCssName('vk-sf-c8'),
  16: goog.getCssName('vk-sf-c16'),
  20: goog.getCssName('vk-sf-c20')
};


/**
 * Builds the caption element for button per key code.
 *
 * @param {number} code The key code.
 * @return {Element} The caption element which should be a SPAN.
 *     If null, the button should be hidden.
 * @private
 */
goog.ime.chrome.vk.View.prototype.buildCaption_ = function(code) {
  var codes = goog.events.KeyCodes;
  if (code == codes.TAB || code == codes.ENTER) {
    return null;
  }

  var caption = this.dom_.createDom(goog.dom.TagName.SPAN);

  if (goog.ime.chrome.vk.View.MODIFIER_CSS_[code]) {
    this.setClasses_(caption,
        [goog.ime.chrome.vk.View.CSS_.KEY_CAPTION,
         goog.ime.chrome.vk.View.CSS_.BUTTON_BACKGROUND,
         goog.ime.chrome.vk.View.MODIFIER_CSS_[code]]);
    return caption;
  }
  if (code == 273) { // ALTGR
    this.setClasses_(caption,
        [goog.ime.chrome.vk.View.CSS_.KEY_CAPTION,
         goog.ime.chrome.vk.View.CSS_.ALTGR]);
    caption.innerHTML = 'Ctrl + Alt';
    return caption;
  }

  var child = null; // child could be an image or text node.
  var disp = this.view_['mappings'][this.state_][String.fromCharCode(code)];

  if (disp && disp[1]) {
    if (disp[0] == 'S') {
      child = this.dom_.createTextNode(disp[1]);
      goog.dom.classes.set(caption, goog.ime.chrome.vk.View.CSS_.KEY_CAPTION);
    } else if (disp[0] == 'P') {
      child = this.dom_.createDom(goog.dom.TagName.IMG, {
        'src': 'images/' + this.view_['id'] + '_' + disp[1] + '.png'
      });
      goog.dom.classes.set(child,
          goog.ime.chrome.vk.View.CSS_.KEY_CAPTION_IMAGE);
    }
  }

  if (child) {
    this.dom_.appendChild(caption, child);
  } else {
    this.dom_.appendChild(caption, goog.dom.createTextNode('.'));
    caption.style.visibility = 'hidden';
  }
  return caption;
};


/** @override */
goog.ime.chrome.vk.View.prototype.disposeInternal = function() {
  goog.dispose(this.eventHandler_);
  delete this.buttons_;
  goog.base(this, 'disposeInternal');
};


(function() {
  var view = new goog.ime.chrome.vk.View();
  view.init();
})();
