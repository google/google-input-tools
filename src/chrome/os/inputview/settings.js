goog.provide('i18n.input.chrome.inputview.Settings');


goog.scope(function() {



/**
 * The settings.
 *
 * @constructor
 */
i18n.input.chrome.inputview.Settings = function() {};
var Settings = i18n.input.chrome.inputview.Settings;


/**
 * True to always render the altgr character in the soft key.
 *
 * @type {boolean}
 */
Settings.prototype.alwaysRenderAltGrCharacter = false;


/** @type {boolean} */
Settings.prototype.autoSpace = false;


/** @type {boolean} */
Settings.prototype.autoCapital = false;


/** @type {boolean} */
Settings.prototype.autoCorrection = false;


/** @type {boolean} */
Settings.prototype.supportCompact = false;


/** @type {boolean} */
Settings.prototype.enableLongPress = true;


/**
 * The flag to control whether candidates naviagation feature is enabled.
 *
 * @type {boolean}
 */
Settings.prototype.candidatesNavigation = false;


/**
 * Saves the preferences.
 *
 * @param {string} preference The name of the preference.
 * @param {*} value The preference value.
 */
Settings.prototype.savePreference = function(preference, value) {
  window.localStorage.setItem(preference, /** @type {string} */(value));
};


/**
 * Gets the preference value.
 *
 * @param {string} preference The name of the preference.
 * @return {*} The value.
 */
Settings.prototype.getPreference = function(preference) {
  return window.localStorage.getItem(preference);
};

});  // goog.scope

