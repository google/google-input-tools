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
// Copyright 2013 The ChromeOS VK Authors. All Rights Reserved.
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
 * @fileoverview VK layout definition for Vietnamese viqr.
 */

var VI_VIQR_LAYOUT = {
  'id': 'vi_viqr',
  'title': 'Ti\u1EBFng Vi\u1EC7t VIQR',
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
    'dd': '\u0111',
    'D[dD]': '\u0110',
    'a\\(': '\u0103',
    'a\\^': '\u00e2',
    'e\\^': '\u00ea',
    'o\\^': '\u00f4',
    'o\\+': '\u01a1',
    'u\\+': '\u01b0',
    'A\\(': '\u0102',
    'A\\^': '\u00c2',
    'E\\^': '\u00ca',
    'O\\^': '\u00d4',
    'O\\+': '\u01a0',
    'U\\+': '\u01af',
    '\\\\\\(': '(',
    '\\\\\\^': '^',
    '\\\\\\+': '+',
    '\\\\\\`': '`',
    '\\\\\\\'': '\'',
    '\\\\\\?': '?',
    '\\\\\\~': '~',
    '\\\\\\\.': '.',
    // for tone mark
    '\\`' : '\u0300',
    '\\\'' : '\u0301',
    '\\?' : '\u0309',
    '\\~' : '\u0303',
    '\\\.' : '\u0323'
  }
};

// Load the layout and inform the keyboard to switch layout if necessary. cibu
cros_vk_loadme(VI_VIQR_LAYOUT);
