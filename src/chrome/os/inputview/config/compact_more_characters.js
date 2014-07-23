goog.provide('i18n.input.chrome.inputview.content.compact.more');

goog.require('i18n.input.chrome.inputview.content.constants');

goog.scope(function() {
var NonLetterKeys = i18n.input.chrome.inputview.content.constants.NonLetterKeys;

/**
 * North American More keyset characters.
 *
 * @return {!Array.<!Object>}
 */
i18n.input.chrome.inputview.content.compact.more.keyNAMoreCharacters =
    function() {
  return [
      /* 0 */ { 'text': '~' },
      /* 1 */ { 'text': '`' },
      /* 2 */ { 'text': '|' },
      // Keep in sync with rowkeys_symbols_shift1.xml in android input tool.
      /* 3 */ { 'text': '\u2022',
        'moreKeys': ['\u266A', '\u2665', '\u2660', '\u2666', '\u2663']},
      /* 4 */ { 'text': '\u23B7' },
      // Keep in sync with rowkeys_symbols_shift1.xml in android input tool.
      /* 5 */ { 'text': '\u03C0',
        'moreKeys': ['\u03A0']},
      /* 6 */ { 'text': '\u00F7' },
      /* 7 */ { 'text': '\u00D7' },
      /* 8 */ { 'text': '\u00B6',
        'moreKeys': ['\u00A7']},
      /* 9 */ { 'text': '\u0394' },
      /* 10 */ NonLetterKeys.BACKSPACE,
      /* 11 */ { 'text': '\u00A3', 'marginLeftPercent': 0.33 },
      /* 12 */ { 'text': '\u00A2' },
      /* 13 */ { 'text': '\u20AC' },
      /* 14 */ { 'text': '\u00A5' },
      // Keep in sync with rowkeys_symbols_shift2.xml in android input tool.
      /* 15 */ { 'text': '^',
        'moreKeys': ['\u2191', '\u2193', '\u2190', '\u2192']},
      /* 16 */ { 'text': '\u00B0',
        'moreKeys': ['\u2032', '\u2033']},
      /* 17 */ { 'text': '=',
        'moreKeys': ['\u2260', '\u2248', '\u221E']},
      /* 18 */ { 'text': '{' },
      /* 19 */ { 'text': '}' },
      /* 20 */ NonLetterKeys.ENTER,
      /* 21 */ NonLetterKeys.SWITCHER,
      /* 22 */ { 'text': '\\' },
      /* 23 */ { 'text': '\u00A9' },
      /* 24 */ { 'text': '\u00AE' },
      /* 25 */ { 'text': '\u2122' },
      /* 26 */ { 'text': '\u2105' },
      /* 27 */ { 'text': '[' },
      /* 28 */ { 'text': ']' },
      /* 29 */ { 'text': '\u00A1' },
      /* 30 */ { 'text': '\u00BF' },
      /* 31 */ NonLetterKeys.SWITCHER,
      /* 32 */ NonLetterKeys.SWITCHER,
      /* 33 */ { 'text': '<', 'isGrey': true,
        'moreKeys': ['\u2039', '\u2264', '\u00AB']},
      /* 34 */ NonLetterKeys.MENU,
      /* 35 */ { 'text': '>', 'isGrey': true,
        'moreKeys': ['\u203A', '\u2265', '\u00BB']},
      /* 36 */ NonLetterKeys.SPACE,
      /* 37 */ { 'text': ',', 'isGrey': true },
      /* 38 */ { 'text': '.', 'isGrey': true,
        'moreKeys': ['\u2026']},
      /* 39 */ NonLetterKeys.HIDE
  ];
};


/**
 * Gets United Kingdom More keyset characters.
 *
 * @return {!Array.<!Object>}
 */
i18n.input.chrome.inputview.content.compact.more.keyUKMoreCharacters =
    function() {
  // Copy North America more characters.
  var data = i18n.input.chrome.inputview.content.compact.more.
      keyNAMoreCharacters();
  data[11]['text'] = '\u20AC';  // pound -> euro
  data[12]['text'] = '\u00A5';  // cent -> yen
  data[13]['text'] = '$';  // euro -> dollar
  data[13]['moreKeys'] = ['\u00A2'];
  data[14]['text'] = '\u00A2';  // yen -> cent
  return data;
};


/**
 * Gets European More keyset characters.
 *
 * @return {!Array.<!Object>}
 */
i18n.input.chrome.inputview.content.compact.more.keyEUMoreCharacters =
    function() {
  // Copy UK more characters.
  var data = i18n.input.chrome.inputview.content.compact.more.
      keyUKMoreCharacters();
  data[11]['text'] = '\u00A3';  // euro -> pound
  return data;
};

});  // goog.scope
