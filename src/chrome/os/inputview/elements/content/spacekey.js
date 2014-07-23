goog.provide('i18n.input.chrome.inputview.elements.content.SpaceKey');

goog.require('goog.dom');
goog.require('goog.dom.classlist');
goog.require('i18n.input.chrome.inputview.Css');
goog.require('i18n.input.chrome.inputview.StateType');
goog.require('i18n.input.chrome.inputview.elements.ElementType');
goog.require('i18n.input.chrome.inputview.elements.content.FunctionalKey');



goog.scope(function() {



/**
 * The space key.
 *
 * @param {string} id the id.
 * @param {!i18n.input.chrome.inputview.StateManager} stateManager The state
 *     manager.
 * @param {string} title The keyboard title.
 * @param {!Array.<string>=} opt_characters The characters.
 * @param {goog.events.EventTarget=} opt_eventTarget The event target.
 * @param {string=} opt_iconCss The icon CSS class
 * @constructor
 * @extends {i18n.input.chrome.inputview.elements.content.FunctionalKey}
 */
i18n.input.chrome.inputview.elements.content.SpaceKey = function(id,
    stateManager, title, opt_characters, opt_eventTarget, opt_iconCss) {
  goog.base(this, id, i18n.input.chrome.inputview.elements.ElementType.
      SPACE_KEY, opt_iconCss ? '' : title, opt_iconCss || '', opt_eventTarget);

  /**
   * The characters.
   *
   * @type {!Array.<string>}
   * @private
   */
  this.characters_ = opt_characters || [];

  /**
   * The state manager.
   *
   * @type {!i18n.input.chrome.inputview.StateManager}
   * @private
   */
  this.stateManager_ = stateManager;
};
goog.inherits(i18n.input.chrome.inputview.elements.content.SpaceKey,
    i18n.input.chrome.inputview.elements.content.FunctionalKey);
var SpaceKey = i18n.input.chrome.inputview.elements.content.SpaceKey;


/** @override */
SpaceKey.prototype.createDom = function() {
  goog.base(this, 'createDom');

  goog.dom.classlist.remove(this.bgElem,
      i18n.input.chrome.inputview.Css.SPECIAL_KEY_BG);
};


/**
 * Gets the character.
 *
 * @return {string} The character.
 */
SpaceKey.prototype.getCharacter = function() {
  if (this.characters_) {
    // The index is based on the characters in order:
    // 0: Default
    // 1: Shift
    // 2: ALTGR
    // 3: SHIFT + ALTGR
    var index = this.stateManager_.hasState(i18n.input.chrome.inputview.
        StateType.SHIFT) ? 1 : 0 + this.stateManager_.hasState(
            i18n.input.chrome.inputview.StateType.ALTGR) ? 2 : 0;
    if (this.characters_.length > index && this.characters_[index]) {
      return this.characters_[index];
    }
  }
  return ' ';
};


/**
 * True to set the title visible.
 *
 * @param {boolean} visible True to set title visible.
 */
SpaceKey.prototype.setTitleVisible = function(
    visible) {
  if (this.textElem) {
    goog.dom.setTextContent(this.textElem, visible ? this.text : '');
    goog.dom.classlist.add(this.textElem,
        i18n.input.chrome.inputview.Css.TITLE);
  }
};

});  // goog.scope
