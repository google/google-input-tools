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
 * @fileoverview Defines the View who can build the vk UI for the given layout
 *     view.
 */

goog.provide('i18n.input.keyboard.View');


goog.require('goog.array');
goog.require('goog.dom');
goog.require('goog.dom.TagName');
goog.require('goog.dom.classes');
goog.require('goog.events.Event');
goog.require('goog.events.EventHandler');
goog.require('goog.events.EventType');
goog.require('goog.events.KeyCodes');
goog.require('goog.fx.Dragger');
goog.require('goog.math.Coordinate');
goog.require('goog.positioning');
goog.require('goog.positioning.CornerBit');
goog.require('goog.style');
goog.require('goog.ui.Component');
goog.require('goog.ui.Container');
goog.require('goog.userAgent');
goog.require('i18n.input.common.ConstrainedDragger');
goog.require('i18n.input.common.GlobalSettings');
goog.require('i18n.input.common.dom');
goog.require('i18n.input.keyboard.EventType');
goog.require('i18n.input.keyboard.KeyCode');



/**
 * The keyboard view which can build the vk UI.
 *
 * @param {Object} view The vk layout view object.
 *     See ParsedLayout.view for its format.
 * @param {goog.dom.DomHelper} domHelper The dom helper.
 * @param {string=} opt_keyboardHelpUrl The keyboard help page url.
 * @param {boolean=} opt_isIframeWrapper Whether UI is wrappered by iframe.
 * @constructor
 * @extends {goog.ui.Container}
 */
i18n.input.keyboard.View = function(view, domHelper, opt_keyboardHelpUrl,
    opt_isIframeWrapper) {
  /**
   * The keyboard help page url.
   *
   * @type {string}
   * @private
   */
  this.keyboardHelpUrl_ = opt_keyboardHelpUrl || '';

  /**
   * Whether view's UI is wrappered by iframe.
   * @type {boolean}
   * @private
   */
  this.isIframeWraper_ = !!opt_isIframeWrapper;
  if (this.isIframeWraper_) {
    /**
     * The iframe wrapper.
     *
     * @type {!Element}
     * @private
     */
    this.iframe_ = i18n.input.common.dom.createIframeWrapper(
        domHelper.getDocument());
    domHelper = goog.dom.getDomHelper(goog.dom.getFrameContentDocument(
        this.iframe_));
  }

  /**
   * The vk layout view object. See ParsedLayout.view for its format.
   *
   * @type {Object}
   * @private
   */
  this.view_ = view;

  /**
   * The active state, which is one of '','s','c','l','sc','cl','sl','scl'.
   *
   * @type {string}
   * @private
   */
  this.state_ = '';

  /**
   * The map of {keycode: button}. For keycode==0, the related button is the
   * close button..
   *
   * @type {Object.<number, Element>}
   * @private
   */
  this.buttons_ = {};

  /**
   * True if this request is from IE6.
   *
   * @type {boolean}
   * @private
   */
  this.isIE6_ = goog.userAgent.IE && !goog.userAgent.isVersionOrHigher(7);

  /**
   * True if this request is from tier2 opera.
   *
   * @type {boolean}
   * @private
   */
  this.isTier2Opera_ = goog.userAgent.OPERA &&
      !goog.userAgent.isVersionOrHigher(11);

  /**
   * True if this request is from tier2 firefox.
   *
   * @type {boolean}
   * @private
   */
  this.isTier2FF_ = goog.userAgent.GECKO &&
      !goog.userAgent.isVersionOrHigher(3);

  /**
   * The event handler.
   *
   * @type {!goog.events.EventHandler}
   * @private
   */
  this.eventHandler_ = new goog.events.EventHandler(this);

  goog.base(this, undefined, undefined, domHelper);
};
goog.inherits(i18n.input.keyboard.View, goog.ui.Container);


/**
 * The dragger for keyboard dragging.
 *
 * @type {i18n.input.common.ConstrainedDragger}
 * @private
 */
i18n.input.keyboard.View.prototype.dragger_ = null;


/**
 * Whether the keyboard has been dragged by user.
 *
 * @type {boolean}
 * @private
 */
i18n.input.keyboard.View.prototype.isDragged_ = false;


/**
 * The title bar div.
 *
 * @type {Element}
 * @private
 */
i18n.input.keyboard.View.prototype.titleBar_ = null;


/**
 * The title button group div.
 *
 * @type {Element}
 * @private
 */
i18n.input.keyboard.View.prototype.titleBtnGroup_ = null;


/**
 * The close button.
 *
 * @type {Element}
 * @private
 */
i18n.input.keyboard.View.prototype.closeBtn_ = null;


/**
 * The min button.
 *
 * @type {Element}
 * @private
 */
i18n.input.keyboard.View.prototype.minBtn_ = null;


/**
 * The help button.
 *
 * @type {Element}
 * @private
 */
i18n.input.keyboard.View.prototype.helpBtn_ = null;


/**
 * The content panel element which contains all the key buttons.
 *
 * @type {Element}
 * @private
 */
i18n.input.keyboard.View.prototype.contentPanel_ = null;


/**
 * The active button, which is for key button UI :active effect.
 *
 * @type {Element}
 * @private
 */
i18n.input.keyboard.View.prototype.activeBtn_ = null;


/**
 * The hovered button, which is for key button UI :hover effect.
 *
 * @type {Element}
 * @private
 */
i18n.input.keyboard.View.prototype.hoverBtn_ = null;


/**
 * The width of the title bar. It's set in resizeKeyButtons_() under IE6/7.
 *
 * @type {number}
 * @private
 */
i18n.input.keyboard.View.prototype.titleBarWidth_ = 0;


/**
 * @define {string} The base url for the layout butotn images.
 */
i18n.input.keyboard.View.BTN_IMG_DIR =
    '//ssl.gstatic.com/inputtools/images/vk/layouts/';


/**
 * The width of each key button in scale 1.
 *
 * @type {number}
 * @private
 */
i18n.input.keyboard.View.prototype.keyButtonWidth_ = 33;


/**
 * The millisecs for visualize click button lag.
 *
 * @type {number}
 * @private
 */
i18n.input.keyboard.View.VISUALIZE_CLICK_BUTTON_LAG_ = 250;


/**
 * CSS used for virtual keyboard.
 * @enum {string}
 * @private
 */
i18n.input.keyboard.View.CSS_ = {
  ALTGR: goog.getCssName('vk-sf-c273'),
  BOX: goog.getCssName('vk-box'),
  BUTTON_BACKGROUND: goog.getCssName('vk-sf-b'),
  CLOSE_BUTTON: goog.getCssName('vk-sf-cl'),
  FF2: goog.getCssName('vk-sf-ff2'),
  HELP_BUTTON: goog.getCssName('vk-sf-hp'),
  IE: goog.getCssName('vk-sf-ie'),
  KEY_BUTTON: goog.getCssName('vk-btn'),
  KEY_BUTTON_ACTIVE: goog.getCssName('vk-sf-a'),
  KEY_BUTTON_HOVER: goog.getCssName('vk-sf-h'),
  KEY_BUTTON_SELECTED: goog.getCssName('vk-sf-s'),
  KEY_CAPTION: goog.getCssName('vk-cap'),
  KEY_CAPTION_IMAGE: goog.getCssName('vk-cap-i'),
  MAX_BUTTON: goog.getCssName('vk-sf-max'),
  MIN_BUTTON: goog.getCssName('vk-sf-min'),
  TITLE_BAR: goog.getCssName('vk-t'),
  TITLE_BUTTON: goog.getCssName('vk-t-btn'),
  TITLE_BUTTON_GROUP: goog.getCssName('vk-t-btns'),
  TITLE_BUTTON_BOX: goog.getCssName('vk-t-btn-o'),
  TITLE_BUTTON_HOVER: goog.getCssName('vk-sf-th'),
  MINIMIZE: goog.getCssName('vk-min')
};


/** @override */
i18n.input.keyboard.View.prototype.render = function(opt_parentElement) {
  goog.base(this, 'render', opt_parentElement);
  if (this.isIframeWraper_) {
    goog.style.setElementShown(this.iframe_, true);
    i18n.input.common.dom.copyNecessaryStyle(this.getElement(), this.iframe_);
  }
  this.reposition();
};


/** @override */
i18n.input.keyboard.View.prototype.createDom = function() {
  goog.base(this, 'createDom');

  var container = this.getElement();
  container.id = 'kbd';
  goog.dom.classes.add(container, 'notranslate');
  goog.dom.classes.add(container, i18n.input.keyboard.View.CSS_.BOX);
  if (goog.userAgent.IE) {
    goog.dom.classes.add(container, i18n.input.keyboard.View.CSS_.IE);
  }
  if (this.isTier2FF_) {
    goog.dom.classes.add(container, i18n.input.keyboard.View.CSS_.FF2);
  }

  var titleBar = this.buildTitleBar_();
  this.contentPanel_ = this.buildContentPane_();
  this.getDomHelper().appendChild(container, titleBar);
  this.getDomHelper().appendChild(container, this.contentPanel_);

  if (this.isIE6_) {
    // IE6 doesn't support 'position:fixed', so change to 'position:absolute'.
    container.style.position = 'absolute';
    // IE6 considers DIV takes the whole width of screen when no width
    // specified. While modern browsers consider DIV wrap its content if no
    // width specified. So if IE6, minimize the width and the DIV will be
    // expanded by its content.
    container.style.width = '1px';
  }
  if (this.isTier2Opera_) {
    // Height in Opera can't be 'auto', otherwise it will fill the whole page.
    container.style.height = '201px';
    container.style.bottom = '10px';
    // Opera doesn't support very large z-index, so change to '20001'.
    container.style.zIndex = '20001';
  }
};


/** @override */
i18n.input.keyboard.View.prototype.enterDocument = function() {
  goog.base(this, 'enterDocument');

  // Resizes the key buttons. It needs to be done in enterDocument because it
  // needs to get the key button's unscaled width.
  this.resizeKeyButtons_();

  this.setFocusableChildrenAllowed(false);
  this.setFocusable(false);

  // Setup dragging.
  this.dragger_ = new i18n.input.common.ConstrainedDragger(
      /** @type {!Element} */ (this.isIframeWraper_ ?
      this.iframe_ : this.getElement()), this.titleBar_);
  this.eventHandler_.listen(
      this.dragger_, goog.fx.Dragger.EventType.START, function() {
        this.isDragged_ = true;
        this.dragger_.repositionTarget();
      });

  var eventsForButtonUI = [
    goog.events.EventType.MOUSEDOWN,
    goog.events.EventType.MOUSEOVER,
    goog.events.EventType.MOUSEOUT];

  // Listen MOUSEUP on document for b/6029296.
  var doc = this.getDomHelper().getDocument();
  if (this.isIframeWraper_) {
    doc = goog.dom.getOwnerDocument(this.iframe_);
  }
  var mouseUpFunc = goog.bind(this.onButtonMouseEvents_, this, null /* btn */);
  this.eventHandler_.listen(doc, goog.events.EventType.MOUSEUP, mouseUpFunc);
  goog.array.forEach(i18n.input.common.dom.getSameDomainDocuments(doc),
      function(iframeDoc) {
        this.eventHandler_.listen(iframeDoc, goog.events.EventType.MOUSEUP,
            mouseUpFunc);
      }, this);

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
      // In IE, only adds UI event for version >= 7.
      if (!this.isIE6_) {
        this.eventHandler_.listen(
            btn, eventsForButtonUI,
            goog.bind(this.onButtonMouseEvents_, this, btn));
      }
    }
  }

  var titleButtons = [this.closeBtn_, this.minBtn_];
  // Handler for title button click.
  for (var i in titleButtons) {
    if (titleButtons[i]) {
      this.eventHandler_.listen(
          titleButtons[i], goog.events.EventType.CLICK,
          this.onTitleButtonClick_);
    }
  }
  // Handlers for title buttons' hover effects.
  titleButtons.push(this.helpBtn_);
  for (var i in titleButtons) {
    if (titleButtons[i]) {
      this.eventHandler_.listen(
          titleButtons[i], eventsForButtonUI, this.onTitleButtonHoverEvents_);
    }
  }

  // On twitter, mouse event will hidden empty input box. see b/8006384.
  // So we stop virtual keyboard itself mouse event's propagation.
  this.eventHandler_.listen(this.getElement(), goog.events.EventType.MOUSEDOWN,
      goog.events.Event.stopPropagation);
};


/** @override */
i18n.input.keyboard.View.prototype.exitDocument = function() {
  goog.base(this, 'exitDocument');

  this.eventHandler_.removeAll();
};


/**
 * The button click event handler.
 *
 * @param {number} code The key code which indicates which button is clicked.
 * @private
 */
i18n.input.keyboard.View.prototype.onButtonClick_ = function(code) {
  this.dispatchEvent(new goog.events.Event(
      goog.ui.Component.EventType.ACTION, {keyCode: code}));
};


/**
 * The button mouse down/up event handler.
 *
 * @param {Element} btn The button element.
 * @param {goog.events.BrowserEvent} e The event.
 * @private
 */
i18n.input.keyboard.View.prototype.onButtonMouseEvents_ = function(btn, e) {
  var isActive = false, isSelected = false;
  if (btn) {
    isActive = goog.dom.classes.has(btn,
        i18n.input.keyboard.View.CSS_.KEY_BUTTON_ACTIVE);
    isSelected = goog.dom.classes.has(btn,
        i18n.input.keyboard.View.CSS_.KEY_BUTTON_SELECTED);
  }
  switch (e.type) {
    case goog.events.EventType.MOUSEOVER:
      this.hoverBtn_ = btn;
      if (!isActive && !isSelected) {
        i18n.input.common.dom.setClasses(btn,
            [i18n.input.keyboard.View.CSS_.KEY_BUTTON,
             i18n.input.keyboard.View.CSS_.KEY_BUTTON_HOVER]);
        goog.dom.classes.add(btn.firstChild,
            i18n.input.keyboard.View.CSS_.KEY_BUTTON_HOVER);
      }
      break;
    case goog.events.EventType.MOUSEOUT:
      this.hoverBtn_ = null;
      if (!isActive && !isSelected) {
        goog.dom.classes.set(btn, i18n.input.keyboard.View.CSS_.KEY_BUTTON);
        goog.dom.classes.remove(btn.firstChild,
            i18n.input.keyboard.View.CSS_.KEY_BUTTON_HOVER);
      }
      break;
    case goog.events.EventType.MOUSEDOWN:
      if (!isSelected) {
        i18n.input.common.dom.setClasses(btn,
            [i18n.input.keyboard.View.CSS_.KEY_BUTTON,
             i18n.input.keyboard.View.CSS_.KEY_BUTTON_ACTIVE]);
        this.activeBtn_ = btn;
      }
      break;
    case goog.events.EventType.MOUSEUP:
      if (this.activeBtn_) {
        goog.dom.classes.set(this.activeBtn_,
            i18n.input.keyboard.View.CSS_.KEY_BUTTON);
        goog.dom.classes.remove(
            this.activeBtn_.firstChild,
            i18n.input.keyboard.View.CSS_.KEY_BUTTON_HOVER);
        this.activeBtn_ = null;
      }
      if (this.hoverBtn_) {
        i18n.input.common.dom.setClasses(this.hoverBtn_,
            [i18n.input.keyboard.View.CSS_.KEY_BUTTON,
             i18n.input.keyboard.View.CSS_.KEY_BUTTON_HOVER]);
        goog.dom.classes.add(
            this.hoverBtn_.firstChild,
            i18n.input.keyboard.View.CSS_.KEY_BUTTON_HOVER);
      }
      break;
  }
};


/**
 * The handler for title button's hover effects.
 *
 * @param {!goog.events.BrowserEvent} e The mouse event.
 * @private
 */
i18n.input.keyboard.View.prototype.onTitleButtonHoverEvents_ = function(e) {
  if (e.type != goog.events.EventType.MOUSEOVER &&
      e.type != goog.events.EventType.MOUSEOUT) {
    return;
  }

  var targetElement = null;
  var titleButtonTargets = [this.closeBtn_, this.minBtn_, this.helpBtn_];
  if (goog.array.contains(titleButtonTargets, e.target)) {
    targetElement = e.target.firstChild;
  } else if (goog.array.contains(titleButtonTargets, e.target.parentElement)) {
    targetElement = e.target;
  }

  if (targetElement) {
    switch (e.type) {
      case goog.events.EventType.MOUSEOVER:
        goog.dom.classes.add(targetElement,
            i18n.input.keyboard.View.CSS_.TITLE_BUTTON_HOVER);
        break;
      case goog.events.EventType.MOUSEOUT:
        goog.dom.classes.remove(targetElement,
            i18n.input.keyboard.View.CSS_.TITLE_BUTTON_HOVER);
        break;
    }
  }
};


/**
 * The handler for title button's click.
 *
 * @param {!goog.events.BrowserEvent} e The mouse event.
 * @private
 */
i18n.input.keyboard.View.prototype.onTitleButtonClick_ = function(e) {
  switch (e.target) {
    case this.closeBtn_:
    case this.closeBtn_.firstChild:
      this.dispatchEvent(i18n.input.keyboard.EventType.KEYBOARD_CLOSED);
      break;
    // Nullable buttons must be at the last switch-case statements.
    case this.minBtn_:
    case this.minBtn_.firstChild:
      this.dispatchEvent(i18n.input.keyboard.EventType.KEYBOARD_MINIMIZED);
      break;
  }
};


/**
 * Gets the commit chars for the given key.
 *
 * @param {number} keyCode The key code for the key button.
 * @return {?string} The commit chars. If null, meaning no key mapping
 *     avaliable.
 */
i18n.input.keyboard.View.prototype.getCommitChars = function(keyCode) {
  var keyChar = String.fromCharCode(keyCode);
  var item = this.view_.mappings[this.state_][keyChar];
  if (item) {
    var chars = item[2];
    if (chars) {
      return chars;
    }
  }
  if (keyCode == goog.events.KeyCodes.SPACE) {
    return '\u0020';
  }
  // Ignore the space bar for layout backward compatibility.
  var codes = this.view_.is102 ? i18n.input.keyboard.KeyCode.CODES102 :
      i18n.input.keyboard.KeyCode.CODES101;
  return codes.indexOf(keyChar) >= 0 ? '' : null;
};


/**
 * Sets the position of the keyboard.
 *
 * @param {goog.math.Coordinate=} opt_pos The coordinate of the position of the
 *     client viewport. If null, reposition to the default position.
 */
i18n.input.keyboard.View.prototype.reposition = function(opt_pos) {
  // The viewport size can't be correctly got in Opera(version < 11),
  // so let's disable reposition for Opera.
  if (!this.getElement() || this.isTier2Opera_) {
    return;
  }
  var doc = this.getDomHelper().getDocument();
  if (this.isIframeWraper_) {
    doc = goog.dom.getOwnerDocument(this.iframe_);
  }

  if (!opt_pos) {
    var anchorRect = this.dragger_.getBoundary();
    var corner = goog.positioning.getEffectiveCorner(doc.body,
        i18n.input.common.GlobalSettings.KeyboardDefaultLocation);

    opt_pos = new goog.math.Coordinate(
        corner & goog.positioning.CornerBit.RIGHT ?
        anchorRect.right : anchorRect.left,
        corner & goog.positioning.CornerBit.BOTTOM ?
        anchorRect.bottom : anchorRect.top);
  }
  this.dragger_.repositionTarget(opt_pos);
};


/**
 * Aligns the keyboard to the anchor element.
 *
 * @param {!goog.positioning.AnchoredPosition} anchoredPosition The anchored
 *     position.
 * @param {!goog.positioning.Corner} corner The corner of the keyboard.
 * @param {goog.math.Box=} opt_margin The margin of the alignment.
 */
i18n.input.keyboard.View.prototype.repositionToAnchor = function(
    anchoredPosition, corner, opt_margin) {
  if (this.getElement()) {
    anchoredPosition.reposition(this.getElement(), corner, opt_margin);
    if (this.isVisible()) {
      this.dragger_.repositionTarget();
    }
  }
};


/**
 * Gets the current vk's position.
 *
 * @return {goog.math.Coordinate} The current position of the vk.
 */
i18n.input.keyboard.View.prototype.getPosition = function() {
  return goog.style.getPosition(
      this.isIframeWraper_ ?
      this.iframe_ : this.getElement());
};


/**
 * Refreshes the UI according to the given state.
 *
 * @param {string} state The vk new state.
 */
i18n.input.keyboard.View.prototype.refreshState = function(state) {
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
  if (this.view_.mappings[state]) {
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
        goog.dom.classes.set(btn, i18n.input.keyboard.View.CSS_.KEY_BUTTON);
        if (state.indexOf(s) >= 0) {
          goog.dom.classes.add(btn,
              i18n.input.keyboard.View.CSS_.KEY_BUTTON_SELECTED);
        }
      }
    } else {
      var caption = this.buildCaption_(code);
      if (caption) {
        this.getDomHelper().removeChildren(button);
        this.getDomHelper().appendChild(button, caption);
      }
    }
  }

  // Makes sure the hovered button have correct style.
  if (this.hoverBtn_ && !goog.dom.classes.has(
      this.hoverBtn_, i18n.input.keyboard.View.CSS_.KEY_BUTTON_SELECTED)) {
    i18n.input.common.dom.setClasses(this.hoverBtn_,
        [i18n.input.keyboard.View.CSS_.KEY_BUTTON,
         i18n.input.keyboard.View.CSS_.KEY_BUTTON_HOVER]);
    goog.dom.classes.add(this.hoverBtn_.firstChild,
        i18n.input.keyboard.View.CSS_.KEY_BUTTON_HOVER);
  }
};


/** @override */
i18n.input.keyboard.View.prototype.setVisible = function(visible, opt_force) {
  var ret = goog.base(this, 'setVisible', visible, true);
  if (this.isIframeWraper_) {
    goog.style.setElementShown(this.iframe_, visible);
  }
  if (visible && this.dragger_) {
    this.dragger_.repositionTarget();
  }
  return ret;
};


/**
 * Gets whether the keyboard has been dragged by user.
 *
 * @return {boolean} True for dragged, false otherwise.
 */
i18n.input.keyboard.View.prototype.isDragged = function() {
  return this.isDragged_;
};


/**
 * Sets whether the keyboard view is dragged.
 *
 * @param {boolean} isDragged True if the keyboard is dragged, false otherwise.
 */
i18n.input.keyboard.View.prototype.setDragged = function(isDragged) {
  this.isDragged_ = isDragged;
};


/**
 * Visualized clicks the button which maps to the given key code.
 *
 * @param {number} keyCode The key code.
 */
i18n.input.keyboard.View.prototype.visualizeClickButton = function(keyCode) {
  if (!this.isVisible()) {
    return;
  }
  var button = this.buttons_[keyCode];
  if (button) {
    i18n.input.common.dom.setClasses(button,
        [i18n.input.keyboard.View.CSS_.KEY_BUTTON,
         i18n.input.keyboard.View.CSS_.KEY_BUTTON_ACTIVE]);
    window.setTimeout(function() {
      goog.dom.classes.set(button, i18n.input.keyboard.View.CSS_.KEY_BUTTON);
    }, i18n.input.keyboard.View.VISUALIZE_CLICK_BUTTON_LAG_);
  }
};


/**
 * Judges the direction of the active layout is RTL or LTR.
 *
 * @return {boolean} True if the layout is RTL; False otherwise.
 */
i18n.input.keyboard.View.prototype.isRTL = function() {
  return this.view_.isRTL;
};


/**
 * Resizes the key buttons according to their scales.
 *
 * @private
 */
i18n.input.keyboard.View.prototype.resizeKeyButtons_ = function() {
  // buttonScales defines the width scale of each button, by default the scale
  // is 1.
  var buttonScales = this.view_.is102 ? [
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
    width = goog.userAgent.WEBKIT && widthStyle ?
        Number(widthStyle.slice(0, -2)) : goog.style.getSize(firstButton).width;
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

  if (goog.userAgent.IE && !goog.userAgent.isVersionOrHigher(8)) {
    // IE7/6 will wrap the titlebar DIV to its content text. So set width
    // explicitly.
    this.titleBarWidth_ = width * 15 - marginWidth - 2;
    this.titleBar_.style.width = this.titleBarWidth_ + 'px';
  }
};


/**
 * Normalizes the inline-block style effect. In IE6/7, inline-block style
 * cannot be understood correctly.
 *
 * @param {!Element} elem The element to apply the inline-block styles.
 * @private
 */
i18n.input.keyboard.View.prototype.normalizeInlineBlock_ = function(elem) {
  if (goog.userAgent.IE && !goog.userAgent.isVersionOrHigher(8)) {
    elem.style.display = 'inline';
    elem.style.zoom = 1;
  } else {
    elem.style.display = 'inline-block';
  }
};


/**
 * Builds the title bar.
 *
 * @return {Element} The title bar div element.
 * @private
 */
i18n.input.keyboard.View.prototype.buildTitleBar_ = function() {
  var isRTL = this.isRTL();

  var ret = this.getDomHelper().createDom(
      goog.dom.TagName.DIV, {
        'dir': isRTL ? 'rtl' : 'ltr',
        'style': 'white-space: nowrap;'
      });

  // Title bar.
  this.titleBar_ = this.getDomHelper().createDom(
      goog.dom.TagName.DIV, {
        'class': i18n.input.keyboard.View.CSS_.TITLE_BAR
      }, this.view_.title);
  // Sometimes the page defines the style as: td {text-align:left}.
  // So forcely set the text-align style for RTL/LTR.
  this.titleBar_.style.textAlign = isRTL ? 'right' : 'left';
  this.normalizeInlineBlock_(this.titleBar_);
  this.getDomHelper().appendChild(ret, this.titleBar_);

  this.titleBtnGroup_ = this.getDomHelper().createDom(
      goog.dom.TagName.DIV, {
        'class': i18n.input.keyboard.View.CSS_.TITLE_BUTTON_GROUP
      });
  this.getDomHelper().appendChild(ret, this.titleBtnGroup_);

  var self = this;
  var createTitleBtn = function(sfCss, opt_isHelp) {
    var titleBtn = self.getDomHelper().createDom(
        opt_isHelp ? goog.dom.TagName.A : goog.dom.TagName.DIV,
        opt_isHelp ? {
          'target': '_blank',
          'href': this.keyboardHelpUrl_,
          'class': i18n.input.keyboard.View.CSS_.TITLE_BUTTON_BOX
        } : {
          'class': i18n.input.keyboard.View.CSS_.TITLE_BUTTON_BOX
        });
    goog.dom.classes.add(titleBtn, sfCss);

    var titleBtnInner = self.getDomHelper().createDom(
        goog.dom.TagName.DIV, {
          'class': i18n.input.keyboard.View.CSS_.TITLE_BUTTON
        });
    goog.dom.classes.add(titleBtnInner, sfCss);
    self.getDomHelper().appendChild(titleBtn, titleBtnInner);

    self.getDomHelper().appendChild(self.titleBtnGroup_, titleBtn);

    self.normalizeInlineBlock_(titleBtn);
    return titleBtn;
  };

  var right = isRTL ? 'left' : 'right';
  var paddingRight = isRTL ? 'paddingLeft' : 'paddingRight';
  // Creates Close/Min/Max/Help buttons.
  this.titleBtnGroup_.style[right] = 0;
  this.normalizeInlineBlock_(this.titleBtnGroup_);
  if (i18n.input.common.GlobalSettings.KeyboardHelpUrl) {
    this.helpBtn_ = createTitleBtn(
        i18n.input.keyboard.View.CSS_.HELP_BUTTON, true);
  }
  if (i18n.input.common.GlobalSettings.KeyboardShowMinMax) {
    this.minBtn_ = createTitleBtn(i18n.input.keyboard.View.CSS_.MIN_BUTTON);
  }
  this.closeBtn_ = createTitleBtn(i18n.input.keyboard.View.CSS_.CLOSE_BUTTON);
  this.closeBtn_.style[paddingRight] = '14px';

  return ret;
};


/**
 * Builds the content pane table which includes the key buttons.
 *
 * @return {Element} The table element of content pane.
 * @private
 */
i18n.input.keyboard.View.prototype.buildContentPane_ = function() {
  var is102 = this.view_.is102;
  var buttonCounts = [14, 14,
    is102 ? 14 : 13,
    is102 ? 13 : 12,
    3];

  var codes = is102 ? i18n.input.keyboard.KeyCode.ALLCODES102 :
      i18n.input.keyboard.KeyCode.ALLCODES101;

  var codeIndex = 0;
  var table = this.getDomHelper().createDom(goog.dom.TagName.DIV);
  table.dir = table.style.direction = 'ltr';
  for (var i = 0; i < 5; ++i) {
    var row = this.getDomHelper().createDom(goog.dom.TagName.DIV);
    row.style.whiteSpace = 'nowrap';
    for (var j = 0; j < buttonCounts[i]; ++j) {
      var code = codes.charCodeAt(codeIndex++);
      var button = this.buildButton_(code);
      this.getDomHelper().appendChild(row, button);
    }
    this.getDomHelper().appendChild(table, row);
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
i18n.input.keyboard.View.prototype.buildButton_ = function(code) {
  var caption = this.buildCaption_(code);
  var button = this.getDomHelper().createDom(goog.dom.TagName.BUTTON, {
    'id': 'K' + code,
    'type': 'button',
    'class': i18n.input.keyboard.View.CSS_.KEY_BUTTON,
    'style': 'visibility:' + (caption ? '' : 'hidden')
  });
  if (caption) {
    this.getDomHelper().appendChild(button, caption);
  }

  this.buttons_[code] = this.buttons_[code] ?
      /** @type {?} */ ([this.buttons_[code], button]) : button;
  return button;
};


/**
 * @enum {string} Css used for caption in modifier states.
 * @private
 * @const
 */
i18n.input.keyboard.View.MODIFIER_CSS_ = {
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
i18n.input.keyboard.View.prototype.buildCaption_ = function(code) {
  var codes = goog.events.KeyCodes;
  if (code == codes.TAB || code == codes.ENTER) {
    return null;
  }

  var caption = this.getDomHelper().createDom(goog.dom.TagName.SPAN);

  if (i18n.input.keyboard.View.MODIFIER_CSS_[code]) {
    i18n.input.common.dom.setClasses(caption,
        [i18n.input.keyboard.View.CSS_.KEY_CAPTION,
         i18n.input.keyboard.View.CSS_.BUTTON_BACKGROUND,
         i18n.input.keyboard.View.MODIFIER_CSS_[code]]);
    return caption;
  }
  if (code == 273) { // ALTGR
    i18n.input.common.dom.setClasses(caption,
        [i18n.input.keyboard.View.CSS_.KEY_CAPTION,
         i18n.input.keyboard.View.CSS_.ALTGR]);
    caption.innerHTML = 'Ctrl + Alt';
    return caption;
  }

  var child = null; // child could be an image or text node.
  var disp = this.view_.mappings[this.state_][String.fromCharCode(code)];

  if (disp && disp[1]) {
    if (disp[0] == 'S') {
      child = this.getDomHelper().createTextNode(disp[1]);
      goog.dom.classes.set(caption, i18n.input.keyboard.View.CSS_.KEY_CAPTION);
    } else if (disp[0] == 'P') {
      child = this.getDomHelper().createDom(goog.dom.TagName.IMG, {
        'src': i18n.input.keyboard.View.BTN_IMG_DIR +
            this.view_.id + '_' + disp[1] + '.png'
      });
      goog.dom.classes.set(child,
          i18n.input.keyboard.View.CSS_.KEY_CAPTION_IMAGE);
    }
  }

  if (child) {
    this.getDomHelper().appendChild(caption, child);
  } else {
    this.getDomHelper().appendChild(caption, goog.dom.createTextNode('.'));
    caption.style.visibility = 'hidden';
  }
  return caption;
};


/** @override */
i18n.input.keyboard.View.prototype.disposeInternal = function() {
  goog.dispose(this.eventHandler_);
  goog.dispose(this.dragger_);

  goog.base(this, 'disposeInternal');

  if (this.isIframeWraper_) {
    goog.dom.removeNode(this.iframe_);
  }
};
