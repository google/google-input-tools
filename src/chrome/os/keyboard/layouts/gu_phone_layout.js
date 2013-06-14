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
 * @fileoverview VK layout definition for Gujarati phonetic.
 */

var GU_PHONE_LAYOUT = {
  'id': 'gu_phone',
  'title': '\u0A97\u0AC1\u0A9C\u0AB0\u0ABE\u0AA4\u0AC0 (\u0AAB\u0ACB\u0AA8' +
      '\u0AC7\u0AA4\u0ABF\u0A95)',
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
      '': '\u0AF1\u0AE6\u0AE7\u0AE8\u0AE9\u0AEA\u0AEB\u0AEC\u0AED\u0AEE\u0AEF' +
          '\u0AD0\u0A85{{\u0A85\u0A82}}{{\u0A85\u0A83}}\u0A86\u0A87\u0A88' +
          '\u0A89' +
          '\u0A8A\u0A8B\u0AE0\u0A8C\u0AE1\u0A8D\u0A8F\u0A90\u0A91\u0A93\u0A94' +
          '\u0A95{{\u0A95\u0ACD\u0AB7}}\u0A96\u0A97\u0A98\u0A99\u0A9A\u0A9B' +
          '\u0A9C{{\u0A9C\u0ACD\u0A9E}}\u0A9D\u0A9E\u0A9F\u0AA0\u0AA1\u0AA2' +
          '\u0AA3'
    },
    'sc': {
      '': '\u0AA4{{\u0AA4\u0ACD\u0AB0}}\u0AA5\u0AA6\u0AA7\u0AA8\u0AAA\u0AAB' +
          '\u0AAC\u0AAD\u0AAE\u0AAF\u0AB0\u0AB2\u0AB3\u0AB5\u0AB6{{\u0AB6' +
          '\u0ACD\u0AB0}}\u0AB7\u0AB8\u0AB9\u0ABC\u0A81\u0A82\u0ACD\u0ABE' +
          '\u0ABF\u0AC0\u0AC1\u0AC2\u0AC3\u0AC4\u0AE2\u0AE3\u0AC5\u0AC7\u0AC8' +
          '\u0AC9\u0ACB\u0ACC\u0ABD'
    }
  },
  'transform': {
    '\u0ACD\u001D?\\.c': '\u0AC5',
    '\u0A86\u0A8A\u001D?M': '\u0AD0',
    '\u0AB0\u0ABC\u001D?\\^i': '\u0A8B',
    '\u0AB0\u0ABC\u001D?\\^I': '\u0AE0',
    '\u0AB3\u001D?\\^i': '\u0A8C',
    '\u0AB3\u001D?\\^I': '\u0AE1',
    '\u0A9A\u001D?h': '\u0A9B',
    '\u0AA1\u0ABC\u001D?h': '\u0AA2\u0ABC',
    '\u0A95\u0ACD\u0AB7\u001D?h': '\u0A95\u0ACD\u0AB7',
    '\\.n': '\u0A82',
    '\\.m': '\u0A82',
    '\\.N': '\u0A81',
    '\\.h': '\u0ACD\u200C',
    '\\.a': '\u0ABD',
    'OM': '\u0AD0',
    '\u0A85\u001D?a': '\u0A86',
    '\u0A87\u001D?i': '\u0A88',
    '\u0A8F\u001D?e': '\u0A88',
    '\u0A89\u001D?u': '\u0A8A',
    '\u0A93\u001D?o': '\u0A8A',
    '\u0A85\u001D?i': '\u0A90',
    '\u0A85\u001D?u': '\u0A94',
    '\u0A95\u001D?h': '\u0A96',
    '\u0A97\u001D?h': '\u0A98',
    '~N': '\u0A99',
    'ch': '\u0A9A',
    'Ch': '\u0A9B',
    '\u0A9C\u001D?h': '\u0A9D',
    '~n': '\u0A9E',
    '\u0A9F\u001D?h': '\u0AA0',
    '\u0AA1\u001D?h': '\u0AA2',
    '\u0AA4\u001D?h': '\u0AA5',
    '\u0AA6\u001D?h': '\u0AA7',
    '\u0AAA\u001D?h': '\u0AAB',
    '\u0AAC\u001D?h': '\u0AAD',
    '\u0AB8\u001D?h': '\u0AB6',
    '\u0AB6\u001D?h': '\u0AB7',
    '~h': '\u0ACD\u0AB9',
    'Kh': '\u0A96\u0ABC',
    '\\.D': '\u0AA1\u0ABC',
    '\u0AB0\u0ABC\u001D?h': '\u0AA2\u0ABC',
    '\u0A95\u001D?S': '\u0A95\u0ACD\u0AB7',
    '\u0A97\u0ABC\u001D?Y': '\u0A9C\u0ACD\u0A9E',
    'M': '\u0A82',
    'H': '\u0A83',
    'a': '\u0A85',
    'A': '\u0A86',
    'i': '\u0A87',
    'I': '\u0A88',
    'u': '\u0A89',
    'U': '\u0A8A',
    'e': '\u0A8F',
    'o': '\u0A93',
    'k': '\u0A95',
    'g': '\u0A97',
    'j': '\u0A9C',
    'T': '\u0A9F',
    'D': '\u0AA1',
    'N': '\u0AA3',
    't': '\u0AA4',
    'd': '\u0AA6',
    'n': '\u0AA8',
    'p': '\u0AAA',
    'b': '\u0AAC',
    'm': '\u0AAE',
    'y': '\u0AAF',
    'r': '\u0AB0',
    'l': '\u0AB2',
    'L': '\u0AB3',
    'v': '\u0AB5',
    'w': '\u0AB5',
    'S': '\u0AB6',
    's': '\u0AB8',
    'h': '\u0AB9',
    'R': '\u0AB0\u0ABC',
    'q': '\u0A95\u0ABC',
    'G': '\u0A97\u0ABC',
    'z': '\u0A9C\u0ABC',
    'J': '\u0A9C\u0ABC',
    'f': '\u0AAB\u0ABC',
    'Y': '\u0AAF\u0ABC',
    'x': '\u0A95\u0ACD\u0AB7',
    '([\u0A95-\u0AB9])\u001Da': '$1',
    '([\u0A95-\u0AB9])\u001Daa': '$1\u0ABE',
    '([\u0A95-\u0AB9])\u001Dai': '$1\u0AC8',
    '([\u0A95-\u0AB9])\u001Dau': '$1\u0ACC',
    '([\u0A95-\u0AB9])\u001DA': '$1\u0ABE',
    '([\u0A95-\u0AB9])\u001Di': '$1\u0ABF',
    '\u0ABF\u001Di': '\u0AC0',
    '\u0AC7\u001De': '\u0AC0',
    '([\u0A95-\u0AB9])\u001DI': '$1\u0AC0',
    '([\u0A95-\u0AB9])\u001Du': '$1\u0AC1',
    '([\u0A95-\u0AB9])\u001DU': '$1\u0AC2',
    '\u0AC1\u001Du': '\u0AC2',
    '\u0ACB\u001Do': '\u0AC2',
    '([\u0A95-\u0AB9])\u0ACD\u0AB0\u0ABC\u0ACD\u0AB0\u0ABC\u001Di': '$1\u0AC3',
    '([\u0A95-\u0AB9])\u0ACD\u0AB0\u0ABC^i': '$1\u0AC3',
    '([\u0A95-\u0AB9])\u0ACD\u0AB0\u0ABC\u0ACD\u0AB0\u0ABC\u001DI': '$1\u0AC4',
    '([\u0A95-\u0AB9])\u0ACD\u0AB0\u0ABC^I': '$1\u0AC4',
    '\u0AB0\u0ABC\u0ACD\u0AB0\u0ABC\u001Di': '\u0A8B',
    '\u0AB0\u0ABC\u0ACD\u0AB0\u0ABC\u001DI': '\u0AE0',
    '\u0AB3\u0ACD\u0AB3\u001Di': '\u0A8C',
    '\u0AB3\u0ACD\u0AB3\u001DI': '\u0AE1',
    '([\u0A95-\u0AB9])\u001De': '$1\u0AC7',
    '([\u0A95-\u0AB9])\u001Do': '$1\u0ACB',
    '([\u0A95-\u0AB9])\u001Dk': '$1\u0ACD\u0A95',
    '([\u0A95-\u0AB9])\u001Dg': '$1\u0ACD\u0A97',
    '([\u0A95-\u0AB9])\u001D~N': '$1\u0ACD\u0A99',
    '([\u0A95-\u0AB9])\u001Dch': '$1\u0ACD\u0A9A',
    '([\u0A95-\u0AB9])\u001DCh': '$1\u0ACD\u0A9B',
    '([\u0A95-\u0AB9])\u001Dj': '$1\u0ACD\u0A9C',
    '([\u0A95-\u0AB9])\u001D~n': '$1\u0ACD\u0A9E',
    '([\u0A95-\u0AB9])\u001DT': '$1\u0ACD\u0A9F',
    '([\u0A95-\u0AB9])\u001DD': '$1\u0ACD\u0AA1',
    '([\u0A95-\u0AB9])\u001DN': '$1\u0ACD\u0AA3',
    '([\u0A95-\u0AB9])\u001Dt': '$1\u0ACD\u0AA4',
    '([\u0A95-\u0AB9])\u001Dd': '$1\u0ACD\u0AA6',
    '([\u0A95-\u0AB9])\u001Dn': '$1\u0ACD\u0AA8',
    '([\u0A95-\u0AB9])\u001Dp': '$1\u0ACD\u0AAA',
    '([\u0A95-\u0AB9])\u001Db': '$1\u0ACD\u0AAC',
    '([\u0A95-\u0AB9])\u001Dm': '$1\u0ACD\u0AAE',
    //'([\u0A95-\u0AB9])\u001Dy': '$1\u0ACD\u0AAF',
    '([\u0A95-\u0AB9])\u001Dr': '$1\u0ACD\u0AB0',
    '([\u0A95-\u0AB9])\u001Dl': '$1\u0ACD\u0AB2',
    '([\u0A95-\u0AB9])\u001DL': '$1\u0ACD\u0AB3',
    '([\u0A95-\u0AB9])\u001Dv': '$1\u0ACD\u0AB5',
    '([\u0A95-\u0AB9])\u001Dw': '$1\u0ACD\u0AB5',
    '([\u0A95-\u0AB9])\u001DS': '$1\u0ACD\u0AB6',
    '([\u0A95-\u0AB9])\u001Ds': '$1\u0ACD\u0AB8',
    '([\u0A95-\u0AB9])\u001Dh': '$1\u0ACD\u0AB9',
    '([\u0A95-\u0AB9])\u001DR': '$1\u0ACD\u0AB0\u0ABC',
    '([\u0A95-\u0AB9])\u001Dq': '$1\u0ACD\u0A95\u0ABC',
    '([\u0A95-\u0AB9])\u001DKh': '$1\u0ACD\u0A96\u0ABC',
    '([\u0A95-\u0AB9])\u001DG': '$1\u0ACD\u0A97\u0ABC',
    '([\u0A95-\u0AB9])\u001Dz': '$1\u0ACD\u0A9C\u0ABC',
    '([\u0A95-\u0AB9])\u001DJ': '$1\u0ACD\u0A9C\u0ABC',
    '([\u0A95-\u0AB9])\u001D.D': '$1\u0ACD\u0AA1\u0ABC',
    '([\u0A95-\u0AB9])\u001Df': '$1\u0ACD\u0AAB\u0ABC',
    '([\u0A95-\u0AB9])\u001Dy': '$1\u0ACD\u0AAF\u0ABC',
    '([\u0A95-\u0AB9])\u001Dx': '$1\u0ACD\u0A95\u0ACD\u0AB7',
    '([\u0A95-\u0AB9])\u001Dak': '$1\u0A95',
    '([\u0A95-\u0AB9])\u001Dag': '$1\u0A97',
    '([\u0A95-\u0AB9])\u001Da~N': '$1\u0A99',
    '([\u0A95-\u0AB9])\u001Dach': '$1\u0A9A',
    '([\u0A95-\u0AB9])\u001DaCh': '$1\u0A9B',
    '([\u0A95-\u0AB9])\u001Daj': '$1\u0A9C',
    '([\u0A95-\u0AB9])\u001Da~n': '$1\u0A9E',
    '([\u0A95-\u0AB9])\u001DaT': '$1\u0A9F',
    '([\u0A95-\u0AB9])\u001DaD': '$1\u0AA1',
    '([\u0A95-\u0AB9])\u001DaN': '$1\u0AA3',
    '([\u0A95-\u0AB9])\u001Dat': '$1\u0AA4',
    '([\u0A95-\u0AB9])\u001Dad': '$1\u0AA6',
    '([\u0A95-\u0AB9])\u001Dan': '$1\u0AA8',
    '([\u0A95-\u0AB9])\u001Dap': '$1\u0AAA',
    '([\u0A95-\u0AB9])\u001Dab': '$1\u0AAC',
    '([\u0A95-\u0AB9])\u001Dam': '$1\u0AAE',
    //'([\u0A95-\u0AB9])\u001Day': '$1\u0AAF',
    '([\u0A95-\u0AB9])\u001Dar': '$1\u0AB0',
    '([\u0A95-\u0AB9])\u001Dal': '$1\u0AB2',
    '([\u0A95-\u0AB9])\u001DaL': '$1\u0AB3',
    '([\u0A95-\u0AB9])\u001Dav': '$1\u0AB5',
    '([\u0A95-\u0AB9])\u001Daw': '$1\u0AB5',
    '([\u0A95-\u0AB9])\u001DaS': '$1\u0AB6',
    '([\u0A95-\u0AB9])\u001Das': '$1\u0AB8',
    '([\u0A95-\u0AB9])\u001Dah': '$1\u0AB9',
    '([\u0A95-\u0AB9])\u001DaR': '$1\u0AB0\u0ABC',
    '([\u0A95-\u0AB9])\u001Daq': '$1\u0A95\u0ABC',
    '([\u0A95-\u0AB9])\u001DaKh': '$1\u0A96\u0ABC',
    '([\u0A95-\u0AB9])\u001DaG': '$1\u0A97\u0ABC',
    '([\u0A95-\u0AB9])\u001Daz': '$1\u0A9C\u0ABC',
    '([\u0A95-\u0AB9])\u001DaJ': '$1\u0A9C\u0ABC',
    '([\u0A95-\u0AB9])\u001Da.D': '$1\u0AA1\u0ABC',
    '([\u0A95-\u0AB9])\u001Daf': '$1\u0AAB\u0ABC',
    '([\u0A95-\u0AB9])\u001Day': '$1\u0AAF\u0ABC',
    '([\u0A95-\u0AB9])\u001Dax': '$1\u0A95\u0ACD\u0AB7',
    '([\u0A95-\u0AB9])\u001Daak': '$1\u0ABE\u0A95',
    '([\u0A95-\u0AB9])\u001Daag': '$1\u0ABE\u0A97',
    '([\u0A95-\u0AB9])\u001Daa~N': '$1\u0ABE\u0A99',
    '([\u0A95-\u0AB9])\u001Daach': '$1\u0ABE\u0A9A',
    '([\u0A95-\u0AB9])\u001DaaCh': '$1\u0ABE\u0A9B',
    '([\u0A95-\u0AB9])\u001Daaj': '$1\u0ABE\u0A9C',
    '([\u0A95-\u0AB9])\u001Daa~n': '$1\u0ABE\u0A9E',
    '([\u0A95-\u0AB9])\u001DaaT': '$1\u0ABE\u0A9F',
    '([\u0A95-\u0AB9])\u001DaaD': '$1\u0ABE\u0AA1',
    '([\u0A95-\u0AB9])\u001DaaN': '$1\u0ABE\u0AA3',
    '([\u0A95-\u0AB9])\u001Daat': '$1\u0ABE\u0AA4',
    '([\u0A95-\u0AB9])\u001Daad': '$1\u0ABE\u0AA6',
    '([\u0A95-\u0AB9])\u001Daan': '$1\u0ABE\u0AA8',
    '([\u0A95-\u0AB9])\u001Daap': '$1\u0ABE\u0AAA',
    '([\u0A95-\u0AB9])\u001Daab': '$1\u0ABE\u0AAC',
    '([\u0A95-\u0AB9])\u001Daam': '$1\u0ABE\u0AAE',
    //'([\u0A95-\u0AB9])\u001Daay': '$1\u0ABE\u0AAF',
    '([\u0A95-\u0AB9])\u001Daar': '$1\u0ABE\u0AB0',
    '([\u0A95-\u0AB9])\u001Daal': '$1\u0ABE\u0AB2',
    '([\u0A95-\u0AB9])\u001DaaL': '$1\u0ABE\u0AB3',
    '([\u0A95-\u0AB9])\u001Daav': '$1\u0ABE\u0AB5',
    '([\u0A95-\u0AB9])\u001Daaw': '$1\u0ABE\u0AB5',
    '([\u0A95-\u0AB9])\u001DaaS': '$1\u0ABE\u0AB6',
    '([\u0A95-\u0AB9])\u001Daas': '$1\u0ABE\u0AB8',
    '([\u0A95-\u0AB9])\u001Daah': '$1\u0ABE\u0AB9',
    '([\u0A95-\u0AB9])\u001DaaR': '$1\u0ABE\u0AB0\u0ABC',
    '([\u0A95-\u0AB9])\u001Daaq': '$1\u0ABE\u0A95\u0ABC',
    '([\u0A95-\u0AB9])\u001DaaKh': '$1\u0ABE\u0A96\u0ABC',
    '([\u0A95-\u0AB9])\u001DaaG': '$1\u0ABE\u0A97\u0ABC',
    '([\u0A95-\u0AB9])\u001Daaz': '$1\u0ABE\u0A9C\u0ABC',
    '([\u0A95-\u0AB9])\u001DaaJ': '$1\u0ABE\u0A9C\u0ABC',
    '([\u0A95-\u0AB9])\u001Daa.D': '$1\u0ABE\u0AA1\u0ABC',
    '([\u0A95-\u0AB9])\u001Daaf': '$1\u0ABE\u0AAB\u0ABC',
    '([\u0A95-\u0AB9])\u001Daay': '$1\u0ABE\u0AAF\u0ABC',
    '([\u0A95-\u0AB9])\u001Daax': '$1\u0ABE\u0A95\u0ACD\u0AB7'
  },
  'historyPruneRegex': 'a|aa|ac|aaC|aac|a\\.|aK|aC|aaK|aS|aaS|aa~|aa\\.|a~'
};

// Load the layout and inform the keyboard to switch layout if necessary.
cros_vk_loadme(GU_PHONE_LAYOUT);
