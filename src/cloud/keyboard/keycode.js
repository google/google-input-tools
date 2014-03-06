// Copyright 2014 The Cloud Input Tools Authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS-IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * @fileoverview Defines all the key codes and key codes map.
 */

goog.provide('i18n.input.keyboard.KeyCode');

goog.require('goog.object');


/**
 * The standard 101 keyboard keys. Each char represent the key code of each key.
 *
 * @type {string}
 */
i18n.input.keyboard.KeyCode.CODES101 =
    '\u00c01234567890\u00bd\u00bb' +
    'QWERTYUIOP\u00db\u00dd\u00dc' +
    'ASDFGHJKL\u00ba\u00de' +
    'ZXCVBNM\u00bc\u00be\u00bf' +
    '\u0020';


/**
 * The standard 102 keyboard keys.
 *
 * @type {string}
 */
i18n.input.keyboard.KeyCode.CODES102 =
    '\u00c01234567890\u00bd\u00bb' +
    'QWERTYUIOP\u00db\u00dd' +
    'ASDFGHJKL\u00ba\u00de\u00dc' +
    '\u00e2ZXCVBNM\u00bc\u00be\u00bf' +
    '\u0020';


/**
 * The standard 101 keyboard keys, including the BS/TAB/CAPS/ENTER/SHIFT/ALTGR.
 * Each char represent the key code of each key.
 *
 * @type {string}
 */
i18n.input.keyboard.KeyCode.ALLCODES101 =
    '\u00c01234567890\u00bd\u00bb\u0008' +
    '\u0009QWERTYUIOP\u00db\u00dd\u00dc' +
    '\u0014ASDFGHJKL\u00ba\u00de\u000d' +
    '\u0010ZXCVBNM\u00bc\u00be\u00bf\u0010' +
    '\u0111\u0020\u0111';


/**
 * The standard 102 keyboard keys, including the BS/TAB/CAPS/ENTER/SHIFT/ALTGR.
 * Each char represent the key code of each key.
 *
 * @type {string}
 */
i18n.input.keyboard.KeyCode.ALLCODES102 =
    '\u00c01234567890\u00bd\u00bb\u0008' +
    '\u0009QWERTYUIOP\u00db\u00dd\u000d' +
    '\u0014ASDFGHJKL\u00ba\u00de\u00dc\u000d' +
    '\u0010\u00e2ZXCVBNM\u00bc\u00be\u00bf\u0010' +
    '\u0111\u0020\u0111';


/**
 * OEM keyboard remapping table:
 * for each type (de, fr) we map a keyboard char to its "actual"
 * English counterpart: e.g. French AZERTY is decoded as QWERTY, etc.
 * The map is from char of key code to key code.
 *
 * @type {Object.<Object.<number>>}
 */
i18n.input.keyboard.KeyCode.OEM_CODES = {
  'de': goog.object.create([
    'Y', 90,         // Z
    'Z', 89,         // Y
    '\u00dc', 0xc0,  // \ -> `
    '\u00db', 0xbd,  // [ -> -
    '\u00dd', 0xbb,  // ] -> =
    '\u00ba', 0xdb,  // ; -> [
    '\u00bb', 0xdd,  // = -> ]
    '\u00bf', 0xdc,  // / -> \
    '\u00cc', 0xba,  //   -> ;
    '\u00bd', 0xbf   // - -> /
  ]),
  'fr': goog.object.create([
    'Q', 65,         // A
    'A', 81,         // Q
    'Z', 87,         // W
    'W', 90,         // Z
    '\u00dd', 0xdb,  // ] -> [
    '\u00ba', 0xdd,  // ; -> ]
    'M', 0xba,       // M -> ;
    '\u00c0', 0xde,  // ` -> '
    '\u00de', 0xc0,  // ' -> `
    '\u00bc', 77,    // , -> M
    '\u00be', 0xbc,  // . -> ,
    '\u00bf', 0xbe,  // / -> .
    '\u00df', 0xbf,  //   -> /
    '\u00db', 0xbd   // [ -> -
  ])
};


/**
 * Mozilla key map. From key code to key code.
 *
 * @type {Object.<number>}
 */
i18n.input.keyboard.KeyCode.MOZ_CODES = {
  59: 186,
  61: 187,
  107: 187,
  109: 189,
  173: 189
};


/**
 * Mozilla key map for sepcial keys with SHIFT. From char code to key code.
 * This is for Firefox on Mac when holding SHIFT. For some chars, Firefox
 * assgines 0 to keyCode of keydown and keyup events. So we need to get
 * charCode from keypress event, and translate it to keyCode.
 *
 * @type {Object.<number>}
 */
i18n.input.keyboard.KeyCode.MOZ_SHIFT_CHAR_CODES = {
  126: 192,
  95: 189,
  //34: 222,
  //43: 187,
  //123: 219,
  //125: 221,
  124: 220,
  58: 186,
  60: 188,
  62: 190,
  63: 191
};
