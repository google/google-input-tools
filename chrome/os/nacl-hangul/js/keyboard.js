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
// Copyright 2013 Google Inc. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * @fileoverview Keyboard layouts of Hangul IME.
 */

'use strict';

/**
 * Enum of Keyboard layout.
 *
 * @enum {string}
 */
HangulIme.Keyboard = {
  DUBEOLSIK: 'hangul_2set',
  SEBEOLSIK_390: 'hangul_3set390',
  SEBEOLSIK_FINAL: 'hangul_3setfinal',
  SEBEOLSIK_NOSHIFT: 'hangul_3setnoshift',
  ROMAJA: 'hangul_romaja',
  AHNMATAE: 'hangul_ahnmatae'
};

/**
 * Layouts of Korean keyboards
 *
 * @type {Array}
 */
HangulIme.keyboardLayouts = [];

HangulIme.keyboardLayouts.push({
  id: HangulIme.Keyboard.DUBEOLSIK,
  name: '2',
  validKeys: 'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ',
  charMap: {
    '$': '\u20A9',
  }
});

HangulIme.keyboardLayouts.push({
  id: HangulIme.Keyboard.SEBEOLSIK_390,
  name: '39',
  validKeys: 'abcdefghijklmnopqrstuvwxyz1234567890!QWERASDFZXCV/;\'',
  charMap: {
    'T': ';',
    'Y': '<',
    'U': '7',
    'I': '8',
    'O': '9',
    'P': '>',
    'G': '/',
    'H': '\'',
    'J': '4',
    'K': '5',
    'L': '6',
    'B': '!',
    'N': '0',
    'M': '1',
    '<': '2',
    '>': '3',
  }
});

HangulIme.keyboardLayouts.push({
  id: HangulIme.Keyboard.SEBEOLSIK_FINAL,
  name: '3f',
  validKeys: 'abcdefghijklmnopqrstuvwxyz1234567890!@#$%QWERTASDFGZXCV/;\'',
  charMap: {
    '^': '=',
    '&': '“',
    '*': '”',
    '(': '\'',
    ')': '~',
    '-': ')',
    '=': '>',
    '_': ';',
    '+': '+',
    'Y': '5',
    'U': '6',
    'I': '7',
    'O': '8',
    'P': '9',
    '[': '(',
    ']': '<',
    '{': '%',
    '}': '/',
    '\\': ':',
    '|': '\\',
    'H': '0',
    'J': '1',
    'K': '2',
    'L': '3',
    ':': '4',
    '"': '·',
    'N': '-',
    'M': '"',
    '<': ',',
    '>': '.',
    '?': '!',
    'B': '?',
  }
});

HangulIme.keyboardLayouts.push({
  id: HangulIme.Keyboard.SEBEOLSIK_NOSHIFT,
  name: '3s',
  validKeys: 'abcdefghijklmnopqrstuvwxyz1234567890QWERASDFZXCV-=[]\\;\'/',
  charMap: {
    'T': ';',
    'Y': '<',
    'U': '7',
    'I': '8',
    'O': '9',
    'P': '>',
    'G': '/',
    'H': '\'',
    'J': '4',
    'K': '5',
    'L': '6',
    'B': '!',
    'N': '0',
    'M': '1',
    '<': '2',
    '>': '3',
  }
});

HangulIme.keyboardLayouts.push({
  id: HangulIme.Keyboard.ROMAJA,
  name: 'ro',
  validKeys: 'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ',
  charMap: {}
});

HangulIme.keyboardLayouts.push({
  id: HangulIme.Keyboard.AHNMATAE,
  name: 'ahn',
  validKeys: 'abcdefghijklmnopqrstuvwxyzACDEFHKLMQRSVXZ;,./',
  charMap: {
    'W': '\u317f',
    'T': '\u3186',
    'Y': ';',
    'U': '\'',
    'I': '/',
    'O': '[',
    'P': ']',
    '[': ',',
    ']': '?',
    'G': 'ㆁ',
    'J': 'ㆍ',
    '\'': '.',
    'B': '\u11f0',
    'N': '\u11eb',
    '?': '\u11f9',
  }
});
