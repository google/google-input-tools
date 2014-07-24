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
goog.provide('i18n.input.chrome.inputview.elements.content.CompactKey');

goog.require('goog.dom');
goog.require('goog.dom.TagName');
goog.require('goog.dom.classlist');
goog.require('i18n.input.chrome.inputview.Css');
goog.require('i18n.input.chrome.inputview.StateType');
goog.require('i18n.input.chrome.inputview.SwipeDirection');
goog.require('i18n.input.chrome.inputview.elements.ElementType');
goog.require('i18n.input.chrome.inputview.elements.content.FunctionalKey');



goog.scope(function() {



/**
 * The key to switch between different key set.
 *
 * @param {string} id The id.
 * @param {string} text The text.
 * @param {string} hintText The hint text.
 * @param {!i18n.input.chrome.inputview.StateManager} stateManager The state
 *     manager.
 * @param {number=} opt_marginLeftPercent The percent of the left margin.
 * @param {number=} opt_marginRightPercent The percent of the right margin.
 * @param {boolean=} opt_isGrey True if it is grey.
 * @param {!Array.<string>=} opt_moreKeys The more keys characters.
 * @param {goog.events.EventTarget=} opt_eventTarget The event target.
 * @constructor
 * @extends {i18n.input.chrome.inputview.elements.content.FunctionalKey}
 */
i18n.input.chrome.inputview.elements.content.CompactKey = function(id, text,
    hintText, stateManager, opt_marginLeftPercent, opt_marginRightPercent,
    opt_isGrey, opt_moreKeys, opt_eventTarget) {
  goog.base(this, id, i18n.input.chrome.inputview.elements.ElementType.
      COMPACT_KEY, text, '', opt_eventTarget);

  /**
   * The hint text.
   *
   * @type {string}
   */
  this.hintText = hintText;

  /**
   * The left margin.
   *
   * @type {number}
   * @private
   */
  this.marginLeftPercent_ = opt_marginLeftPercent || 0;

  /**
   * The right margin.
   *
   * @type {number}
   * @private
   */
  this.marginRightPercent_ = opt_marginRightPercent || 0;

  /**
   * The state manager.
   *
   * @type {!i18n.input.chrome.inputview.StateManager}
   * @private
   */
  this.stateManager_ = stateManager;

  /**
   * True if it is grey.
   *
   * @type {boolean}
   * @private
   */
  this.isGrey_ = !!opt_isGrey;

  /**
   * The more keys array.
   *
   * @type {!Array.<string>}
   */
  this.moreKeys = opt_moreKeys || [];

  this.pointerConfig.longPressWithPointerUp = true;
  this.pointerConfig.flickerDirection =
      i18n.input.chrome.inputview.SwipeDirection.UP;
  this.pointerConfig.longPressDelay = 500;
};
goog.inherits(i18n.input.chrome.inputview.elements.content.CompactKey,
    i18n.input.chrome.inputview.elements.content.FunctionalKey);
var CompactKey = i18n.input.chrome.inputview.elements.content.CompactKey;


/**
 * The flickerred character.
 *
 * @type {string}
 */
CompactKey.prototype.flickerredCharacter = '';


/** @override */
CompactKey.prototype.createDom = function() {
  goog.base(this, 'createDom');

  goog.dom.classlist.add(this.tableCell,
      i18n.input.chrome.inputview.Css.COMPACT_KEY);
  if (!this.isGrey_) {
    goog.dom.classlist.remove(this.bgElem,
        i18n.input.chrome.inputview.Css.SPECIAL_KEY_BG);
  }

  if (this.hintText) {
    var dom = this.getDomHelper();
    dom.removeChildren(this.tableCell);
    var inlineWrap = dom.createDom(goog.dom.TagName.DIV,
        i18n.input.chrome.inputview.Css.INLINE_DIV);
    dom.appendChild(this.tableCell, inlineWrap);
    this.hintTextElem = dom.createDom(goog.dom.TagName.DIV,
        i18n.input.chrome.inputview.Css.HINT_TEXT, this.hintText);
    dom.appendChild(inlineWrap, this.hintTextElem);
    dom.appendChild(inlineWrap, this.textElem);
  }
};


/** @override */
CompactKey.prototype.resize = function(width, height) {
  var marginLeft = Math.floor(width * this.marginLeftPercent_);
  this.getElement().style.marginLeft = marginLeft + 'px';

  var marginRight = Math.floor(width * this.marginRightPercent_);
  // TODO: Remove this ugly hack. The default margin right is 10px, we
  // need to add the default margin here to make all the keys have the same
  // look.
  marginRight += 10;
  this.getElement().style.marginRight = marginRight + 'px';

  goog.base(this, 'resize', width, height);
};


/**
 * Get the active character. It may be upper case |text| when shift is pressed
 * or flickerred character when swipe. Note this should replace Compactkey.text
 * for compact keys.
 */
CompactKey.prototype.getActiveCharacter = function() {
  if (this.flickerredCharacter) {
    return this.flickerredCharacter;
  } else {
    return this.stateManager_.hasState(i18n.input.chrome.inputview.StateType.
        SHIFT) ? this.text.toUpperCase() : this.text;
  }
};


/** @override */
CompactKey.prototype.update = function() {
  goog.base(this, 'update');

  goog.dom.setTextContent(this.textElem, this.getActiveCharacter());
};

});  // goog.scope

