// Copyright 2013 The ChromeOS IME Authors. All Rights Reserved.
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
 * @fileoverview The script for pinyin option pages.
 */

goog.provide('goog.ime.chrome.os.PinyinOption');

goog.require('goog.Disposable');
goog.require('goog.dom');
goog.require('goog.dom.TagName');
goog.require('goog.events');
goog.require('goog.events.EventHandler');
goog.require('goog.events.EventType');
goog.require('goog.ime.chrome.os.LocalStorageHandlerFactory');
goog.require('goog.ime.offline.InputToolCode');
goog.require('goog.object');



/**
 * Creates an option page for a given input tool code.
 *
 * @constructor
 * @extends {goog.Disposable}
 */
goog.ime.chrome.os.PinyinOption = function() {
  /**
   * The event handler.
   *
   * @type {!goog.events.EventHandler}
   * @private
   */
  this.eventHandler_ = new goog.events.EventHandler(this);

  /**
   * The local storage handler.
   *
   * @type {goog.ime.chrome.os.LocalStorageHandler}
   * @private
   */
  this.localStorageHandler_ =
      /** @type {goog.ime.chrome.os.PinyinLocalStorageHandler} */ (
      goog.ime.chrome.os.LocalStorageHandlerFactory.getInstance().
      getLocalStorageHandler(
          goog.ime.offline.InputToolCode.
          INPUTMETHOD_PINYIN_CHINESE_SIMPLIFIED));

  this.init_();
  goog.events.listenOnce(window, goog.events.EventType.UNLOAD,
      goog.bind(this.dispose, this));
};
goog.inherits(goog.ime.chrome.os.PinyinOption, goog.Disposable);


/**
 * The checkbox ids on the option page.
 *
 * @enum {string}
 * @private
 */
goog.ime.chrome.os.PinyinOption.ItemIDs_ = {
  FUZZY_PINYIN: 'chos_fuzzy_pinyin_selection',
  USER_DICT: 'chos_user_dict_selection',
  TOP_PAGE: 'chos_top_page_selection',
  BOTTOM_PAGE: 'chos_bottom_page_selection',
  INIT_LANG: 'chos_init_lang_selection',
  INIT_SBC: 'chos_init_sbc_selection',
  INIT_PUNC: 'chos_init_punc_selection',
  FUZZY_ITEM_PREFIX: 'chos_fuzzy_expansion_'
};


/**
 * The descriptions used in the option page.
 *
 * @enum {string}
 * @private
 */
goog.ime.chrome.os.PinyinOption.ItemLabels_ = {
  TITLE: chrome.i18n.getMessage('pinyin_setting_page'),
  FUZZY_PINYIN: chrome.i18n.getMessage('fuzzy_pinyin'),
  USER_DICT: chrome.i18n.getMessage('user_dict'),
  TOP_PAGE: chrome.i18n.getMessage('move_page_key_above'),
  BOTTOM_PAGE: chrome.i18n.getMessage('move_page_key_below'),
  INIT_LANG: chrome.i18n.getMessage('init_lang'),
  INIT_SBC: chrome.i18n.getMessage('init_sbc'),
  INIT_PUNC: chrome.i18n.getMessage('init_punc')
};


/**
 * The CSS class names.
 *
 * @enum {string}
 * @private
 */
goog.ime.chrome.os.PinyinOption.ClassNames_ = {
  MAIN: 'chos-main',
  TITLE: 'chos-title',
  SELECTS: 'chos-select-div',
  SELECT_ITEM: 'chos-select-item',
  FUZZY_DIV: 'chos-fuzzy-div',
  FUZZY_DIV_LEFT: 'chos-fuzzy-div-left',
  FUZZY_DIV_RIGHT: 'chos-fuzzy-div-right'
};


/**
 * Initialize the option page.
 *
 * @private
 */
goog.ime.chrome.os.PinyinOption.prototype.init_ = function() {
  var OPTION = goog.ime.chrome.os.PinyinOption;
  // The title.
  var title = goog.dom.createDom(goog.dom.TagName.H2, {
    'class': OPTION.ClassNames_.TITLE});
  title.appendChild(goog.dom.createTextNode(OPTION.ItemLabels_.TITLE));

  // The div contains the selection items.
  var selectionDiv = goog.dom.createDom(goog.dom.TagName.DIV, {
    'class': OPTION.ClassNames_.SELECTS});

  var fuzzyPinyinEnabled = this.localStorageHandler_.getFuzzyPinyinEnabled();
  var fuzzyPinyinItem = this.createSelectItem_(
      OPTION.ItemIDs_.FUZZY_PINYIN,
      OPTION.ItemLabels_.FUZZY_PINYIN,
      fuzzyPinyinEnabled);
  this.eventHandler_.listen(fuzzyPinyinItem, goog.events.EventType.CHANGE,
      goog.bind(this.updateFuzzyPinyin, this));

  var fuzzyExpansions = this.localStorageHandler_.getFuzzyExpansions();
  var fuzzyExpansionDiv = this.createFuzzyExpansionDiv_(
      fuzzyExpansions, fuzzyPinyinEnabled);

  var userDictEnabled = this.localStorageHandler_.getUserDictEnabled();
  var userDictItem = this.createSelectItem_(
      OPTION.ItemIDs_.USER_DICT,
      OPTION.ItemLabels_.USER_DICT,
      userDictEnabled);

  var topPageEnabled = this.localStorageHandler_.getTopPageEnabled();
  var topPageItem = this.createSelectItem_(
      OPTION.ItemIDs_.TOP_PAGE,
      OPTION.ItemLabels_.TOP_PAGE,
      topPageEnabled);

  var bottomPageEnabled = this.localStorageHandler_.getBottomPageEnabled();
  var bottomPageItem = this.createSelectItem_(
      OPTION.ItemIDs_.BOTTOM_PAGE,
      OPTION.ItemLabels_.BOTTOM_PAGE,
      bottomPageEnabled);

  var initLang = this.localStorageHandler_.getInitLang();
  var initLangItem = this.createSelectItem_(
      OPTION.ItemIDs_.INIT_LANG,
      OPTION.ItemLabels_.INIT_LANG,
      initLang);

  var initSBC = this.localStorageHandler_.getInitSBC();
  var initSBCItem = this.createSelectItem_(
      OPTION.ItemIDs_.INIT_SBC,
      OPTION.ItemLabels_.INIT_SBC,
      initSBC);

  var initPunc = this.localStorageHandler_.getInitPunc();
  var initPuncItem = this.createSelectItem_(
      OPTION.ItemIDs_.INIT_PUNC,
      OPTION.ItemLabels_.INIT_PUNC,
      initPunc);

  selectionDiv.appendChild(fuzzyPinyinItem);
  selectionDiv.appendChild(fuzzyExpansionDiv);
  selectionDiv.appendChild(userDictItem);
  selectionDiv.appendChild(topPageItem);
  selectionDiv.appendChild(bottomPageItem);
  selectionDiv.appendChild(initLangItem);
  selectionDiv.appendChild(initSBCItem);
  selectionDiv.appendChild(initPuncItem);

  var mainDiv = goog.dom.createDom(goog.dom.TagName.DIV, {
    'class': OPTION.ClassNames_.MAIN});
  mainDiv.appendChild(title);
  mainDiv.appendChild(selectionDiv);

  document.body.appendChild(mainDiv);
};


/**
 * Creates a select item.
 *
 * @param {string} id The item id.
 * @param {string} label The item label.
 * @param {boolean} value Whether the item is checked.
 * @param {boolean=} opt_disabled Whether the item is disabled.
 * @return {!Element} The div contains the checkbox and label.
 * @private
 */
goog.ime.chrome.os.PinyinOption.prototype.createSelectItem_ = function(
    id, label, value, opt_disabled) {
  var checkbox = goog.dom.createDom('input', {
    'type': 'checkbox',
    'id': id,
    'checked': value,
    'disabled': opt_disabled});
  var labelNode = goog.dom.createDom('label', {
    'for': id});
  labelNode.appendChild(goog.dom.createTextNode(label));

  var checkboxDiv = goog.dom.createDom(goog.dom.TagName.DIV, {
    'class': goog.ime.chrome.os.PinyinOption.ClassNames_.SELECT_ITEM});
  checkboxDiv.appendChild(checkbox);
  checkboxDiv.appendChild(labelNode);

  this.eventHandler_.listen(checkbox, goog.events.EventType.CHANGE,
      goog.bind(this.saveSettings, this));
  return checkboxDiv;
};


/**
 * Creates the fuzzy expansions div.
 *
 * @param {Object.<string, boolean>} fuzzyExpansions The fuzzy expansions.
 * @param {boolean} enabled Whether the fuzzy pinyin is enabled.
 * @return {!Element} The div contains all the checkbox for fuzzy expansions.
 * @private
 */
goog.ime.chrome.os.PinyinOption.prototype.createFuzzyExpansionDiv_ =
    function(fuzzyExpansions, enabled) {
  var OPTION = goog.ime.chrome.os.PinyinOption;
  var expansionDiv = goog.dom.createDom(goog.dom.TagName.DIV, {
    'class': OPTION.ClassNames_.FUZZY_DIV});
  var leftRightDivs = [
    goog.dom.createDom(goog.dom.TagName.DIV, {
      'class': OPTION.ClassNames_.FUZZY_DIV_LEFT}),
    goog.dom.createDom(goog.dom.TagName.DIV, {
      'class': OPTION.ClassNames_.FUZZY_DIV_RIGHT})];

  var index = 0;
  for (var fuzzyPair in fuzzyExpansions) {
    var selected = fuzzyExpansions[fuzzyPair];
    if (selected == undefined) {
      selected = enabled;
    }

    var fuzzyPairLabel = fuzzyPair.split('_').join(':');
    var selectItem = this.createSelectItem_(
        OPTION.ItemIDs_.FUZZY_ITEM_PREFIX + fuzzyPair,
        fuzzyPairLabel, selected && enabled, !enabled);

    leftRightDivs[index].appendChild(selectItem);
    index = (index + 1) % 2;
  }

  goog.dom.append(expansionDiv, leftRightDivs);
  return expansionDiv;
};


/**
 * Enables/Disable fuzzy pinyin.
 */
goog.ime.chrome.os.PinyinOption.prototype.updateFuzzyPinyin = function() {
  var enabled = goog.dom.getElement(
      goog.ime.chrome.os.PinyinOption.ItemIDs_.FUZZY_PINYIN).checked;
  var fuzzyExpansions = this.localStorageHandler_.getFuzzyExpansions();
  for (var fuzzyPair in fuzzyExpansions) {
    var selected = fuzzyExpansions[fuzzyPair];
    if (selected == undefined) {
      selected = enabled;
    }
    var selectItem = goog.dom.getElement(
        goog.ime.chrome.os.PinyinOption.ItemIDs_.FUZZY_ITEM_PREFIX +
        fuzzyPair);
    goog.dom.setProperties(selectItem, {
      'checked': selected && enabled,
      'disabled': !enabled});
  }
};


/**
 * Saves the setting page to localstorage, input method config and nacl
 * decoder.
 */
goog.ime.chrome.os.PinyinOption.prototype.saveSettings = function() {
  var ItemIDs_ = goog.ime.chrome.os.PinyinOption.ItemIDs_;

  var fuzzyPinyinEnabled = goog.dom.getElement(ItemIDs_.FUZZY_PINYIN).checked;
  this.localStorageHandler_.setFuzzyPinyinEnabled(fuzzyPinyinEnabled);

  var userDictEnabled = goog.dom.getElement(ItemIDs_.USER_DICT).checked;
  this.localStorageHandler_.setUserDictEnabled(userDictEnabled);

  var topPageEnabled = goog.dom.getElement(ItemIDs_.TOP_PAGE).checked;
  this.localStorageHandler_.setTopPageEnabled(topPageEnabled);

  var bottomPageEnabled = goog.dom.getElement(ItemIDs_.BOTTOM_PAGE).checked;
  this.localStorageHandler_.setBottomPageEnabled(bottomPageEnabled);

  var initLangEnabled = goog.dom.getElement(ItemIDs_.INIT_LANG).checked;
  this.localStorageHandler_.setInitLang(initLangEnabled);

  var initSBCEnabled = goog.dom.getElement(ItemIDs_.INIT_SBC).checked;
  this.localStorageHandler_.setInitSBC(initSBCEnabled);

  var initPuncEnabled = goog.dom.getElement(ItemIDs_.INIT_PUNC).checked;
  this.localStorageHandler_.setInitPunc(initPuncEnabled);

  if (fuzzyPinyinEnabled) {
    var fuzzyExpansions = this.localStorageHandler_.getFuzzyExpansions();
    for (var fuzzyPair in fuzzyExpansions) {
      var selected = goog.dom.getElement(
          ItemIDs_.FUZZY_ITEM_PREFIX + fuzzyPair).checked;
      fuzzyExpansions[fuzzyPair] = selected;
    }
    this.localStorageHandler_.setFuzzyExpansions(fuzzyExpansions);
  }

  chrome.extension.sendRequest(goog.object.create(
      'update',
      goog.ime.offline.InputToolCode.INPUTMETHOD_PINYIN_CHINESE_SIMPLIFIED));
};


/** @override */
goog.ime.chrome.os.PinyinOption.prototype.disposeInternal = function() {
  goog.dispose(this.eventHandler_);
  goog.base(this, 'disposeInternal');
};


(function() {
  new goog.ime.chrome.os.PinyinOption();
})();
