// Copyright 2007, Google Inc.
// All Rights Reserved.

/**
 * @fileoverview translit.lang.LanguageCode lists the available languages
 * and their language codes.
 *
 * @author skanuj@google.com (Anuj Sharma)
*/

goog.provide('i18n.input.lang.LanguageCode');
goog.provide('i18n.input.lang.LanguageCodePair');


/**
 * Available language codes as two character strings
 *
 * @enum {string}
 */
i18n.input.lang.LanguageCode = {
  AMHARIC: 'am',
  ARABIC: 'ar',
  BELARUSIAN: 'be',
  BENGALI: 'bn',
  BULGARIAN: 'bg',
  DUTCH: 'nl',
  ENGLISH: 'en',
  FRENCH: 'fr',
  GERMAN: 'de',
  GREEK: 'el',
  GUJARATI: 'gu',
  HEBREW: 'he',
  HEBREW_IW: 'iw',
  HINDI: 'hi',
  HIRAGANA: 'ja-Hira',
  ITALIAN: 'it',
  JAPANESE: 'ja',
  KANNADA: 'kn',
  MALAYALAM: 'ml',
  MARATHI: 'mr',
  NEPALI: 'ne',
  ORIYA: 'or',
  PERSIAN: 'fa',
  POLISH: 'pl',
  PORTUGUESE: 'pt',
  PUNJABI: 'pa',
  RUSSIAN: 'ru',
  SANSKRIT: 'sa',
  SERBIAN: 'sr',
  SERBIAN_LATIN: 'sr-Latn',
  SINHALESE: 'si',
  SPANISH: 'es',
  TAMIL: 'ta',
  TELUGU: 'te',
  TIGRINYA: 'ti',
  TURKISH: 'tr',
  UKRAINE: 'uk',
  URDU: 'ur',
  VIETNAMESE: 'vi',
  SIMPLIFIED_CHINESE: 'zh-Hans',
  TRADITIONAL_CHINESE: 'zh-Hant'
};



/**
 * The language code pair.
 *
 * @param {!i18n.input.lang.LanguageCode} sourceLangCode
 *     The source language code.
 * @param {!i18n.input.lang.LanguageCode} targetLangCode
 *     The target language code.
 * @constructor
 */
i18n.input.lang.LanguageCodePair = function(sourceLangCode, targetLangCode) {

  /**
   * @type {!i18n.input.lang.LanguageCode}
   */
  this.sourceLangCode = sourceLangCode;

  /**
   * @type {!i18n.input.lang.LanguageCode}
   */
  this.targetLangCode = targetLangCode;

  /**
   * @type {string}
   * @private
   */
  this.string_ =
      i18n.input.lang.LanguageCodePair.toString(sourceLangCode, targetLangCode);
};


/**
 * @type {Object.<string, i18n.input.lang.LanguageCodePair>}
 * @private
 */
i18n.input.lang.LanguageCodePair.instances_ = {};


/**
 * 'toString' method.
 *
 * @param {i18n.input.lang.LanguageCode} sourceLangCode
 *     The source language code.
 * @param {i18n.input.lang.LanguageCode} targetLangCode
 *     The target language code.
 * @return {string} The string.
 */
i18n.input.lang.LanguageCodePair.toString =
    function(sourceLangCode, targetLangCode) {
  return [sourceLangCode, targetLangCode].join('|');
};


/**
 * Gets the language code pair.
 *
 * @param {!i18n.input.lang.LanguageCode} sourceLangCode
 *     The source language code.
 * @param {!i18n.input.lang.LanguageCode} targetLangCode
 *     The target language code.
 * @return {!i18n.input.lang.LanguageCodePair} The language code pair.
 */
i18n.input.lang.LanguageCodePair.get =
    function(sourceLangCode, targetLangCode) {
  // Hacks to support both 'iw' and 'he'.
  if (sourceLangCode == i18n.input.lang.LanguageCode.HEBREW_IW) {
    sourceLangCode = i18n.input.lang.LanguageCode.HEBREW;
  } else if (targetLangCode == i18n.input.lang.LanguageCode.HEBREW_IW) {
    targetLangCode = i18n.input.lang.LanguageCode.HEBREW;
  }
  var langCodePairStr =
      i18n.input.lang.LanguageCodePair.toString(sourceLangCode, targetLangCode);
  return i18n.input.lang.LanguageCodePair.instances_[langCodePairStr] ||
         (i18n.input.lang.LanguageCodePair.instances_[langCodePairStr] =
          new i18n.input.lang.LanguageCodePair(sourceLangCode, targetLangCode));
};


/** @override */
i18n.input.lang.LanguageCodePair.prototype.toString = function() {
  return this.string_;
};


/**
 * Inverses the language code pair.
 *
 * @return {!i18n.input.lang.LanguageCodePair} The inverted language code pair.
 */
i18n.input.lang.LanguageCodePair.prototype.inverse = function() {
  return i18n.input.lang.LanguageCodePair.get(
      this.targetLangCode, this.sourceLangCode);
};

