goog.provide('i18n.input.chrome.inputview.M17nModel');

goog.require('goog.events.EventHandler');
goog.require('goog.events.EventTarget');
goog.require('i18n.input.chrome.inputview.SpecNodeName');
goog.require('i18n.input.chrome.inputview.content.util');
goog.require('i18n.input.chrome.vk.KeyCode');
goog.require('i18n.input.chrome.vk.Model');



/**
 * The model to legacy cloud vk configuration.
 *
 * @constructor
 * @extends {goog.events.EventTarget}
 */
i18n.input.chrome.inputview.M17nModel = function() {
  goog.base(this);

  /**
   * The event handler.
   *
   * @type {!goog.events.EventHandler}
   * @private
   */
  this.handler_ = new goog.events.EventHandler(this);

  /**
   * The model for cloud vk.
   *
   * @type {!i18n.input.chrome.vk.Model}
   * @private
   */
  this.model_ = new i18n.input.chrome.vk.Model();
  this.handler_.listen(this.model_,
      i18n.input.chrome.vk.EventType.LAYOUT_LOADED,
      this.onLayoutLoaded_);
};
goog.inherits(i18n.input.chrome.inputview.M17nModel,
    goog.events.EventTarget);


/**
 * The active layout view.
 *
 * @type {!i18n.input.chrome.vk.ParsedLayout}
 * @private
 */
i18n.input.chrome.inputview.M17nModel.prototype.layoutView_;


/**
 * Loads the configuration.
 *
 * @param {string} lang The language code.
 */
i18n.input.chrome.inputview.M17nModel.prototype.loadConfig = function(lang) {
  this.model_.loadLayout(lang);
};


/**
 * Callback when legacy model is loaded.
 *
 * @param {!i18n.input.chrome.vk.LayoutEvent} e The event.
 * @private
 */
i18n.input.chrome.inputview.M17nModel.prototype.onLayoutLoaded_ = function(
    e) {
  var layoutView = /** @type {!i18n.input.chrome.vk.ParsedLayout} */
      (e.layoutView);
  this.layoutView_ = layoutView;
  var is102 = layoutView.view.is102;
  var codes = is102 ? i18n.input.chrome.vk.KeyCode.CODES102 :
      i18n.input.chrome.vk.KeyCode.CODES101;
  var keyCount = is102 ? 48 : 47;
  var keyCharacters = [];
  for (var i = 0; i < keyCount; i++) {
    var characters = this.findCharacters_(layoutView.view.mappings,
        codes[i]);
    keyCharacters.push(characters);
  }
  keyCharacters.push(['\u0020', '\u0020']);
  var hasAltGrKey = !!layoutView.view.mappings['c'];
  var skvPrefix = is102 ? '102kbd-k-' : '101kbd-k-';
  var skPrefix = layoutView.view.id + '-k-';
  var data = i18n.input.chrome.inputview.content.util.createData(keyCharacters,
      skvPrefix, is102, hasAltGrKey);
  if (data) {
    data[i18n.input.chrome.inputview.SpecNodeName.TITLE] =
        layoutView.view.title;
    data[i18n.input.chrome.inputview.SpecNodeName.ID] =
        e.layoutCode;
    this.dispatchEvent(new i18n.input.chrome.inputview.events.
        ConfigLoadedEvent(data));
  }
};


/**
 * Finds out the characters for the key.
 *
 * @param {!Object} mappings The mappings.
 * @param {string} code The code.
 * @return {!Array.<string>} The characters for the code.
 * @private
 */
i18n.input.chrome.inputview.M17nModel.prototype.findCharacters_ = function(
    mappings, code) {
  var characters = [];
  var states = [
    '',
    's',
    'c',
    'sc'
  ];
  for (var i = 0; i < states.length; i++) {
    if (mappings[states[i]] && mappings[states[i]][code]) {
      characters[i] = mappings[states[i]][code][1];
    } else {
      characters[i] = '';
    }
  }
  return characters;
};


/** @override */
i18n.input.chrome.inputview.M17nModel.prototype.disposeInternal = function() {
  goog.dispose(this.handler_);

  goog.base(this, 'disposeInternal');
};
