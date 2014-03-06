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
 * @fileoverview The entry of standalone vk. Standalone class could be
 *     considered as the same role of ITA's KeyboardPlugin which is responsible
 *     for passing events to keyboard, controling its visibility, handling
 *     keyboard CLOSE event, providing ability to manipulate the text input,
 *     as well as managing the current/active text input.
 */



goog.provide('i18n.input.keyboard.Standalone');

goog.require('goog.array');
goog.require('goog.events');
goog.require('goog.events.EventType');
goog.require('goog.math.Box');
goog.require('goog.positioning.AnchoredPosition');
goog.require('i18n.input.common.Inputable');
goog.require('i18n.input.keyboard.Controller');
goog.require('i18n.input.keyboard.EventType');



/**
 * The entry of virtual keyboard. It implements the interface InputManager, and
 * pass the key events to Controller.
 *
 * @constructor
 * @implements {i18n.input.common.Inputable}
 */
i18n.input.keyboard.Standalone = function() {
  /**
   * The active input box.
   *
   * @type {HTMLElement}
   * @private
   */
  this.activeInput_ = null;

  // Hooks Controller with this as an input manager.
  i18n.input.keyboard.Controller.getInstance().setInputable(this);
};


/**
 * Gets the supported event types for the input field.
 *
 * @return {Array.<goog.events.EventType>} The supported event types.
 */
i18n.input.keyboard.Standalone.prototype.getEventTypes = function() {
  return [goog.events.EventType.KEYDOWN,
    goog.events.EventType.KEYPRESS,
    goog.events.EventType.KEYUP];
};


/**
 * Handles the ui events.
 *
 * @param {goog.events.BrowserEvent} e The ui event.
 * @return {boolean|undefined} False to continue handling, True otherwise.
 * */
i18n.input.keyboard.Standalone.prototype.handleEvent = function(e) {
  var ret = false;
  if (!goog.array.contains(this.getEventTypes(), e.type)) {
    return false;
  }

  var controller = i18n.input.keyboard.Controller.getInstance();
  if (controller.handleEvent(e)) {
    e.stopPropagation();
    e.preventDefault();
  }

  // IE7/8 doesn't continue handling keydown for return false;
  // Intead, it works for return undefined.
  // So safely make the return value as undefined.
  if (!ret) {
    ret = undefined;
  }
  return ret;
};


/**
 * Sets the current active input box. Other functions will perform on this
 * input box.
 *
 * @param {?HTMLElement} input The input box element.
 */
i18n.input.keyboard.Standalone.prototype.setActiveInput = function(input) {
  var tag = input.tagName.toUpperCase();
  var type = input.type.toUpperCase();
  if ((tag != 'INPUT' && tag != 'TEXTAREA') ||
      (tag == 'INPUT' && type != 'TEXT')) {
    return;
  }

  var eventtypes = this.getEventTypes();
  for (var i = 0, et; et = eventtypes[i]; ++i) {
    // Stop listening on the old active input.
    if (this.activeInput_) {
      goog.events.unlisten(
          this.activeInput_, et, this.handleEvent, false, this);
    }
    if (input) {
      // Listens key events of the new active input.
      goog.events.listen(input, et, this.handleEvent, false, this);
    }
  }
  this.activeInput_ = input;
};


/** @override */
i18n.input.keyboard.Standalone.prototype.getTextBeforeCursor = function(
    maxLength) {
  if (!this.activeInput_) return '';

  if (!!document.selection) { // IE
    var bookmark = document.selection.createRange().getBookmark();
    var selection = this.activeInput_.createTextRange();
    selection['moveToBookmark'](bookmark);
    var before = this.activeInput_.createTextRange();
    before.collapse(true);
    before.setEndPoint('EndToStart', selection);
    return before.text;
  }

  return this.activeInput_.value.slice(0, this.activeInput_.selectionStart);
};


/** @override */
i18n.input.keyboard.Standalone.prototype.commitText = function(text, opt_back) {
  if (!this.activeInput_) return;
  var back = opt_back ? opt_back : 0;

  if (!!document.selection) { // IE
    var range = document.selection.createRange();
    if (range.parentElement() == this.activeInput_) {
      if (!text && back == 1 && range.text) {
        // Select + BS only deletes the selection.
        back = 0;
      }
      range.moveStart('character', -back);
      range.text = text;
      range.collapse(false);
      range.select();
    }
  } else {
    var value = this.activeInput_.value;
    var from = this.activeInput_.selectionStart;
    var to = this.activeInput_.selectionEnd;
    if (from > to) {
      // Switches from and to.
      from = from + to;
      to = from - to;
      from = from - to;
    }
    if (!text && back == 1 && from < to) {
      // Select + BS only deletes the selection.
      back = 0;
    }
    from -= (from < back ? from : back);

    this.activeInput_.value = value.slice(0, from) + text + value.slice(to);
    from += text.length;
    this.activeInput_.setSelectionRange(from, from);
  }
};


/** @override */
i18n.input.keyboard.Standalone.prototype.setFocus = function() {
  if (!this.activeInput_) return;

  this.activeInput_.focus();
};


/** @override */
i18n.input.keyboard.Standalone.prototype.createToken = function() {
  return null;
};


/** @override */
i18n.input.keyboard.Standalone.prototype.getCursorPosition = function() {
  return null;
};


/** @override */
i18n.input.keyboard.Standalone.prototype.isRangeCollapsed = function() {
  return true;
};


(function() {
  goog.exportSymbol('i18n.input.keyboard.Keyboard',
      i18n.input.keyboard.Standalone);
  goog.exportSymbol('i18n.input.keyboard.EventType',
      i18n.input.keyboard.EventType);

  var prototype = i18n.input.keyboard.Standalone.prototype;
  goog.exportProperty(prototype, 'register', prototype.setActiveInput);

  var controller = i18n.input.keyboard.Controller;
  goog.exportProperty(prototype, 'loadLayout', function(layout) {
    controller.getInstance().loadLayout(layout);
  });
  goog.exportProperty(prototype, 'activateLayout', function(layout) {
    controller.getInstance().activateLayout(layout);
  });
  goog.exportProperty(prototype, 'setVisible', function(visible) {
    controller.getInstance().setVisible(visible);
  });
  goog.exportProperty(prototype, 'reposition', function(
      anchorElement, anchorCorner, selfCorner, margins) {
        var anchoredPosition = new goog.positioning.AnchoredPosition(
            anchorElement, anchorCorner);
        var marginBox;
        if (margins && margins.length == 4) {
          marginBox = new goog.math.Box(
              margins[0], margins[1], margins[2], margins[3]);
        }
        controller.getInstance().reposition(
            anchoredPosition, selfCorner, marginBox);
      });
  goog.exportProperty(prototype, 'addEventListener', function(
      type, listener, opt_scope) {
        controller.getInstance().listen(type, listener, opt_scope);
      });
})();
