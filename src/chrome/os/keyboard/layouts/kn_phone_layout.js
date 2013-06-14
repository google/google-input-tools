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
 * @fileoverview VK layout definition for Kannada phonetic.
 */

var KN_PHONE_LAYOUT = {
  'id': 'kn_phone',
  'title': '\u0C95\u0CA8\u0CCD\u0CA8\u0CA1 (\u0CAB\u0CCA\u0CA8' +
      '\u0CC6\u0C9F\u0CBF\u0C95\u0CCD)',
  'mappings': {
    '': {
      '': '`1234567890-=' +
          'qwertyuiop[]\\' +
          'asdfghjkl;\'' +
          'zxcvbnm,./'
    },
    's': {
      '': '~!@#$%^&*()_+' +
          'QWERTYUIOP{}|' +
          'ASDFGHJKL:"' +
          'ZXCVBNM<>?'
    },
    'l': {
      '': '`1234567890-=' +
          'QWERTYUIOP[]\\' +
          'ASDFGHJKL;\'' +
          'ZXCVBNM,./'
    },
    'sl': {
      '': '~!@#$%^&*()_+' +
          'qwertyuiop{}|' +
          'asdfghjkl:"' +
          'zxcvbnm<>?'
    }
  },
  'transform': {
    '0': '೦',
    '1': '೧',
    '2': '೨',
    '3': '೩',
    '4': '೪',
    '5': '೫',
    '6': '೬',
    '7': '೭',
    '8': '೮',
    '9': '೯',
    '([ಕ-ಹೞ])?u': '$1ೌ',
    'ೊ\u001D?o': 'ೋ',
    '್\u001D?O': 'ೋ',
    '್\u001D?o': 'ೊ',
    '([ಕ-ಹೞ])?i': '$1ೈ',
    'ೆ\u001D?e': 'ೇ',
    '್\u001D?E': 'ೇ',
    '್\u001D?e': 'ೆ',
    '್ಳ್ಳ್\u001D?I': 'ೣ',
    '್ಳ್\u001D?\\^I': 'ೣ',
    '್ಳ್ಳ್\u001D?i': 'ೢ',
    '್ಳ್\u001D?\\^i': 'ೢ',
    'ೃ\u001D?I': 'ೄ',
    'ೃ\u001D?\\^I': 'ೄ',
    'ೃ\u001D?R': 'ೄ',
    'ೃ\u001D?\\^i': 'ೃ',
    'ೄ\u001D?i': 'ೃ',
    '್\u001D?R': 'ೃ',
    'ು\u001D?u': 'ೂ',
    '್\u001D?U': 'ೂ',
    '್\u001D?u': 'ು',
    'ಿ\u001D?i': 'ೀ',
    '್\u001D?I': 'ೀ',
    '್\u001D?i': 'ಿ',
    '([ಕ-ಹೞ])?a': '$1ಾ',
    '್\u001D?A': 'ಾ',
    '್\u001D?a': '',
    '\\.a': 'ಽ',
    '\\.m': 'ಂ',
    '\\.z': '಼',
    '\\.N': 'ँ',
    '್\u001D?\\.h': '್\u200C',
    '\\.h': '್\u200C',
    'M': 'ಂ',
    'H': 'ಃ',
    'ಕ್\u001D?H': 'ೱ',
    'ಪ್\u001D?H': 'ೲ',
    // Vowels in reverse.
    'ಅ\u001D?u': 'ಔ',
    'ಒ\u001D?o': 'ಓ',
    'O': 'ಓ',
    'o': 'ಒ',
    'ಅ\u001D?i': 'ಐ',
    'ಎ\u001D?e': 'ಏ',
    'E': 'ಏ',
    'e': 'ಎ',
    'ಳ್ಳ್\u001D?I': 'ೡ',
    'ಳ್\u001D?\\^I': 'ೡ',
    'ಳ್ಳ್\u001D?i': 'ಌ',
    'ಳ್\u001D?^i': 'ಌ',
    'ೠ\u001D?I': 'ೠ',
    'ೠ\u001D?^I': 'ೠ',
    'ಋ\u001D?R': 'ೠ',
    'R': 'ಋ',
    'ಉ\u001D?u': 'ಊ',
    'U': 'ಊ',
    'u': 'ಉ',
    'ಇ\u001D?i': 'ಈ',
    'I': 'ಈ',
    'i': 'ಇ',
    'A': 'ಆ',
    'ಅ\u001D?a': 'ಆ',
    'a': 'ಅ',
    // Major conjoint consonants.
    'ಕ್ಷ್\u001D?h': 'ಕ್ಷ್',
    'ಗ್\u001D?Y': 'ಜ್ಞ್',
    'ಜ್\u001D?n': 'ಜ್ಞ್',
    'ಕ್\u001D?S': 'ಕ್ಷ್',
    'ಕ್ಸ್\u001D?h': 'ಕ್ಷ್',
    'x': 'ಕ್ಷ್',
    // Ungrouped consonants.
    // Reverse order - except for sibilants.
    'h': 'ಹ್',
    'ಷ್\u001D?h': 'ಷ್',
    'S': 'ಷ್',
    'z': 'ಶ್',
    'ಸ್\u001D?h': 'ಶ್',
    's': 'ಸ್',
    'v': 'ವ್',
    'w': 'ವ್',
    'L': 'ಳ್',
    '\\.L': 'ೞ್',
    'l': 'ಲ್',
    'r': 'ರ್',
    '\\.r': 'ಱ್',
    'y': 'ಯ್',
    // Grouped consonants in reverse (by group).
    // ka-varga
    '~N': 'ಙ್',
    'ಗ್\u001D?h': 'ಘ್',
    'G': 'ಘ್',
    '\\.g': 'ಗ಼್',
    'g': 'ಗ್',
    '\\.K': 'ಖ಼್',
    'K': 'ಖ್',
    'ಕ್\u001D?h': 'ಖ್',
    'q': 'ಕ಼್',
    'k': 'ಕ್',
    // cha-varga
    '~n': 'ಞ್',
    'ಜ್\u001D?h': 'ಝ್',
    'J': 'ಝ್',
    '\\.j': 'ಜ಼್',
    'j': 'ಜ್',
    'ಚ್\u001D?h': 'ಛ್', //chh
    'Ch': 'ಛ್',
    'ch': 'ಚ್',
    'C': 'ಛ್',
    'c': 'ಚ್',
    // Ta-varga
    'N': 'ಣ್',
    'ಡ಼್\u001D?h': 'ಢ಼್',
    '\\.D': 'ಡ಼್',
    'ಡ್\u001D?h': 'ಢ್',
    'D': 'ಡ್',
    'ಟ್\u001D?h': 'ಠ್',
    'T': 'ಟ್',
    // da-varga
    'n': 'ನ್',
    'ದ್\u001D?h': 'ಧ್',
    'd': 'ದ್',
    'ತ್\u001D?h': 'ಥ್',
    't': 'ತ್',
    // pa-varga
    'm': 'ಮ್',
    'ಬ್\u001D?h': 'ಭ್',
    'B': 'ಭ್',
    'b': 'ಬ್',
    'f': 'ಫ಼್',
    'ಪ್\u001D?h': 'ಫ್',
    'P': 'ಫ್',
    'p': 'ಪ್',
    //Shortcuts for ಂಕ್ ಂಖ್ ಂಗ್ and the like.
    // Rules like the following are not required:
    // 'ಂಗ್\u001D?h': 'ಂಘ್'
    //because 'ಗ್\u001D?h': 'ಘ್'
    // k-varga
    'ನ್\u001D?G': 'ಂಘ್',
    'ನ್\u001D?g': 'ಂಗ್',
    'ನ್\u001D?K': 'ಂಖ್',
    'ನ್\u001D?k': 'ಂಕ್',
    // cha-varga
    'ನ್\u001D?J': 'ಂಝ್',
    'ನ್\u001D?j': 'ಂಜ್',
    'ನ್\u001D?Ch': 'ಂಛ್',
    'ನ್\u001D?ch': 'ಂಚ್',
    'ನ್\u001D?C': 'ಂಛ್',
    'ನ್\u001D?c': 'ಂಚ್',
    // Ta-varga
    'ನ್\u001D?D': 'ಂಡ್',
    'ನ್\u001D?T': 'ಂಟ್',
    //Shortcuts for ಙ್ಕ್ ಞ್ಜ್ ಙ್ಗ್ and the like.
    // Rules like the following are not required:
    //'ಙ್ಕ್\u001D?h': 'ಙ್ಖ್',
    //because 'ಕ್\u001D?h': 'ಖ್'
    // k-varga
    'ನ್ನ್\u001D?g': 'ಙ್ಗ್',
    'ನ್ನ್\u001D?k': 'ಙ್ಕ್',
    // cha-varga
    'ನ್ನ್\u001D?j': 'ಞ್ಜ್',
    'ನ್ನ್\u001D?Ch': 'ಞ್ಛ್',
    'ನ್ನ್\u001D?ch': 'ಞ್ಚ್',
    'ನ್ನ್\u001D?C': 'ಞ್ಛ್',
    'ನ್ನ್\u001D?c': 'ಞ್ಚ್',
    // Ta-varga
    'ನ್ನ್\u001D?D': 'ಣ್ಡ್',
    'ನ್ನ್\u001D?T': 'ಣ್ಟ್',
    // daNDa-s
    '\\|': '।',
    '।\u001D?\\|': '॥'
  },
  // If there are cases where, without looking at history of the chars pressed,
  // you cannot deduce what the output would be, then the following is required.
  'historyPruneRegex': 'C|c'
};

// Load the layout and inform the keyboard to switch layout if necessary.
cros_vk_loadme(KN_PHONE_LAYOUT);
