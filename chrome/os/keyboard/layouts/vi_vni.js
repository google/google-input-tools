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
// Copyright 2013 Google Inc.
// All Rights Reserved.
// Reference: http://en.wikipedia.org/wiki/VNI

var VI_VNI_LAYOUT = {
  'id': 'vi_vni',
  'title': 'Ti\u1EBFng Vi\u1EC7t VNI',
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
    'd\u001d?9': '\u0111',
    'D\u001d?9': '\u0110',
    '(.*)a\u001d?6': '$1\u00e2',
    '(.*)e\u001d?6': '$1\u00ea',
    '(.*)o\u001d?6': '$1\u00f4',
    '(.*)a\u001d?8': '$1\u0103',
    '(.*)o\u001d?7': '$1\u01a1',
    '(.*)u\u001d?7': '$1\u01b0',
    '(.*)A\u001d?6': '$1\u00c2',
    '(.*)E\u001d?6': '$1\u00ca',
    '(.*)O\u001d?6': '$1\u00d4',
    '(.*)A\u001d?8': '$1\u0102',
    '(.*)O\u001d?7': '$1\u01a0',
    '(.*)U\u001d?7': '$1\u01af',
    // For tone mark
    // 'y' is also a vowel
    // The SPACE is \u00a0 in the regex
    // Notice 'qu' and 'gi', which are considered as consonants
    // (u/i is not vowel here)
    // Examples:
    // gio + 2 -> giò
    // cio + 2 -> cìo
    // o + 2 -> ò
    // aoe + 2 -> aòe
    // aon + 2 -> aòn
    // quang + 2 -> quàng
    '(([qQ][uU]|[gG][iI])?[aeiouyAEIOUY])([aeiouyAEIOUY]?|[bcdghklmnpqtvBCDGHKLMNPQTV]+)\u001d?2': // NOLINT
        '$1\u0300$3',
    '(([qQ][uU]|[gG][iI])?[aeiouyAEIOUY])([aeiouyAEIOUY]?|[bcdghklmnpqtvBCDGHKLMNPQTV]+)\u001d?1': // NOLINT
        '$1\u0301$3',
    '(([qQ][uU]|[gG][iI])?[aeiouyAEIOUY])([aeiouyAEIOUY]?|[bcdghklmnpqtvBCDGHKLMNPQTV]+)\u001d?3': // NOLINT
        '$1\u0309$3',
    '(([qQ][uU]|[gG][iI])?[aeiouyAEIOUY])([aeiouyAEIOUY]?|[bcdghklmnpqtvBCDGHKLMNPQTV]+)\u001d?4': // NOLINT
        '$1\u0303$3',
    '(([qQ][uU]|[gG][iI])?[aeiouyAEIOUY])([aeiouyAEIOUY]?|[bcdghklmnpqtvBCDGHKLMNPQTV]+)\u001d?5': // NOLINT
        '$1\u0323$3',
    // ([ăâêôơưĂÂÊÔƠƯ])([a-zA-Z]*)2 : $1̀$2
    // Examples:
    // côad + 2 -> cồad
    // côâ + 2 -> côầ
    '([\u0103\u00e2\u00ea\u00f4\u01a1\u01b0\u0102\u00c2\u00ca\u00d4\u01a0\u01af])\u001d?([a-zA-Z]*)2': // NOLINT
        '$1\u0300$2',
    '([\u0103\u00e2\u00ea\u00f4\u01a1\u01b0\u0102\u00c2\u00ca\u00d4\u01a0\u01af])\u001d?([a-zA-Z]*)1': // NOLINT
        '$1\u0301$2',
    '([\u0103\u00e2\u00ea\u00f4\u01a1\u01b0\u0102\u00c2\u00ca\u00d4\u01a0\u01af])\u001d?([a-zA-Z]*)3': // NOLINT
        '$1\u0309$2',
    '([\u0103\u00e2\u00ea\u00f4\u01a1\u01b0\u0102\u00c2\u00ca\u00d4\u01a0\u01af])\u001d?([a-zA-Z]*)4': // NOLINT
        '$1\u0303$2',
    '([\u0103\u00e2\u00ea\u00f4\u01a1\u01b0\u0102\u00c2\u00ca\u00d4\u01a0\u01af])\u001d?([a-zA-Z]*)5': // NOLINT
        '$1\u0323$2',
    // auto-correction for tones
    // Examples:
    // cao + 2 -> cào, + e -> caòe, + n -> caoèn
    '(([qQ][uU]|[gG][iI])?[aeiouyAEIOUY]+)([\u0300\u0301\u0303\u0309\u0323\])\u001d?([aeiouyAEIOUY])\u001d?([a-eg-ik-qtuvyA-EG-IK-QTUVY])': // NOLINT
        '$1$4$3$5',
    // ([ ̀ ́ ̃ ̉ ̣])([ăâêôơưĂÂÊÔƠƯ])([a-zA-Z]) : $1$3$2$4
    // Examples:
    // co + 2 -> cò, + â -> còâ, + n -> coần
    '([\u0300\u0301\u0303\u0309\u0323])([\u0103\u00e2\u00ea\u00f4\u01a1\u01b0\u0102\u00c2\u00ca\u00d4\u01a0\u01af])\u001d?([a-zA-Z])': // NOLINT
        '$2$1$3'
  }
};

// Loads the layout and inform the keyboard to switch layout if necessary.
cros_vk_loadme(VI_VNI_LAYOUT);
