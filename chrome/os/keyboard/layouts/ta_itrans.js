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
 * @fileoverview VK layout definition for Tamil itrans.
 */

var TA_ITRANS_LAYOUT = {
  'id': 'ta_itrans',
  'title': '\u0BA4\u0BAE\u0BBF\u0BB4\u0BCD (ITRANS)',
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
    'k': 'க்',
    'g': 'க்',
    '~N': 'ங்',
    'N\\^': 'ங்',
    'ch': 'ச்',
    'j': 'ஜ்',
    '~n': 'ஞ்',
    'JN': 'ஞ்',
    'T': 'ட்',
    'Th': 'ட்',
    'N': 'ண்',
    't': 'த்',
    'th': 'த்',
    'n': 'ந்',
    '\\^n': 'ன்',
    'nh': 'ன',
    'p': 'ப்',
    'b': 'ப்',
    'm': 'ம்',
    'y': 'ய்',
    'r': 'ர்',
    'R': 'ற்',
    'rh': 'ற',
    'l': 'ல்',
    'L': 'ள்',
    'ld': 'ள்',
    'J': 'ழ்',
    'z': 'ழ்',
    'v': 'வ்',
    'w': 'வ்',
    'Sh': 'ஷ்',
    'shh': 'ஷ',
    's': 'ஸ்',
    'h': 'ஹ்',
    'GY': 'ஜ்ஞ',
    'dny': 'ஜ்ஞ',
    'x': 'க்ஷ்',
    'ksh': 'க்ஷ்',
    'a': 'அ',
    'aa': 'ஆ',
    'A': 'ஆ',
    'i': 'இ',
    'ii': 'ஈ',
    'I': 'ஈ',
    'u': 'உ',
    'uu': 'ஊ',
    'U': 'ஊ',
    'e': 'எ',
    'E': 'ஏ',
    'ee': 'ஏ',
    'ai': 'ஐ',
    'o': 'ஒ',
    'O': 'ஓ',
    'oo': 'ஓ',
    'au': 'ஔ',
    '\\.n': 'ஂ',
    'M': 'ஂ',
    'q': 'ஃ',
    'H': 'ஃ',
    '\\.h': '்',
    '0': '௦',
    '1': '௧',
    '2': '௨',
    '3': '௩',
    '4': '௪',
    '5': '௫',
    '6': '௬',
    '7': '௭',
    '8': '௮',
    '9': '௯',
    '#': '்',
    '\\$': 'ர',
    '\\^': 'த்'
  },
  'historyPruneRegex': '[\\^lrshkdnJNtTaeiouAEIOU]|sh|ks|dn'
};

// Loads the layout and inform the keyboard to switch layout if necessary.
cros_vk_loadme(TA_ITRANS_LAYOUT);
