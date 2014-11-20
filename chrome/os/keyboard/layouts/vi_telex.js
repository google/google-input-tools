// Copyright 2013 The ChromeOS IME Authors. All Rights Reserved.
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

/**
 * @fileoverview VK layout definition for Vietnamese telex.
 */

var VI_TELEX_LAYOUT = {
  'id': 'vi_telex',
  'title': 'Ti\u1EBFng Vi\u1EC7t Telex',
  'mappings': {
    ',c': {
      '': '`1234567890-=' +
          'qwertyuiop[]\\' +
          'asdfghjkl;\'' +
          'zxcvbnm,./'
    },
    's,sc': {
      '': '~!@#$%^&*()_+' +
          'QWERTYUIOP{}|' +
          'ASDFGHJKL:"' +
          'ZXCVBNM<>?'
    },
    'l,cl': {
      '': '`1234567890-=' +
          'QWERTYUIOP[]\\' +
          'ASDFGHJKL;\'' +
          'ZXCVBNM,./'
    },
    'sl,scl': {
      '': '~!@#$%^&*()_+' +
          'qwertyuiop{}|' +
          'asdfghjkl:"' +
          'zxcvbnm<>?'
    }
  },
  'transform' : {
    // for Non-tonal diacritics
    'd\u001d?d': '\u0111',
    'a\u001d?a': '\u00e2',
    'e\u001d?e': '\u00ea',
    'o\u001d?o': '\u00f4',
    'a\u001d?w': '\u0103',
    'o\u001d?w': '\u01a1',
    'u\u001d?w': '\u01b0',
    'w': '\u01b0',
    'D\u001d?[Dd]': '\u0110',
    'A\u001d?[aA]': '\u00c2',
    'E\u001d?[eE]': '\u00ca',
    'O\u001d?[oO]': '\u00d4',
    'A\u001d?[wW]': '\u0102',
    'O\u001d?[wW]': '\u01a0',
    'U\u001d?[wW]': '\u01af',
    'W': '\u01af',
    // for repeat case
    '\u0111\u001dd': 'dd',
    '\u0110\u001dD': 'DD',
    '\u00e2\u001da': 'aa',
    '\u00ea\u001de': 'ee',
    '\u00f4\u001do': 'oo',
    '\u00c2\u001dA': 'AA',
    '\u00ca\u001dE': 'EE',
    '\u00d4\u001dO': 'OO',
    // For tone mark
    // 'y' is also a vowel
    // The SPACE is \u00a0 in the regex
    // Notice 'qu' and 'gi', which are considered as consonants
    // (u/i is not vowel here)
    // Examples:
    // gio + f -> giò
    // cio + f -> cìo
    // o + f -> ò
    // aoe + f -> aòe
    // aon + f -> aòn
    // quang + f -> quàng
    '(([qQ][uU]|[gG][iI])?[aeiouyAEIOUY])([aeiouyAEIOUY]?|[bcdghklmnpqtvBCDGHKLMNPQTV]+)\u001d?[fF]': // NOLINT
        '$1\u0300$3',
    '(([qQ][uU]|[gG][iI])?[aeiouyAEIOUY])([aeiouyAEIOUY]?|[bcdghklmnpqtvBCDGHKLMNPQTV]+)\u001d?[sS]': // NOLINT
        '$1\u0301$3',
    '(([qQ][uU]|[gG][iI])?[aeiouyAEIOUY])([aeiouyAEIOUY]?|[bcdghklmnpqtvBCDGHKLMNPQTV]+)\u001d?[rR]': // NOLINT
        '$1\u0309$3',
    '(([qQ][uU]|[gG][iI])?[aeiouyAEIOUY])([aeiouyAEIOUY]?|[bcdghklmnpqtvBCDGHKLMNPQTV]+)\u001d?[xX]': // NOLINT
        '$1\u0303$3',
    '(([qQ][uU]|[gG][iI])?[aeiouyAEIOUY])([aeiouyAEIOUY]?|[bcdghklmnpqtvBCDGHKLMNPQTV]+)\u001d?[jJ]': // NOLINT
        '$1\u0323$3',
    // ([ăâêôơưĂÂÊÔƠƯ])([a-zA-Z]*)[fF] : $1̀$2
    // Examples:
    // côad + f -> cồad
    // côâ + f -> côầ
    '([\u0103\u00e2\u00ea\u00f4\u01a1\u01b0\u0102\u00c2\u00ca\u00d4\u01a0\u01af])\u001d?([a-zA-Z]*)[fF]': // NOLINT
        '$1\u0300$2',
    '([\u0103\u00e2\u00ea\u00f4\u01a1\u01b0\u0102\u00c2\u00ca\u00d4\u01a0\u01af])\u001d?([a-zA-Z]*)[sS]': // NOLINT
        '$1\u0301$2',
    '([\u0103\u00e2\u00ea\u00f4\u01a1\u01b0\u0102\u00c2\u00ca\u00d4\u01a0\u01af])\u001d?([a-zA-Z]*)[rR]': // NOLINT
        '$1\u0309$2',
    '([\u0103\u00e2\u00ea\u00f4\u01a1\u01b0\u0102\u00c2\u00ca\u00d4\u01a0\u01af])\u001d?([a-zA-Z]*)[xX]': // NOLINT
        '$1\u0303$2',
    '([\u0103\u00e2\u00ea\u00f4\u01a1\u01b0\u0102\u00c2\u00ca\u00d4\u01a0\u01af])\u001d?([a-zA-Z]*)[jJ]': // NOLINT
        '$1\u0323$2',
    // double ff (or ss, rr, xx, jj) should cancel the combinging mark
    // Examples:
    // a + f -> à, + f -> af
    // coan + f -> coàn, + g -> coàng, + f -> coangf
    '([\u0300\u0301\u0309\u0303\u0323])([a-yA-Y\u001d]*)([zZ])' : '$2',
    '(\u0300)([a-zA-Z\u001d]*)([fF])' : '$2$3',
    '(\u0301)([a-zA-Z\u001d]*)([sS])' : '$2$3',
    '(\u0309)([a-zA-Z\u001d]*)([rR])' : '$2$3',
    '(\u0303)([a-zA-Z\u001d]*)([xX])' : '$2$3',
    '(\u0323)([a-zA-Z\u001d]*)([jJ])' : '$2$3',
    // auto-correction for tones
    // Examples:
    // cao + f -> cào, + e -> caòe, + n -> caoèn
    '(([qQ][uU]|[gG][iI])?[aeiouyAEIOUY]+)([\u0300\u0301\u0303\u0309\u0323\])\u001d?([aeiouyAEIOUY])\u001d?([a-eg-ik-qtuvyA-EG-IK-QTUVY])': // NOLINT
        '$1$4$3$5',
    // ([ ̀ ́ ̃ ̉ ̣])([ăâêôơưĂÂÊÔƠƯ])([a-zA-Z]) : $1$3$2$4
    // Examples:
    // co + f -> cò, + â -> còâ, + n -> coần
    '([\u0300\u0301\u0303\u0309\u0323])([\u0103\u00e2\u00ea\u00f4\u01a1\u01b0\u0102\u00c2\u00ca\u00d4\u01a0\u01af])\u001d?([a-zA-Z])': // NOLINT
        '$2$1$3'
  }
};

// Loads the layout and inform the keyboard to switch layout if necessary.
cros_vk_loadme(VI_TELEX_LAYOUT);
