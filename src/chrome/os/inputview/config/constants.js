goog.provide('i18n.input.chrome.inputview.content.constants');

goog.require('i18n.input.chrome.inputview.Css');
goog.require('i18n.input.chrome.inputview.StateType');
goog.require('i18n.input.chrome.inputview.elements.ElementType');

goog.scope(function() {

var ElementType = i18n.input.chrome.inputview.elements.ElementType;

/**
 * The non letter keys.
 *
 * @const
 * @enum {Object}
 */
i18n.input.chrome.inputview.content.constants.NonLetterKeys = {
  BACKSPACE: {
      'iconCssClass': i18n.input.chrome.inputview.Css.BACKSPACE_ICON,
      'type': ElementType.BACKSPACE_KEY,
      'id': 'Backspace'
  },
  ENTER: {
      'iconCssClass': i18n.input.chrome.inputview.Css.ENTER_ICON,
      'type': ElementType.ENTER_KEY,
      'id': 'Enter'
  },
  HIDE: {
      'iconCssClass': i18n.input.chrome.inputview.Css.HIDE_KEYBOARD_ICON,
      'type': ElementType.HIDE_KEYBOARD_KEY,
      'id': 'HideKeyboard'
  },
  LEFT_SHIFT: {
      'toState': i18n.input.chrome.inputview.StateType.SHIFT,
      'iconCssClass': i18n.input.chrome.inputview.Css.SHIFT_ICON,
      'type': ElementType.MODIFIER_KEY,
      'id': 'ShiftLeft',
      'supportSticky': true
  },
  RIGHT_SHIFT: {
      'toState': i18n.input.chrome.inputview.StateType.SHIFT,
      'iconCssClass': i18n.input.chrome.inputview.Css.SHIFT_ICON,
      'type': ElementType.MODIFIER_KEY,
      'id': 'ShiftRight',
      'supportSticky': true
  },
  SPACE: {
      'name': ' ',
      'type': ElementType.SPACE_KEY,
      'id': 'Space'
  },
  SWITCHER: {
      'type': ElementType.SWITCHER_KEY
  },
  MENU: {
      'type': ElementType.MENU_KEY,
      'name': '\u22EE',
      'id': 'Menu'
  },
  GLOBE: {
      'iconCssClass': i18n.input.chrome.inputview.Css.GLOBE_ICON,
      'type': ElementType.GLOBE_KEY,
      'id': 'Globe'
  }
};


});  // goog.scope
