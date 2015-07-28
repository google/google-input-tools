// Copyright 2012 Google Inc. All Rights Reserved.

/**
 * @fileoverview The utility functions for languages of input tools.
 * @author shuchen@google.com (Shu Chen)
 */

goog.provide('i18n.input.lang.LanguageUtil');

goog.require('goog.array');
goog.require('goog.object');
goog.require('i18n.input.common.GlobalSettings');
goog.require('i18n.input.lang.InputTool');
goog.require('i18n.input.lang.InputToolInfo');
goog.require('i18n.input.lang.InputToolType');
goog.require('proto.i18n_input.InputToolsProto.Application');


/**
 * Application name to application id map.
 *
 * @type {!Object.<string, !proto.i18n_input.InputToolsProto.Application>}
 */
i18n.input.lang.LanguageUtil.ApplicationNameId = {
  'translate': proto.i18n_input.InputToolsProto.Application.TRANSLATE,
  'kix': proto.i18n_input.InputToolsProto.Application.KIX,
  'trix': proto.i18n_input.InputToolsProto.Application.TRIX,
  'punch': proto.i18n_input.InputToolsProto.Application.PUNCH,
  'gmail': proto.i18n_input.InputToolsProto.Application.GMAIL
};


/**
 * Gets the input tools from the given language.
 *
 * @param {string} languageCode The language code.
 * @param {proto.i18n_input.InputToolsProto.Application=} opt_app
 *     The optional application id.
 * @return {!Array.<string>} The input tool code list.
 */
i18n.input.lang.LanguageUtil.getInputToolsFromLanguage =
    function(languageCode, opt_app) {
  if (!opt_app) {
    opt_app = i18n.input.lang.LanguageUtil.ApplicationNameId[
        i18n.input.common.GlobalSettings.ApplicationName];
  }
  var lang = i18n.input.lang.LanguageUtil.getStandardLanguageCode(languageCode);
  var inputTools = [];
  var list = i18n.input.lang.InputToolInfo.LANGUGAGE_INPUT_TOOL[lang];
  if (list) {
    goog.array.forEach(list, function(entry) {
      var inputTool = i18n.input.lang.InputTool.get(
          /** @type {!string} */ (entry.getCode()));
      if (!opt_app || !goog.array.contains(entry.getDisabledAppList(),
          opt_app)) {
        inputTools.push(entry.getCode());
      }
    });
  }
  return inputTools;
};


/**
 * Gets the input tools from the given language list.
 *
 * @param {!Array.<string>} languages The language code list.
 * @param {proto.i18n_input.InputToolsProto.Application=} opt_app
 *     The optional application id.
 * @return {!Array.<string>} The input tool code list.
 */
i18n.input.lang.LanguageUtil.getInputToolsFromLanguages = function(languages,
    opt_app) {
  var inputToolCodes = goog.array.map(languages, function(lang) {
    return i18n.input.lang.LanguageUtil.getInputToolsFromLanguage(
        lang, opt_app);
  });
  inputToolCodes = goog.array.flatten(inputToolCodes);
  goog.array.removeDuplicates(inputToolCodes);
  return inputToolCodes;
};


/**
 * Gets the list of supported languages.
 *
 * @param {proto.i18n_input.InputToolsProto.Application=} opt_app
 *     The optional application id.
 * @return {!Array.<string>} The supported languages.
 */
i18n.input.lang.LanguageUtil.getSupportedLanguages = function(opt_app) {
  var langs = [];

  goog.object.forEach(i18n.input.lang.InputToolInfo.LANGUGAGE_INPUT_TOOL,
      function(value, key) {
        if (!opt_app ||
            !goog.array.every(value, function(entry) {
              return goog.array.contains(
                  entry.getDisabledAppList(), opt_app);
            })) {
          langs.push(key);
        }
      });
  return langs;
};


/**
 * Gets the standard language code from the given language code.
 *
 * @param {string} lang The maybe-non-standard language code.
 * @return {string} The standard language code.
 */
i18n.input.lang.LanguageUtil.getStandardLanguageCode = function(lang) {
  lang = lang.replace(/_/g, '-').toLowerCase();

  // Special alias cases.
  if (lang.indexOf('zh-tw') == 0 || lang.indexOf('zh-hk') == 0) {
    return 'zh-Hant';
  } else if (lang == 'zh' || lang.indexOf('zh-cn') == 0) {
    return 'zh-Hans';
  } else if (lang == 'pt') {
    return 'pt-BR';
  }

  var parts = lang.split(/[\-]/g);
  var validLang = '';
  lang = parts[0];
  if (lang == 'iw') {
    lang = 'he';
  }
  if (i18n.input.lang.InputToolInfo.LANGUGAGE_INPUT_TOOL[lang]) {
    validLang = lang;
  }

  for (var i = 1; i < parts.length; i++) {
    var part = parts[i];
    if (part.length == 2) {
      part = part.toUpperCase();
    } else if (part.length == 4) {
      part = part.charAt(0).toUpperCase() + part.slice(1);
    }
    lang += '-' + part;

    if (i18n.input.lang.InputToolInfo.LANGUGAGE_INPUT_TOOL[lang]) {
      validLang = lang;
    }
  }
  return validLang;
};

