goog.provide('i18n.input.chrome.inputview.elements.content.MenuKey');



goog.scope(function() {



/**
 * The three dots menu key.
 *
 * @param {string} id The id.
 * @param {!i18n.input.chrome.inputview.elements.ElementType} type The element
 *     type.
 * @param {string} text The text.
 * @param {string} iconCssClass The css class for the icon.
 * @param {string} toKeyset The keyset id.
 * @param {goog.events.EventTarget=} opt_eventTarget The event target.
 * @constructor
 * @extends {i18n.input.chrome.inputview.elements.content.FunctionalKey}
 */
i18n.input.chrome.inputview.elements.content.MenuKey = function(id, type,
    text, iconCssClass, toKeyset, opt_eventTarget) {
  goog.base(this, id, type, text, iconCssClass, opt_eventTarget);

  /**
   * The id of the key set to go after this switcher key in menu is pressed.
   *
   * @type {string}
   */
  this.toKeyset = toKeyset;

};
goog.inherits(i18n.input.chrome.inputview.elements.content.MenuKey,
    i18n.input.chrome.inputview.elements.content.FunctionalKey);
var MenuKey = i18n.input.chrome.inputview.elements.content.MenuKey;


/** @override */
MenuKey.prototype.resize = function(width,
    height) {
  goog.base(this, 'resize', width, height);

  // TODO: override width to remove space between menu key and the
  // following key.
};



});  // goog.scope
