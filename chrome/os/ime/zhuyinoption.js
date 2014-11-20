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
 * @fileoverview The script for zhuyin option pages.
 */
goog.provide('goog.ime.chrome.os.ZhuyinOption');

goog.require('goog.Disposable');
goog.require('goog.dom');
goog.require('goog.dom.TagName');
goog.require('goog.events');
goog.require('goog.events.EventHandler');
goog.require('goog.events.EventType');
goog.require('goog.ime.chrome.os.KeyboardLayouts');
goog.require('goog.ime.chrome.os.LocalStorageHandlerFactory');
goog.require('goog.ime.offline.InputToolCode');
goog.require('goog.object');



/**
 * Creates an option page for a given input tool code.
 *
 * @constructor
 * @extends {goog.Disposable}
 */
goog.ime.chrome.os.ZhuyinOption = function() {
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
      /** @type {goog.ime.chrome.os.ZhuyinLocalStorageHandler} */ (
      goog.ime.chrome.os.LocalStorageHandlerFactory.getInstance().
      getLocalStorageHandler(
          goog.ime.offline.InputToolCode.
          INPUTMETHOD_ZHUYIN_CHINESE_TRADITIONAL));

  this.init_();
  goog.events.listenOnce(window, goog.events.EventType.UNLOAD,
      goog.bind(this.dispose, this));
};
goog.inherits(goog.ime.chrome.os.ZhuyinOption, goog.Disposable);


/**
 * The dropdown box ids on the option page.
 *
 * @enum {string}
 * @private
 */
goog.ime.chrome.os.ZhuyinOption.ItemIDs_ = {
  LAYOUT: 'chos_layouts',
  SELECT_KEYS: 'chos_select_keys',
  PAGE_SIZE: 'chos_page_size'
};


/**
 * The descriptions used in the option page.
 *
 * @enum {string}
 * @private
 */
goog.ime.chrome.os.ZhuyinOption.ItemLabels_ = {
  TITLE: chrome.i18n.getMessage('zhuyin_setting_page'),
  LAYOUT: chrome.i18n.getMessage('zhuyin_keyboard_layout'),
  SELECT_KEYS: chrome.i18n.getMessage('zhuyin_select_keys'),
  PAGE_SIZE: chrome.i18n.getMessage('zhuyin_page_size')
};


/**
 * The CSS class names.
 *
 * @enum {string}
 * @private
 */
goog.ime.chrome.os.ZhuyinOption.ClassNames_ = {
  MAIN: 'chos-main',
  TITLE: 'chos-title',
  SELECTS: 'chos-select-div',
  SELECT_ITEM: 'chos-select-item',
  OPTION_ITEM: 'chos-option-item'
};


/**
 * Initialize the option page.
 *
 * @private
 */
goog.ime.chrome.os.ZhuyinOption.prototype.init_ = function() {
  var OPTION = goog.ime.chrome.os.ZhuyinOption;
  // The title.
  var title = goog.dom.createDom(goog.dom.TagName.H2, {
    'class': OPTION.ClassNames_.TITLE});
  title.appendChild(goog.dom.createTextNode(OPTION.ItemLabels_.TITLE));

  // The div contains the selection items.
  var selectionDiv = goog.dom.createDom(goog.dom.TagName.DIV, {
    'class': OPTION.ClassNames_.SELECTS});

  var layouts = goog.ime.chrome.os.KeyboardLayouts;
  var layoutItem = this.createItem_(
      OPTION.ItemIDs_.LAYOUT, OPTION.ItemLabels_.LAYOUT, [
        layouts.STANDARD,
        layouts.HSU,
        layouts.IBM,
        layouts.ETEN,
        layouts.ETEN26],
      this.localStorageHandler_.getLayout());

  var selectKeysItem = this.createItem_(
      OPTION.ItemIDs_.SELECT_KEYS,
      OPTION.ItemLabels_.SELECT_KEYS, [
        '1234567890',
        'asdfghjkl;',
        'asdfzxcv89',
        'asdfjkl789',
        '1234qweras'],
      this.localStorageHandler_.getSelectKeys());

  var pageSizeItem = this.createItem_(
      OPTION.ItemIDs_.PAGE_SIZE,
      OPTION.ItemLabels_.PAGE_SIZE,
      [8, 9, 10],
      this.localStorageHandler_.getPageSize());

  selectionDiv.appendChild(layoutItem);
  selectionDiv.appendChild(selectKeysItem);
  selectionDiv.appendChild(pageSizeItem);

  var mainDiv = goog.dom.createDom(goog.dom.TagName.DIV, {
    'class': OPTION.ClassNames_.MAIN});
  mainDiv.appendChild(title);
  mainDiv.appendChild(selectionDiv);

  document.body.appendChild(mainDiv);
};


/**
 * Creates a selection item.
 *
 * @param {string} id The item id.
 * @param {string} label The item label.
 * @param {Array} options The options.
 * @param {*} value The selected option.
 * @return {!Element} The div contains the checkbox and label.
 * @private
 */
goog.ime.chrome.os.ZhuyinOption.prototype.createItem_ = function(
    id, label, options, value) {
  var OPTION = goog.ime.chrome.os.ZhuyinOption;
  var selectionItem = goog.dom.createDom('select', {
    'id': id, 'class': OPTION.ClassNames_.OPTION_ITEM});
  for (var i = 0; i < options.length; ++i) {
    selectionItem[i] = new Option(options[i]);
  }
  selectionItem.value = value;

  var labelNode = goog.dom.createDom('label', {'for': id});
  labelNode.appendChild(goog.dom.createTextNode(label));

  var selectionDiv = goog.dom.createDom(goog.dom.TagName.DIV, {
    'class': OPTION.ClassNames_.SELECT_ITEM});
  selectionDiv.appendChild(labelNode);
  selectionDiv.appendChild(selectionItem);

  this.eventHandler_.listen(selectionItem, goog.events.EventType.CHANGE,
      goog.bind(this.saveSettings, this));
  return selectionDiv;
};


/**
 * Saves the setting page to localstorage, input method config and nacl
 * decoder.
 */
goog.ime.chrome.os.ZhuyinOption.prototype.saveSettings = function() {
  var ItemIDs_ = goog.ime.chrome.os.ZhuyinOption.ItemIDs_;

  var layout = goog.dom.getElement(ItemIDs_.LAYOUT).value;
  this.localStorageHandler_.setLayout(layout);

  var selectKeys = goog.dom.getElement(ItemIDs_.SELECT_KEYS).value;
  this.localStorageHandler_.setSelectKeys(selectKeys);

  var pageSize = goog.dom.getElement(ItemIDs_.PAGE_SIZE).value;
  this.localStorageHandler_.setPageSize(pageSize);

  chrome.extension.sendRequest(goog.object.create(
      'update',
      goog.ime.offline.InputToolCode.
      INPUTMETHOD_ZHUYIN_CHINESE_TRADITIONAL));
};


/** @override */
goog.ime.chrome.os.ZhuyinOption.prototype.disposeInternal = function() {
  goog.dispose(this.eventHandler_);
  goog.base(this, 'disposeInternal');
};


(function() {
  new goog.ime.chrome.os.ZhuyinOption();
})();
