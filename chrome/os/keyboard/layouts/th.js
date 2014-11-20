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
 * @fileoverview VK layout definition for Thai kedmanee.
 */

var TH_LAYOUT = {
  'id': 'th',
  'title': '\u0e20\u0e32\u0e29\u0e32\u0e44\u0e17\u0e22',
  'mappings': {
    'sl,': {
      '\u00c0': '_',
      '1': '\u0e45',
      '2': '/',
      '3': '-',
      '4': '\u0e20',
      '5': '\u0e16',
      '6': '\u0e38',
      '7': '\u0e36',
      '8': '\u0e04',
      '9': '\u0e15',
      '0': '\u0e08',
      'm': '\u0e02',
      '=': '\u0e0a',
      'Q': '\u0e46',
      'W': '\u0e44',
      'E': '\u0e33',
      'R': '\u0e1e',
      'T': '\u0e30',
      'Y': '\u0e31',
      'U': '\u0e35',
      'I': '\u0e23',
      'O': '\u0e19',
      'P': '\u0e22',
      '\u00db': '\u0e1a',
      '\u00dd': '\u0e25',
      '\u00dc': '\u0e03',
      'A': '\u0e1f',
      'S': '\u0e2b',
      'D': '\u0e01',
      'F': '\u0e14',
      'G': '\u0e40',
      'H': '\u0e49',
      'J': '\u0e48',
      'K': '\u0e32',
      'L': '\u0e2a',
      ';': '\u0e27',
      '\u00de': '\u0e07',
      'Z': '\u0e1c',
      'X': '\u0e1b',
      'C': '\u0e41',
      'V': '\u0e2d',
      'B': '\u0e34',
      'N': '\u0e37',
      'M': '\u0e17',
      '\u00bc': '\u0e21',
      '\u00be': '\u0e43',
      '\u00bf': '\u0e1d'
    },
    'cl,c': {
      '\u00db': '%',
      '\u00dd': '\u0e51',
      '\u00dc': '+'
    },
    'l,s': {
      '\u00c0': '%',
      '1': '+',
      '2': '\u0e51',
      '3': '\u0e52',
      '4': '\u0e53',
      '5': '\u0e54',
      '6': '\u0e39',
      '7': '\u0e3f',
      '8': '\u0e55',
      '9': '\u0e56',
      '0': '\u0e57',
      'm': '\u0e58',
      '=': '\u0e59',
      'Q': '\u0e50',
      'W': '\"',
      'E': '\u0e0e',
      'R': '\u0e11',
      'T': '\u0e18',
      'Y': '\u0e4d',
      'U': '\u0e4a',
      'I': '\u0e13',
      'O': '\u0e2f',
      'P': '\u0e0d',
      '\u00db': '\u0e10',
      '\u00dd': ',',
      '\u00dc': '\u0e05',
      'A': '\u0e24',
      'S': '\u0e06',
      'D': '\u0e0f',
      'F': '\u0e42',
      'G': '\u0e0c',
      'H': '\u0e47',
      'J': '\u0e4b',
      'K': '\u0e29',
      'L': '\u0e28',
      ';': '\u0e0b',
      '\u00de': '.',
      'Z': '(',
      'X': ')',
      'C': '\u0e09',
      'V': '\u0e2e',
      'B': '\u0e3a',
      'N': '\u0e4c',
      'M': '?',
      '\u00bc': '\u0e12',
      '\u00be': '\u0e2c',
      '\u00bf': '\u0e26'
    }
  }
};

// Load the layout and inform the keyboard to switch layout if necessary.
cros_vk_loadme(TH_LAYOUT);
