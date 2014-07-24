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
 * @fileoverview VK layout definition for Telugu phonetic.
 */

var TE_PHONE_LAYOUT = {
  'id': 'te_phone',
  'title': '\u0C24\u0C46\u0C32\u0C41\u0C17\u0C41 (\u0C2B' +
      '\u0C4B\u0C28\u0C46\u0C1F\u0C3F\u0C15\u0C4D)',
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
    '([క-హ])?u': '$1ౌ',
    'ొ\u001D?o': 'ో',
    '్\u001D?O': 'ో',
    '్\u001D?o': 'ొ',
    '([క-హ])?i': '$1ై',
    'ె\u001D?e': 'ీ',
    '్\u001D?E': 'ే',
    '్\u001D?e': 'ె',
    '్ళ్ళ్\u001D?I': 'ౣ',
    '్ళ్\u001D?\\^I': 'ౣ',
    '్ళ్ళ్\u001D?i': 'ౢ',
    '్ళ్\u001D?\\^i': 'ౢ',
    'ృ\u001D?I': 'ౄ',
    'ృ\u001D?\\^I': 'ౄ',
    'ృ\u001D?R': 'ౄ',
    'ృ\u001D?\\^i': 'ృ',
    'ౄ\u001D?i': 'ృ',
    '్\u001D?R': 'ృ',
    'ు\u001D?u': 'ూ',
    '్\u001D?U': 'ూ',
    '్\u001D?u': 'ు',
    'ి\u001D?i': 'ీ',
    '్\u001D?I': 'ీ',
    '్\u001D?i': 'ి',
    '([క-హ])?a': '$1ా',
    '్\u001D?A': 'ా',
    '్\u001D?a': '',
    '\\.a': 'ఽ',
    '\\.m': 'ం',
    '\\.z': '',
    '\\.N': 'ఁ',
    '్\u001D?\\.h': '్\u200C',
    '\\.h': '్\u200C',
    '@M': 'ం',
    '@h': 'ః',
    '@2': 'ఽ',
    'M': 'ం',
    'H': 'ః',
    'క్\u001D?H': 'ೱ',
    'ప్\u001D?H': 'ೲ',
    // Vowels in reverse.
    'అ\u001D?u': 'ఔ',
    'ఒ\u001D?o': 'ఓ',
    'O': 'ఓ',
    'o': 'ఒ',
    'అ\u001D?i': 'ఐ',
    'ఎ\u001D?e': 'ఈ',
    'E': 'ఏ',
    'e': 'ఎ',
    'ళ్ళ్\u001D?I': 'ౡ',
    'ళ్\u001D?\\^I': 'ౡ',
    'ళ్ళ్\u001D?i': 'ఌ',
    'ళ్\u001D?i': 'ఌ',
    'ౠ\u001D?I': 'ౠ',
    'ఋ\u001D?R': 'ౠ',
    'R': 'ఋ',
    'ఉ\u001D?u': 'ఊ',
    'U': 'ఊ',
    'u': 'ఉ',
    'ఇ\u001D?i': 'ఈ',
    'I': 'ఈ',
    'i': 'ఇ',
    'A': 'ఆ',
    'అ\u001D?a': 'ఆ',
    'a': 'అ',
    // Major conjoint consonants.
    'క్ష్\u001D?h': 'క్ష్',
    'గ్\u001D?Y': 'జ్ఞ్',
    'జ్\u001D?n': 'జ్ఞ్',
    'క్స్\u001D?h': 'క్ష్',
    'క్\u001D?S': 'క్ష్',
    'x': 'క్ష్',
    // Ungrouped consonants.
    // Reverse order - except for sibilants.
    'h': 'హ్',
    'శ్\u001D?h': 'ష్',
    'S': 'శ్',
    'z': 'శ్',
    'స్\u001D?h': 'శ్',
    's': 'స్',
    'v': 'వ్',
    'w': 'వ్',
    'L': 'ళ్',
    '\\.L': 'ళ్',
    'l': 'ల్',
    'r': 'ర్',
    // Maps such as the following are placeholders for when nukta gets added
    // to telugu unicode.
    '\\.r': 'ఱ్',
    'y': 'య్',
    // Grouped consonants in reverse (by group).
    // ka-varga
    '~N': 'ఙ్',
    'గ్\u001D?h': 'ఘ్',
    'G': 'ఘ్',
    '\\.g': 'గ్',
    'g': 'గ్',
    '\\.K': 'ఖ్',
    'K': 'ఖ్',
    'క్\u001D?h': 'ఖ్',
    'q': 'క్',
    'k': 'క్',
    // cha-varga
    '~n': 'ఞ్',
    'జ్\u001D?h': 'ఝ్',
    'J': 'ఝ్',
    '\\.j': 'జ్',
    'j': 'జ్',
    'చ్\u001D?h': 'ఛ్', //chh
    'Ch': 'ఛ్',
    'C': 'ఛ్',
    'ch': 'చ్',
    'c': 'చ్',
    // Ta-varga
    'N': 'ణ్',
    'డ్\u001D?h': 'ఢ్',
    '\\.D': 'డ్',
    'D': 'డ్',
    'ట్\u001D?h': 'ఠ్',
    'T': 'ట్',
    // da-varga
    'n': 'న్',
    'ద్\u001D?h': 'ధ్',
    'd': 'ద్',
    'త్\u001D?h': 'థ్',
    't': 'త్',
    // pa-varga
    'm': 'మ్',
    'బ్\u001D?h': 'భ్',
    'B': 'భ్',
    'b': 'బ్',
    'f': 'ఫ్',
    'ప్\u001D?h': 'ఫ్',
    'P': 'ఫ్',
    'p': 'ప్',
    //Shortcuts for ంక్ ంఖ్ ంగ్ and the like.
    // Rules like the following are not required:
    // 'ంగ్\u001D?h': 'ంఘ్'
    //because 'గ్\u001D?h': 'ఘ్'
    // k-varga
    'న్\u001D?G': 'ంఘ్',
    'న్\u001D?g': 'ంగ్',
    'న్\u001D?K': 'ంఖ్',
    'న్\u001D?k': 'ంక్',
    // cha-varga
    'న్\u001D?J': 'ంఝ్',
    'న్\u001D?j': 'ంజ్',
    'న్\u001D?Ch': 'ంఛ్',
    'న్\u001D?ch': 'ంచ్',
    'న్\u001D?C': 'ంఛ్',
    'న్\u001D?c': 'ంచ్',
    // Ta-varga
    'న్\u001D?D': 'ండ్',
    'న్\u001D?T': 'ంట్',
    //Shortcuts for ఙ్క్ ఞ్జ్ ఙ్గ్ and the like.
    // Rules like the following are not required:
    //'ఙ్క్\u001D?h': 'ఙ్ఖ్',
    //because 'క్\u001D?h': 'ఖ్'
    // k-varga
    'న్న్\u001D?g': 'ఙ్గ్',
    'న్న్\u001D?k': 'ఙ్క్',
    // cha-varga
    'న్న్\u001D?j': 'ఞ్జ్',
    'న్న్\u001D?Ch': 'ఞ్ఛ్',
    'న్న్\u001D?ch': 'ఞ్చ్',
    'న్న్\u001D?C': 'ఞ్ఛ్',
    'న్న్\u001D?c': 'ఞ్చ్',
    // Ta-varga
    'న్న్\u001D?D': 'ణ్డ్',
    'న్న్\u001D?T': 'ణ్ట్',
    // daNDa-s
    '\\|': '।',
    '।\u001D?\\|': '॥'
  },
  // If there are cases where, without looking at history of the chars pressed,
  // you cannot deduce what the output would be, then the following is required.
  'historyPruneRegex': 'C|c'
};

// Load the layout and inform the keyboard to switch layout if necessary.
cros_vk_loadme(TE_PHONE_LAYOUT);
