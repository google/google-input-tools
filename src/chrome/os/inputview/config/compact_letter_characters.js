// Copyright 2014 The ChromeOS IME Authors. All Rights Reserved.
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
goog.provide('i18n.input.chrome.inputview.content.compact.letter');

goog.require('i18n.input.chrome.inputview.content.constants');

goog.scope(function() {
var NonLetterKeys = i18n.input.chrome.inputview.content.constants.NonLetterKeys;

/**
 * Common qwerty Letter keyset characters.
 *
 * @return {!Array.<!Object>}
 */
i18n.input.chrome.inputview.content.compact.letter.keyQwertyCharacters =
    function() {
  return [
      /* 0 */ { 'text': 'q', 'hintText': '1' },
      /* 1 */ { 'text': 'w', 'hintText': '2' },
      /* 2 */ { 'text': 'e', 'hintText': '3',
        'moreKeys': ['\u00E8', '\u00E9', '\u00EA', '\u00EB', '\u0113']},
      /* 3 */ { 'text': 'r', 'hintText': '4' },
      /* 4 */ { 'text': 't', 'hintText': '5' },
      /* 5 */ { 'text': 'y', 'hintText': '6' },
      /* 6 */ { 'text': 'u', 'hintText': '7',
        'moreKeys': ['\u00FB', '\u00FC', '\u00F9', '\u00FA', '\u016B']},
      /* 7 */ { 'text': 'i', 'hintText': '8',
        'moreKeys': ['\u00EE', '\u00EF', '\u00ED', '\u012B', '\u00EC']},
      /* 8 */ { 'text': 'o', 'hintText': '9',
        'moreKeys': ['\u00F4', '\u00F6', '\u00F2', '\u00F3', '\u0153', '\u00F8',
                     '\u014D', '\u00F5']},
      /* 9 */ { 'text': 'p', 'hintText': '0' },
      /* 10 */ NonLetterKeys.BACKSPACE,
      /* 11 */ { 'text': 'a', 'marginLeftPercent': 0.33,
        'moreKeys': ['\u00E0', '\u00E1', '\u00E2', '\u00E4', '\u00E6', '\u00E3',
                     '\u00E5', '\u0101']},
      /* 12 */ { 'text': 's',
        'moreKeys': ['\u00DF']},
      /* 13 */ { 'text': 'd' },
      /* 14 */ { 'text': 'f' },
      /* 15 */ { 'text': 'g' },
      /* 16 */ { 'text': 'h' },
      /* 17 */ { 'text': 'j' },
      /* 18 */ { 'text': 'k' },
      /* 19 */ { 'text': 'l' },
      /* 20 */ NonLetterKeys.ENTER,
      /* 21 */ NonLetterKeys.LEFT_SHIFT,
      /* 22 */ { 'text': 'z' },
      /* 23 */ { 'text': 'x' },
      /* 24 */ { 'text': 'c',
        'moreKeys': ['\u00E7']},
      /* 25 */ { 'text': 'v' },
      /* 26 */ { 'text': 'b' },
      /* 27 */ { 'text': 'n',
        'moreKeys': ['\u00F1']},
      /* 28 */ { 'text': 'm' },
      /* 29 */ { 'text': '!',
        'moreKeys': ['\u00A1']},
      /* 30 */ { 'text': '?',
        'moreKeys': ['\u00BF']},
      /* 31 */ NonLetterKeys.RIGHT_SHIFT,
      /* 32 */ NonLetterKeys.SWITCHER,
      /* 33 */ NonLetterKeys.GLOBE,
      /* 34 */ NonLetterKeys.MENU,
      /* 35 */ { 'text': '/', 'isGrey': true },
      /* 36 */ NonLetterKeys.SPACE,
      /* 37 */ { 'text': ',', 'isGrey': true },
      /* 38 */ { 'text': '.', 'isGrey': true,
        'moreKeys': ['\u0023', '\u0021', '\u005C', '\u003F', '\u002D', '\u003A',
                     '\u0027', '\u0040']},
      /* 39 */ NonLetterKeys.HIDE
  ];
};

/**
 * Belgian Letter keyset characters.
 *
 * @return {!Array.<!Object>}
 */
i18n.input.chrome.inputview.content.compact.letter.keyBelgianCharacters =
    function() {
  var data =
      i18n.input.chrome.inputview.content.compact.letter.keyQwertyCharacters();
  data[2]['moreKeys'] =
      ['\u0117', '\u00E8', '\u0119', '\u00E9', '\u00EA', '\u00EB', '\u0113'];
  data[5]['moreKeys'] = ['\u0133'];
  data[7]['moreKeys'] =
      ['\u00EE', '\u00EF', '\u012F', '\u0133', '\u00ED', '\u012B', '\u00EC'];
  data[12]['moreKeys'] = undefined;
  data[24]['moreKeys'] = undefined;
  data[27]['moreKeys'] = ['\u00F1', '\u0144'];
  return data;
};


/**
 * Icelandic Letter keyset characters.
 *
 * @return {!Array.<!Object>}
 */
i18n.input.chrome.inputview.content.compact.letter.keyIcelandicCharacters =
    function() {
  var data =
      i18n.input.chrome.inputview.content.compact.letter.keyQwertyCharacters();
  data[2]['moreKeys'] = ['\u00E9', '\u00EB', '\u00E8', '\u00EA', '\u0119',
                         '\u0117', '\u0113'];  // e
  data[4]['moreKeys'] = ['\u00FE'];  // t
  data[5]['moreKeys'] = ['\u00FD', '\u00FF']; // y
  data[7]['moreKeys'] = ['\u00ED', '\u00EF', '\u00EE', '\u00EC', '\u012F',
                         '\u012B'];  // i
  data[12]['moreKeys'] = undefined;
  data[13]['moreKeys'] = ['\u00F0'];
  data[24]['moreKeys'] = undefined;
  return data;
};


/**
 * Qwertz Germany Letter keyset characters.
 *
 * @return {!Array.<!Object>}
 */
i18n.input.chrome.inputview.content.compact.letter.keyQwertzCharacters =
    function() {
  var data =
      i18n.input.chrome.inputview.content.compact.letter.keyQwertyCharacters();
  data[2]['moreKeys'] = ['\u00E9', '\u00E8', '\u00EA', '\u00EB', '\u0117'];
  data[7]['moreKeys'] = undefined;
  data[12]['moreKeys'] = ['\u00DF', '\u015B', '\u0161'];
  data[24]['moreKeys'] = undefined;
  data[27]['moreKeys'] = ['\u00F1', '\u0144'];

  data[5].text = 'z';
  data[22].text = 'y';
  return data;
};


/**
 * Common azerty Letter keyset characters.
 *
 * @return {!Array.<!Object>}
 */
i18n.input.chrome.inputview.content.compact.letter.keyAzertyCharacters =
    function() {
  return [
      /* 0 */ { 'text': 'a', 'hintText': '1',
        'moreKeys': ['\u00E0', '\u00E2', '\u00E6', '\u00E1', '\u00E4',
                     '\u00E3', '\u00E5', '\u0101', '\u00AA']},
      /* 1 */ { 'text': 'z', 'hintText': '2' },
      /* 2 */ { 'text': 'e', 'hintText': '3',
        'moreKeys': ['\u00E9', '\u00E8', '\u00EA', '\u00EB', '\u0119',
                     '\u0117', '\u0113']},
      /* 3 */ { 'text': 'r', 'hintText': '4' },
      /* 4 */ { 'text': 't', 'hintText': '5' },
      /* 5 */ { 'text': 'y', 'hintText': '6',
        'moreKeys': ['\u00FF']},
      /* 6 */ { 'text': 'u', 'hintText': '7',
        'moreKeys': ['\u00F9', '\u00FB', '\u00FC', '\u00FA', '\u016B']},
      /* 7 */ { 'text': 'i', 'hintText': '8',
        'moreKeys': ['\u00EE', '\u00EF', '\u00EC', '\u00ED', '\u012F',
                     '\u012B']},
      /* 8 */ { 'text': 'o', 'hintText': '9',
        'moreKeys': ['\u00F4', '\u0153', '\u00F6', '\u00F2', '\u00F3', '\u00F5',
                     '\u00F8', '\u014D', '\u00BA']},
      /* 9 */ { 'text': 'p', 'hintText': '0' },
      /* 10 */ NonLetterKeys.BACKSPACE,
      /* 11 */ { 'text': 'q' },
      /* 12 */ { 'text': 's' },
      /* 13 */ { 'text': 'd' },
      /* 14 */ { 'text': 'f' },
      /* 15 */ { 'text': 'g' },
      /* 16 */ { 'text': 'h' },
      /* 17 */ { 'text': 'j' },
      /* 18 */ { 'text': 'k' },
      /* 19 */ { 'text': 'l' },
      /* 20 */ { 'text': 'm' },
      /* 21 */ NonLetterKeys.ENTER,
      /* 22 */ NonLetterKeys.LEFT_SHIFT,
      /* 23 */ { 'text': 'w' },
      /* 24 */ { 'text': 'x' },
      /* 25 */ { 'text': 'c',
        'moreKeys': ['\u00E7', '\u0107', '\u010D']},
      /* 26 */ { 'text': 'v' },
      /* 27 */ { 'text': 'b' },
      /* 28 */ { 'text': 'n' },
      /* 29 */ { 'text': '\'',
        'moreKeys': ['\u2018', '\u201A', '\u2019', '\u2039', '\u203A']},
      /* 30 */ { 'text': '!',
        'moreKeys': ['\u00A1']},
      /* 31 */ { 'text': '?',
        'moreKeys': ['\u00BF']},
      /* 32 */ NonLetterKeys.RIGHT_SHIFT,
      /* 33 */ NonLetterKeys.SWITCHER,
      /* 34 */ NonLetterKeys.GLOBE,
      /* 35 */ NonLetterKeys.MENU,
      /* 36 */ { 'text': '/', 'isGrey': true },
      /* 37 */ NonLetterKeys.SPACE,
      /* 38 */ { 'text': ',', 'isGrey': true },
      /* 39 */ { 'text': '.', 'isGrey': true,
        'moreKeys': ['\u0023', '\u0021', '\u005C', '\u003F', '\u002D', '\u003A',
                     '\u0027', '\u0040']},
      /* 40 */ NonLetterKeys.HIDE
  ];
};


/**
 * Basic nordic Letter keyset characters(based on Finish).
 *
 * @return {!Array.<!Object>}
 */
i18n.input.chrome.inputview.content.compact.letter.keyNordicCharacters =
    function() {
  return [
      /* 0 */ { 'text': 'q', 'hintText': '1' },
      /* 1 */ { 'text': 'w', 'hintText': '2' },
      /* 2 */ { 'text': 'e', 'hintText': '3' },
      /* 3 */ { 'text': 'r', 'hintText': '4' },
      /* 4 */ { 'text': 't', 'hintText': '5' },
      /* 5 */ { 'text': 'y', 'hintText': '6' },
      /* 6 */ { 'text': 'u', 'hintText': '7',
        'moreKeys': ['\u00FC']},
      /* 7 */ { 'text': 'i', 'hintText': '8' },
      /* 8 */ { 'text': 'o', 'hintText': '9',
        'moreKeys': ['\u00F8', '\u00F4', '\u00F2', '\u00F3', '\u00F5', '\u0153',
                     '\u014D']},
      /* 9 */ { 'text': 'p', 'hintText': '0' },
      /* 10 */ { 'text': '\u00e5' },
      /* 11 */ NonLetterKeys.BACKSPACE,
      /* 12 */ { 'text': 'a',
        'moreKeys': ['\u00E6', '\u00E0', '\u00E1', '\u00E2', '\u00E3',
                     '\u0101']},
      /* 13 */ { 'text': 's',
        'moreKeys': ['\u0161', '\u00DF', '\u015B']},
      /* 14 */ { 'text': 'd' },
      /* 15 */ { 'text': 'f' },
      /* 16 */ { 'text': 'g' },
      /* 17 */ { 'text': 'h' },
      /* 18 */ { 'text': 'j' },
      /* 19 */ { 'text': 'k' },
      /* 20 */ { 'text': 'l' },
      /* 21 */ { 'text': '\u00f6',
        'moreKeys': ['\u00F8']},
      /* 22 */ { 'text': '\u00e4',
        'moreKeys': ['\u00E6']},
      /* 23 */ NonLetterKeys.ENTER,
      /* 24 */ NonLetterKeys.LEFT_SHIFT,
      /* 25 */ { 'text': 'z', 'marginLeftPercent': 0.33,
        'moreKeys': ['\u017E', '\u017A', '\u017C']},
      /* 26 */ { 'text': 'x' },
      /* 27 */ { 'text': 'c' },
      /* 28 */ { 'text': 'v' },
      /* 29 */ { 'text': 'b' },
      /* 30 */ { 'text': 'n' },
      /* 31 */ { 'text': 'm' },
      /* 32 */ { 'text': '!',
        'moreKeys': ['\u00A1']},
      /* 33 */ { 'text': '?', 'marginRightPercent': 0.33,
        'moreKeys': ['\u00BF']},
      /* 34 */ NonLetterKeys.RIGHT_SHIFT,
      /* 35 */ NonLetterKeys.SWITCHER,
      /* 36 */ NonLetterKeys.GLOBE,
      /* 37 */ NonLetterKeys.MENU,
      /* 38 */ { 'text': '/', 'isGrey': true },
      /* 39 */ NonLetterKeys.SPACE,
      /* 40 */ { 'text': ',', 'isGrey': true },
      /* 41 */ { 'text': '.', 'isGrey': true,
        'moreKeys': ['\u0023', '\u0021', '\u005C', '\u003F', '\u002D', '\u003A',
                     '\u0027', '\u0040']},
      /* 42 */ NonLetterKeys.HIDE
  ];
};


/**
 * Sweden nordic Letter keyset characters.
 *
 * @return {!Array.<!Object>}
 */
i18n.input.chrome.inputview.content.compact.letter.keySwedenCharacters =
    function() {
  var data =
      i18n.input.chrome.inputview.content.compact.letter.keyNordicCharacters();
  data[2]['moreKeys'] = ['\u00E9', '\u00E8', '\u00EA', '\u00EB',
                         '\u0119'];  // e
  data[3]['moreKeys'] = ['\u0159'];  // r
  data[4]['moreKeys'] = ['\u0165', '\u00FE'];  // t
  data[5]['moreKeys'] = ['\u00FD', '\u00FF'];  // y
  data[6]['moreKeys'] = ['\u00FC', '\u00FA', '\u00F9', '\u00FB',
                         '\u016B'];  // u
  data[7]['moreKeys'] = ['\u00ED', '\u00EC', '\u00EE', '\u00EF'];  // i
  data[8]['moreKeys'] = ['\u00F3', '\u00F2', '\u00F4', '\u00F5',
                         '\u014D'];  // o
  data[12]['moreKeys'] = ['\u00E1', '\u00E0', '\u00E2', '\u0105',
                          '\u00E3'];  // a
  data[13]['moreKeys'] = ['\u015B', '\u0161', '\u015F', '\u00DF'];  // s
  data[14]['moreKeys'] = ['\u00F0', '\u010F'];  // d
  data[20]['moreKeys'] = ['\u0142'];  // l
  data[21]['moreKeys'] = ['\u00F8', '\u0153'];
  data[22]['moreKeys'] = ['\u00E6'];
  data[25]['moreKeys'] = ['\u017A', '\u017E', '\u017C'];  //z
  data[27]['moreKeys'] = ['\u00E7', '\u0107', '\u010D'];  //c
  data[30]['moreKeys'] = ['\u0144', '\u00F1', '\u0148'];  //n

  return data;
};


/**
 * Norway nordic Letter keyset characters.
 *
 * @return {!Array.<!Object>}
 */
i18n.input.chrome.inputview.content.compact.letter.keyNorwayCharacters =
    function() {
  var data =
      i18n.input.chrome.inputview.content.compact.letter.keyNordicCharacters();
  data[2]['moreKeys'] = ['\u00E9', '\u00E8', '\u00EA', '\u00EB', '\u0119',
                         '\u0117', '\u0113'];  // e
  data[6]['moreKeys'] = ['\u00FC', '\u00FB', '\u00F9', '\u00FA',
                         '\u016B'];  // u
  data[8]['moreKeys'] = ['\u00F4', '\u00F2', '\u00F3', '\u00F6', '\u00F5',
                         '\u0153', '\u014D'];  // o
  data[12]['moreKeys'] = ['\u00E0', '\u00E4', '\u00E1', '\u00E2', '\u00E3',
                          '\u0101'];  // a
  data[13]['moreKeys'] = undefined;  //s
  data[21]['moreKeys'] = ['\u00F6'];
  data[22]['moreKeys'] = ['\u00E4'];
  data[25]['moreKeys'] = undefined;  //z

  data[21]['text'] = '\u00f8';
  data[22]['text'] = '\u00e6';

  return data;
};


/**
 * Denmark nordic Letter keyset characters.
 *
 * @return {!Array.<!Object>}
 */
i18n.input.chrome.inputview.content.compact.letter.keyDenmarkCharacters =
    function() {
  var data =
      i18n.input.chrome.inputview.content.compact.letter.keyNordicCharacters();
  data[2]['moreKeys'] = ['\u00E9', '\u00EB'];  // e
  data[5]['moreKeys'] = ['\u00FD', '\u00FF'];  // y
  data[6]['moreKeys'] = ['\u00FA', '\u00FC', '\u00FB', '\u00F9',
                         '\u016B'];  // u
  data[7]['moreKeys'] = ['\u00ED', '\u00EF'];  // i
  data[8]['moreKeys'] = ['\u00F3', '\u00F4', '\u00F2', '\u00F5',
                         '\u0153', '\u014D'];  // o
  data[12]['moreKeys'] = ['\u00E1', '\u00E4', '\u00E0', '\u00E2', '\u00E3',
                          '\u0101'];  // a
  data[13]['moreKeys'] = ['\u00DF', '\u015B', '\u0161'];  // s
  data[14]['moreKeys'] = ['\u00F0'];  // d
  data[20]['moreKeys'] = ['\u0142'];  // l
  data[21]['moreKeys'] = ['\u00E4'];
  data[22]['moreKeys'] = ['\u00F6'];
  data[25]['moreKeys'] = undefined;  //z
  data[30]['moreKeys'] = ['\u00F1', '\u0144'];  //n

  data[21]['text'] = '\u00e6';
  data[22]['text'] = '\u00f8';

  return data;
};

});  // goog.scope
