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
 * @fileoverview VK layout definition for Devanagari phonetic.
 */

var DEVA_PHONE_LAYOUT = {
  'id': 'deva_phone',
  'title': '\u0926\u0947\u0935\u0928\u093E\u0917\u0930\u0940 (\u092B' +
      '\u094B\u0928\u0947\u091F\u093F\u0915)',
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
      '': '~!@' +
          'qwertyuiop{}|' +
          'asdfghjkl:"' +
          'zxcvbnm<>?'
    }
  },
  'transform': {
    '0': '०',
    '1': '१',
    '2': '२',
    '3': '३',
    '4': '४',
    '5': '५',
    '6': '६',
    '7': '७',
    '8': '८',
    '9': '९',
    //vowels for use with consonents. In reverse order.
    '([क-हक़-य़])\u001D?au': '$1ौ',
    'ो\u001D?a': 'ॉ',
    'ो\u001D?\\.c': 'ॉ',
    'ो\u001D?o': 'ॊ',
    '([क-हक़-य़])\u001D?O': '$1ो',
    '([क-हक़-य़])\u001D?o': '$1ो',
    '([क-हक़-य़])\u001D?ai': '$1ै',
    'े\u001D?e': 'ॆ',
    '([क-हक़-य़])\u001D?E': '$1े',
    'े\u001D?\\.c': 'ॅ',
    '([क-हक़-य़])\u001D?e': '$1े',
    '([क-हक़-य़])्ळ्ळ\u001D?I': '$1ॣ',
    '([क-हक़-य़])्ळ\u001D?\\^I': '$1ॣ',
    '([क-हक़-य़])्ळ्ळ\u001D?i': '$1ॢ',
    '([क-हक़-य़])्ळ\u001D?\\^i': '$1ॢ',
    'ॄ\u001D?I': 'ॄ',
    'ृ\u001D?\\^I': 'ॄ',
    'ृ\u001D?R': 'ॄ',
    'ृ\u001D?\\^i': 'ृ',
    'ॄ\u001D?i': 'ृ',
    '([क-हक़-य़])\u001D?R': '$1ृ',
    'ु\u001D?u': 'ू',
    '([क-हक़-य़])\u001D?U': '$1ू',
    '([क-हक़-य़])\u001D?u': '$1ु',
    'ि\u001D?i': 'ी',
    '([क-हक़-य़])\u001D?I': '$1ी',
    '([क-हक़-य़])\u001D?i': '$1ि',
    '([क-हक़-य़])\u001D?A': '$1ा',
    '([क-हक़-य़])\u001D?aa': '$1ा',
    '([क-हक़-य़])\u001D?a': '$1',
    '\\.a': 'ऽ',
    '\\.n': 'ं',
    '\\.m': 'ं',
    '\\.N': 'ँ',
    '\\.h': '्\u200C',
    'M': 'ं',
    'H': 'ः',
    // Special combinations.
    'आऊ\u001D?M': 'ॐ',
    'ओ\u001D?M': 'ॐ',
    // Vowels in reverse.
    'अ\u001D?u': 'औ',
    'ओ\u001D?\\.c': 'ऑ',
    'ओ\u001D?o': 'ऒ',
    'O': 'ओ',
    'o': 'ओ',
    'अ\u001D?i': 'ऐ',
    'ए\u001D?e': 'ऎ',
    'E': 'ए',
    'ए\u001D?\\.c': 'ऍ',
    'e': 'ए',
    'ळ्ळ\u001D?I': 'ॡ',
    'ळ\u001D?\\^I': 'ॡ',
    'ळ्ळ\u001D?i': 'ऌ',
    'ळ\u001D?\\^i': 'ऌ',
    'ॠ\u001D?I': 'ॠ',
    'ऋ\u001D?\\^I': 'ॠ',
    'ऋ\u001D?R': 'ॠ',
    'ॠ\u001D?i': 'ऋ',
    'ऋ\u001D?\\^i': 'ऋ',
    'R': 'ऋ',
    'उ\u001D?u': 'ऊ',
    'U': 'ऊ',
    'u': 'उ',
    'इ\u001D?i': 'ई',
    'I': 'ई',
    'i': 'इ',
    'A': 'आ',
    'अ\u001D?a': 'आ',
    'a': 'अ',
    // Major conjoint consonants.
    //'ष\u001D?h': 'क्ष',
    'ग\u001D?Y': 'ज्ञ',
    'ज\u001D?n': 'ज्ञ',
    'क\u001D?S': 'क्ष',
    'क्स्\u001D?h': 'क्ष',
    'x': 'क्ष',
    // Ungrouped consonants.
    // Reverse order - except for sibilants.
    'h': 'ह',
    'ष\u001D?h': 'ष',
    'S': 'ष',
    'z': 'श',
    'स\u001D?h': 'श',
    's': 'स',
    'v': 'व',
    'w': 'व',
    'L': 'ळ',
    '\\.L': 'ऴ',
    'l': 'ल',
    'r': 'र',
    '\\.r': 'ऱ',
    'Y': 'य़',
    '\\.y': 'य़',
    'y': 'य',
    // Grouped consonants in reverse (by group).
    // ka-varga
    '~N': 'ङ',
    'ग\u001D?h': 'घ',
    'G': 'घ',
    '\\.g': 'ग़',
    'g': 'ग',
    'K': 'ख',
    '\\.K': 'ख़',
    'क़\u001D?h': 'ख़',
    'क\u001D?h': 'ख',
    'q': 'क़',
    '\\.k': 'क़',
    'k': 'क',
    // cha-varga
    '~n': 'ञ',
    'ज\u001D?h': 'झ',
    'J': 'झ',
    '\\.j': 'ज़',
    'j': 'ज',
    'chh': 'छ',
    'Ch': 'छ',
    'ch': 'च',
    'C': 'छ',
    'c': 'च',
    // Ta-varga
    'N': 'ण',
    'ड़\u001D?h': 'ढ़',
    '\\.D': 'ड़',
    'ड\u001D?h': 'ढ',
    'D': 'ड',
    'ट\u001D?h': 'ठ',
    'T': 'ट',
    // da-varga
    'n': 'न',
    'द\u001D?h': 'ध',
    'd': 'द',
    'त\u001D?h': 'थ',
    't': 'त',
    // pa-varga
    'm': 'म',
    'ब\u001D?h': 'भ',
    'B': 'भ',
    'b': 'ब',
    'f': 'फ़',
    '\\.P': 'फ़',
    '\\.प\u001D?h': 'फ़',
    'प\u001D?h': 'फ',
    'P': 'फ',
    'p': 'प',
    // samyuktAkShara-s
    // Rules such as the below are not necessary.
    //'([क-हक़-य़])\u001D?द\u001D?h': '$1्ध',
    // Major conjoint consonants.
    '([क-हक़-य़])\u001D?x': '$1्क्ष',
    // Ungrouped consonants.
    // Reverse order - except for sibilants.
    '([क-हक़-य़])\u001D?h': '$1्ह',
    '([क-हक़-य़])\u001D?S': '$1्ष',
    '([क-हक़-य़])\u001D?z': '$1्श',
    '([क-हक़-य़])\u001D?s': '$1्स',
    '([क-हक़-य़])\u001D?v': '$1्व',
    '([क-हक़-य़])\u001D?w': '$1्व',
    '([क-हक़-य़])\u001D?L': '$1्ळ',
    '([क-हक़-य़])\u001D?\\.L': '$1्ऴ',
    '([क-हक़-य़])\u001D?l': '$1्ल',
    '([क-हक़-य़])\u001D?r': '$1्र',
    '([क-हक़-य़])\u001D?\\.r': '$1्ऱ',
    '([क-हक़-य़])\u001D?Y': '$1्य़',
    '([क-हक़-य़])\u001D?\\.y': '$1्य़',
    '([क-हक़-य़])\u001D?y': '$1्य',
    // Grouped consonants in reverse (by group).
    // Omitting न in the ([क-धऩ-हक़-य़]) patterns below.
    // ka-varga
    '([क-धऩ-हक़-य़])\u001D?~N': '$1्ङ',
    '([क-धऩ-हक़-य़])\u001D?G': '$1्घ',
    '([क-धऩ-हक़-य़])\u001D?\\.g': '$1्ग़',
    '([क-धऩ-हक़-य़])\u001D?g': '$1्ग',
    '([क-धऩ-हक़-य़])\u001D?K': '$1्ख',
    '([क-धऩ-हक़-य़])\u001D?\\.K': '$1्ख़',
    '([क-धऩ-हक़-य़])\u001D?q': '$1्क़',
    '([क-धऩ-हक़-य़])\u001D?\\.k': '$1्क़',
    '([क-धऩ-हक़-य़])\u001D?k': '$1्क',
    // cha-varga
    '([क-धऩ-हक़-य़])\u001D?~n': '$1्ञ',
    '([क-धऩ-हक़-य़])\u001D?J': '$1्झ',
    '([क-धऩ-हक़-य़])\u001D?\\.j': '$1्ज़',
    '([क-धऩ-हक़-य़])\u001D?j': '$1्ज',
    '([क-धऩ-हक़-य़])\u001D?chh': '$1्छ',
    '([क-धऩ-हक़-य़])\u001D?Ch': '$1्छ',
    '([क-धऩ-हक़-य़])\u001D?ch': '$1्च',
    '([क-धऩ-हक़-य़])\u001D?C': '$1्छ',
    '([क-धऩ-हक़-य़])\u001D?c': '$1्च',
    // Ta-varga
    '([क-धऩ-हक़-य़])\u001D?N': '$1्ण',
    '([क-धऩ-हक़-य़])\u001D?\\.D': '$1्ड़',
    '([क-धऩ-हक़-य़])\u001D?D': '$1्ड',
    '([क-धऩ-हक़-य़])\u001D?T': '$1्ट',
    // ta-varga
    //Omit ज below.
    '([क-छझ-हक़-य़])\u001D?n': '$1्न',
    '([क-हक़-य़])\u001D?d': '$1्द',
    '([क-हक़-य़])\u001D?t': '$1्त',
    // pa-varga
    '([क-हक़-य़])\u001D?m': '$1्म',
    '([क-हक़-य़])\u001D?B': '$1्भ',
    '([क-हक़-य़])\u001D?b': '$1्ब',
    '([क-हक़-य़])\u001D?f': '$1्फ़',
    '([क-हक़-य़])\u001D?\\.P': '$1्फ़',
    '([क-हक़-य़])\u001D?P': '$1्फ',
    '([क-हक़-य़])\u001D?p': '$1्प',


    //Shortcuts for ङ्क ङ्ग and the like.
    // ka-varga
    // Not using rules like: nk: ङ्क for ka-varga because:
    // Mappings such as
    //'नग\u001D?h': 'ङ्घ' are not required because
    // 'ग\u001D?h': 'घ' will take care of it.
    'n\\.g': 'ङ्ग़',
    'ng': 'ङ्ग',
    'nG': 'ङ्घ',
    'n\\.k': 'ङ्क़',
    'nq': 'ङ्क़',
    'nK': 'ङ्ख',
    'nk': 'ङ्क',
    // cha-varga
    'n\\.j': 'ञ्ज़',
    'nJ': 'ञ्झ',
    'nj': 'ञ्ज',
    // Not using rules like:
    // 'न\u001D?c' - ञच below in order to support nchh
    'nC': 'ञ्छ',
    'nCh': 'ञ्छ',
    'nchh': 'ञ्छ',
    'nch': 'ञ्च',
    'nc': 'ञ्च',
    // Ta-varga
    'n\\.D': 'ण्ड़',
    'nD': 'ण्ड',
    'nT': 'ण्ट',

    // अकार followed by व्यञ्जन
    // Rules such as the below are not necessary.
    //'([क-हक़-य़])\u001D?aद\u001D?h': '$1ध',
    // Major conjoint consonants.
    '([क-हक़-य़])\u001D?ax': '$1क्ष',
    // Ungrouped consonants.
    // Reverse order - except for sibilants.
    '([क-हक़-य़])\u001D?ah': '$1ह',
    '([क-हक़-य़])\u001D?aS': '$1ष',
    '([क-हक़-य़])\u001D?az': '$1श',
    '([क-हक़-य़])\u001D?as': '$1स',
    '([क-हक़-य़])\u001D?av': '$1व',
    '([क-हक़-य़])\u001D?aw': '$1व',
    '([क-हक़-य़])\u001D?aL': '$1ळ',
    '([क-हक़-य़])\u001D?a\\.L': '$1ऴ',
    '([क-हक़-य़])\u001D?al': '$1ल',
    '([क-हक़-य़])\u001D?ar': '$1र',
    '([क-हक़-य़])\u001D?a\\.r': '$1ऱ',
    '([क-हक़-य़])\u001D?aY': '$1य़',
    '([क-हक़-य़])\u001D?a\\.y': '$1य़',
    '([क-हक़-य़])\u001D?ay': '$1य',
    // Grouped consonants in reverse (by group).
    // ka-varga
    '([क-हक़-य़])\u001D?a~N': '$1ङ',
    '([क-हक़-य़])\u001D?aG': '$1घ',
    '([क-हक़-य़])\u001D?a\\.g': '$1ग़',
    '([क-हक़-य़])\u001D?ag': '$1ग',
    '([क-हक़-य़])\u001D?aK': '$1ख',
    '([क-हक़-य़])\u001D?a\\.K': '$1ख़',
    '([क-हक़-य़])\u001D?aq': '$1क़',
    '([क-हक़-य़])\u001D?a\\.k': '$1क़',
    '([क-हक़-य़])\u001D?ak': '$1क',
    // cha-varga
    '([क-हक़-य़])\u001D?a~n': '$1ञ',
    '([क-हक़-य़])\u001D?aJ': '$1झ',
    '([क-हक़-य़])\u001D?a\\.j': '$1ज़',
    '([क-हक़-य़])\u001D?aj': '$1ज',
    '([क-हक़-य़])\u001D?achh': '$1छ',
    '([क-हक़-य़])\u001D?aCh': '$1छ',
    '([क-हक़-य़])\u001D?ach': '$1च',
    '([क-हक़-य़])\u001D?aC': '$1छ',
    '([क-हक़-य़])\u001D?ac': '$1च',
    // Ta-varga
    '([क-हक़-य़])\u001D?aN': '$1ण',
    '([क-हक़-य़])\u001D?a\\.D': '$1ड़',
    '([क-हक़-य़])\u001D?aD': '$1ड',
    '([क-हक़-य़])\u001D?aT': '$1ट',
    // da-varga
    '([क-हक़-य़])\u001D?an': '$1न',
    '([क-हक़-य़])\u001D?ad': '$1द',
    '([क-हक़-य़])\u001D?at': '$1त',
    // pa-varga
    '([क-हक़-य़])\u001D?am': '$1म',
    '([क-हक़-य़])\u001D?aB': '$1भ',
    '([क-हक़-य़])\u001D?ab': '$1ब',
    '([क-हक़-य़])\u001D?af': '$1फ़',
    '([क-हक़-य़])\u001D?a\\.P': '$1फ़',
    '([क-हक़-य़])\u001D?aP': '$1फ',
    '([क-हक़-य़])\u001D?ap': '$1प',

    // daNDa-s
    // Constraints:
    // a] Remember .a maps to ऽ.
    // b] Also having । appear before the keystroke .a
    //    is completed is undesirable. । and ऽ  have no relationship, unlike
    //    say ट् and ठ्, इ and ई .
    // A rule like the following (which needs to be fixed)
    // would be desirable.
    // '([\u0900-ॣ])\u001D?\\.(\\s)': '$1।$2',
    // or simply '\\.(\\s)': '।$1'.
    // The below does not catch .\n.
    '\\.(\\s)': '।$1',
    '\\.\\.': '।',
    '\\.,': '॥',
    '\\|': '।',
    '।\u001D?\\|': '॥',
    '।\u001D?\\.': '…'
  },
  // If there are cases where, without looking at history of the chars pressed,
  // you cannot deduce what the output would be, then something like
  // the following is required.
  'historyPruneRegex': 'n(\\.)?|c|ch|C|nc|nC|nch|\\.|a'
};

// Load the layout and inform the keyboard to switch layout if necessary.
cros_vk_loadme(DEVA_PHONE_LAYOUT);
