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
 * @fileoverview VK layout definition for Bengali phonetic.
 */

var BN_PHONE_LAYOUT = {
  'id': 'bn_phone',
  'title': '\u09AC\u09BE\u0982\u09B2\u09BE (\u09AB\u09CB\u09A8\u09C7\u099F' +
      '\u09BF\u0995)',
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
    },
    'c': {
      '': '\u0982\u0981\u09BC\u0983\u09FA\u09F8\u09F9\u09F2\u09F3\u09E6\u09F4' +
          '\u09E7\u09F5' +
          '\u09E8\u09F6\u09E9\u09F7\u09EA\u09EB\u09EC\u09ED\u09EE\u09EF\u0985' +
          '\u0986\u0987\u0988\u0989\u098A' +
          '\u098B\u09E0\u098C\u09E1\u098F\u0990\u0993\u0994\u0995\u0996\u0997' +
          '\u0998\u0999' +
          '\u099A\u099B\u099C\u099D\u099E'
    },
    'sc': {
      '': '\u099F\u09A0\u09A1\u09DC\u09A2\u09DD\u09A3\u09A4\u09CE\u09A5\u09A6' +
          '\u09A7\u09A8\u09AA\u09AB\u09AC\u09AD\u09AE\u09AF\u09DF\u09B0\u09F0' +
          '\u09B2' +
          '\u09F1\u09B6\u09B7\u09B8\u09B9\u09BD\u09BE\u09BF\u09C0\u09C1\u09C2' +
          '\u09C3\u09C4\u09E2\u09E3\u09C7' +
          '\u09C8\u09CB\u09CC\u09CD\u09D7'
    }
  },
  'transform': {
    '\u09CD\u001D?\\.c': '\u09C7',
    '\u0986\u098A\u001D?M': '\u0993\u0982',
    '\u09DC\u001D?\\^i': '\u098B',
    '\u09DC\u001D?\\^I': '\u09E0',
    '\u09B2\u001D?\\^i': '\u098C',
    '\u09B2\u001D?\\^I': '\u09E1',
    '\u099A\u001D?h': '\u099B',
    '\u0995\u09CD\u09B7\u001D?h': '\u0995\u09CD\u09B7',
    '\\.n': '\u0982',
    '\\.m': '\u0982',
    '\\.N': '\u0981',
    '\\.h': '\u09CD\u200C',
    '\\.a': '\u09BD',
    'OM': '\u0993\u0982',
    '\u0985\u001D?a': '\u0986',
    '\u0987\u001D?i': '\u0988',
    '\u098F\u001D?e': '\u0988',
    '\u0989\u001D?u': '\u098A',
    '\u0993\u001D?o': '\u098A',
    '\u0985\u001D?i': '\u0990',
    '\u0985\u001D?u': '\u0994',
    '\u0995\u001D?h': '\u0996',
    '\u0997\u001D?h': '\u0998',
    '~N': '\u0999',
    'ch': '\u099A',
    'Ch': '\u099B',
    '\u099C\u001D?h': '\u099D',
    '~n': '\u099E',
    '\u099F\u001D?h': '\u09A0',
    '\u09A1\u001D?h': '\u09A2',
    '\u09A4\u001D?h': '\u09A5',
    '\u09A6\u001D?h': '\u09A7',
    '\u09AA\u001D?h': '\u09AB',
    '\u09AC\u001D?h': '\u09AD',
    '\u09B8\u001D?h': '\u09B6',
    '\u09B6\u001D?h': '\u09B7',
    '~h': '\u09CD\u09B9',
    '\u09DC\u001D?h': '\u09DD',
    '\u0995\u001D?S': '\u0995\u09CD\u09B7',
    'GY': '\u099C\u09CD\u099E',
    'M': '\u0982',
    'H': '\u0983',
    'a': '\u0985',
    'A': '\u0986',
    'i': '\u0987',
    'I': '\u0988',
    'u': '\u0989',
    'U': '\u098A',
    'e': '\u098F',
    'o': '\u0993',
    'k': '\u0995',
    'g': '\u0997',
    'j': '\u099C',
    'T': '\u099F',
    'D': '\u09A1',
    'N': '\u09A3',
    't': '\u09A4',
    'd': '\u09A6',
    'n': '\u09A8',
    'p': '\u09AA',
    'b': '\u09AC',
    'm': '\u09AE',
    'y': '\u09AF',
    'r': '\u09B0',
    'l': '\u09B2',
    'L': '\u09B2',
    'v': '\u09AC',
    'w': '\u09AC',
    'S': '\u09B6',
    's': '\u09B8',
    'h': '\u09B9',
    'R': '\u09DC',
    'Y': '\u09DF',
    'x': '\u0995\u09CD\u09B7',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001Da': '$1',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001Daa': '$1\u09BE',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001Dai': '$1\u09C8',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001Dau': '$1\u09CC',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001DA': '$1\u09BE',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001Di': '$1\u09BF',
    '\u09BF\u001Di': '\u09C0',
    '\u09C7\u001De': '\u09C0',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001DI': '$1\u09C0',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001Du': '$1\u09C1',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001DU': '$1\u09C2',
    '\u09C1\u001Du': '\u09C2',
    '\u09CB\u001Do': '\u09C2',
    '([\u0995-\u09B9\u09DC-\u09DF])\u09CD\u09B0\u09BC\u09CD\u09B0\u09BC\u001Di':
        '$1\u09C3',
    '([\u0995-\u09B9\u09DC-\u09DF])\u09CD\u09B0\u09BC^i': '$1\u09C3',
    '([\u0995-\u09B9\u09DC-\u09DF])\u09CD\u09B0\u09BC\u09CD\u09B0\u09BC\u001DI':
        '$1\u09C4',
    '([\u0995-\u09B9\u09DC-\u09DF])\u09CD\u09B0\u09BC^I': '$1\u09C4',
    '\u09B0\u09BC\u09CD\u09B0\u09BC\u001Di': '\u098B',
    '\u09B0\u09BC\u09CD\u09B0\u09BC\u001DI': '\u09E0',
    '\u09B2\u09CD\u09B2\u001Di': '\u098C',
    '\u09B2\u09CD\u09B2\u001DI': '\u09E1',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001De': '$1\u09C7',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001Do': '$1\u09CB',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001Dk': '$1\u09CD\u0995',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001Dg': '$1\u09CD\u0997',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001D~N': '$1\u09CD\u0999',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001Dch': '$1\u09CD\u099A',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001DCh': '$1\u09CD\u099B',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001Dj': '$1\u09CD\u099C',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001D~n': '$1\u09CD\u099E',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001DT': '$1\u09CD\u099F',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001DD': '$1\u09CD\u09A1',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001DN': '$1\u09CD\u09A3',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001Dt': '$1\u09CD\u09A4',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001Dd': '$1\u09CD\u09A6',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001Dn': '$1\u09CD\u09A8',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001Dp': '$1\u09CD\u09AA',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001Db': '$1\u09CD\u09AC',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001Dm': '$1\u09CD\u09AE',
    //'([\u0995-\u09B9\u09DC-\u09DF])\u001Dy': '$1\u09CD\u09AF',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001Dr': '$1\u09CD\u09B0',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001Dl': '$1\u09CD\u09B2',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001DL': '$1\u09CD\u09B2',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001Dv': '$1\u09CD\u09AC',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001Dw': '$1\u09CD\u09AC',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001DS': '$1\u09CD\u09B6',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001Ds': '$1\u09CD\u09B8',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001Dh': '$1\u09CD\u09B9',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001DR': '$1\u09CD\u09B0\u09BC',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001Dq': '$1\u09CD\u0995\u09BC',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001DKh': '$1\u09CD\u0996\u09BC',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001DG': '$1\u09CD\u0997\u09BC',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001Dz': '$1\u09CD\u099C\u09BC',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001DJ': '$1\u09CD\u099C\u09BC',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001D.D': '$1\u09CD\u09A1\u09BC',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001Df': '$1\u09CD\u09AB\u09BC',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001Dy': '$1\u09CD\u09AF\u09BC',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001Dx': '$1\u09CD\u0995\u09CD\u09B7',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001Dak': '$1\u0995',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001Dag': '$1\u0997',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001Da~N': '$1\u0999',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001Dach': '$1\u099A',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001DaCh': '$1\u099B',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001Daj': '$1\u099C',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001Da~n': '$1\u099E',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001DaT': '$1\u099F',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001DaD': '$1\u09A1',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001DaN': '$1\u09A3',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001Dat': '$1\u09A4',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001Dad': '$1\u09A6',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001Dan': '$1\u09A8',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001Dap': '$1\u09AA',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001Dab': '$1\u09AC',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001Dam': '$1\u09AE',
    //'([\u0995-\u09B9\u09DC-\u09DF])\u001Day': '$1\u09AF',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001Dar': '$1\u09B0',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001Dal': '$1\u09B2',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001DaL': '$1\u09B2',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001Dav': '$1\u09AC',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001Daw': '$1\u09AC',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001DaS': '$1\u09B6',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001Das': '$1\u09B8',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001Dah': '$1\u09B9',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001DaR': '$1\u09B0\u09BC',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001Daq': '$1\u0995\u09BC',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001DaKh': '$1\u0996\u09BC',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001DaG': '$1\u0997\u09BC',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001Daz': '$1\u099C\u09BC',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001DaJ': '$1\u099C\u09BC',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001Da.D': '$1\u09A1\u09BC',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001Daf': '$1\u09AB\u09BC',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001Day': '$1\u09AF\u09BC',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001Dax': '$1\u0995\u09CD\u09B7',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001Daak': '$1\u09BE\u0995',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001Daag': '$1\u09BE\u0997',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001Daa~N': '$1\u09BE\u0999',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001Daach': '$1\u09BE\u099A',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001DaaCh': '$1\u09BE\u099B',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001Daaj': '$1\u09BE\u099C',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001Daa~n': '$1\u09BE\u099E',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001DaaT': '$1\u09BE\u099F',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001DaaD': '$1\u09BE\u09A1',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001DaaN': '$1\u09BE\u09A3',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001Daat': '$1\u09BE\u09A4',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001Daad': '$1\u09BE\u09A6',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001Daan': '$1\u09BE\u09A8',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001Daap': '$1\u09BE\u09AA',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001Daab': '$1\u09BE\u09AC',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001Daam': '$1\u09BE\u09AE',
    //'([\u0995-\u09B9\u09DC-\u09DF])\u001Daay': '$1\u09BE\u09AF',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001Daar': '$1\u09BE\u09B0',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001Daal': '$1\u09BE\u09B2',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001DaaL': '$1\u09BE\u09B2',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001Daav': '$1\u09BE\u09AC',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001Daaw': '$1\u09BE\u09AC',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001DaaS': '$1\u09BE\u09B6',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001Daas': '$1\u09BE\u09B8',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001Daah': '$1\u09BE\u09B9',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001DaaR': '$1\u09BE\u09B0\u09BC',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001Daaq': '$1\u09BE\u0995\u09BC',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001DaaKh': '$1\u09BE\u0996\u09BC',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001DaaG': '$1\u09BE\u0997\u09BC',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001Daaz': '$1\u09BE\u099C\u09BC',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001DaaJ': '$1\u09BE\u099C\u09BC',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001Daa.D': '$1\u09BE\u09A1\u09BC',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001Daaf': '$1\u09BE\u09AB\u09BC',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001Daay': '$1\u09BE\u09AF\u09BC',
    '([\u0995-\u09B9\u09DC-\u09DF])\u001Daax': '$1\u09BE\u0995\u09CD\u09B7'
  },
  'historyPruneRegex': 'a|aa|ac|aaC|aac|a\\.|aK|aC|aaK|aS|aaS|aa~|aa\\.|a~'
};

// Load the layout and inform the keyboard to switch layout if necessary.
cros_vk_loadme(BN_PHONE_LAYOUT);
