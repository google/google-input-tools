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
 * @fileoverview VK layout definition for Tamil phonetic.
 */

var TA_PHONE_LAYOUT = {
  'id': 'ta_phone',
  'title': '\u0BA4\u0BAE\u0BBF\u0BB4\u0BCD (\u0BAA\u0BCB\u0BA9\u0BC6\u0B9F' +
      '\u0BBF\u0B95\u0BCD)',
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
      '': '\u0B82\u0BF3\u0BF4\u0BF5\u0BF6\u0BF7\u0BF8\u0BFA\u0BF0\u0BF1\u0BF2' +
          '\u0BF9\u0BE6' +
          '\u0BE7\u0BE8\u0BE9\u0BEA\u0BEB\u0BEC\u0BED\u0BEE\u0BEF\u0BD0\u0B83' +
          '\u0B85\u0B86' +
          '\u0B87\u0B88\u0B89\u0B8A\u0B8E\u0B8F\u0B90\u0B92\u0B93\u0B94\u0B95' +
          '\u0B99\u0B9A\u0B9C\u0B9E\u0B9F\u0BA3\u0BA4\u0BA8\u0BA9\u0BAA'
    },
    'sc': {
      '': '\u0BAE\u0BAF\u0BB0\u0BB1\u0BB2\u0BB3\u0BB4\u0BB5\u0BB6\u0BB7\u0BB8' +
          '\u0BB9\u0BBE\u0BBF' +
          '\u0BC0\u0BC1\u0BC2\u0BC6\u0BC7\u0BC8\u0BCA\u0BCB\u0BCC\u0BCD\u0BD7'
    }
  },
  'transform': {
    '\u0BCD\u0BB1\u0BCD\u0BB1\u0BCD\u001D?i': '\u0BCD\u0BB0\u0BBF',
    '\u0BCD\u0BB1\u0BCD\u001D?\\^i': '\u0BCD\u0BB0\u0BBF',
    '\u0BCD\u0BB1\u0BCD\u0BB1\u0BCD\u001D?I': '\u0BCD\u0BB0\u0BBF',
    '\u0BCD\u0BB1\u0BCD\u001D?\\^I': '\u0BCD\u0BB0\u0BBF',
    '\u0BCD\u0B85\u001D?a': '\u0BBE',
    '\u0BBF\u001D?i': '\u0BC0',
    '\u0BC6\u001D?e': '\u0BC0',
    '\u0BC1\u001D?u': '\u0BC2',
    '\u0BCA\u001D?o': '\u0BC2',
    '\u0BCD\u0B85\u001D?i': '\u0BC8',
    '\u0BCD\u0B85\u001D?u': '\u0BCC',
    '\u0BCA\u001D?u': '\u0BCC',
    '\u0BCD\u001D?a': '',
    '\u0BCD\u001D?A': '\u0BBE',
    '\u0BCD\u001D?i': '\u0BBF',
    '\u0BCD\u001D?I': '\u0BC0',
    '\u0BCD\u001D?u': '\u0BC1',
    '\u0BCD\u001D?U': '\u0BC2',
    '\u0BCD\u001D?e': '\u0BC6',
    '\u0BCD\u001D?E': '\u0BC7',
    '\u0BCD\u001D?o': '\u0BCA',
    '\u0BCD\u001D?O': '\u0BCB',
    '\u0BA9\u0BCD\u001D?ch': '\u0B9E\u0BCD\u0B9A\u0BCD',
    '\u0B95\u0BCD\u0B9A\u0BCD\u001D?h': '\u0B95\u0BCD\u0BB7\u0BCD',
    '\u0B9A\u0BCD\u0BB0\u0BCD\u001D?i': '\u0BB8\u0BCD\u0BB0\u0BC0',
    '\u0B9F\u0BCD\u0B9F\u0BCD\u001D?r': '\u0BB1\u0BCD\u0BB1\u0BCD',
    '\u0B85\u001D?a': '\u0B86',
    '\u0B87\u001D?i': '\u0B88',
    '\u0B8E\u001D?e': '\u0B88',
    '\u0B89\u001D?u': '\u0B8A',
    '\u0B92\u001D?o': '\u0B8A',
    '\u0B85\u001D?i': '\u0B90',
    '\u0B85\u001D?u': '\u0B94',
    '\u0B92\u001D?u': '\u0B94',
    '\u0BA9\u0BCD\u001D?g': '\u0B99\u0BCD',
    'ch': '\u0B9A\u0BCD',
    '\u0BA9\u0BCD\u001D?j': '\u0B9E\u0BCD',
    '\u0B9F\u0BCD\u001D?h': '\u0BA4\u0BCD',
    '\u0B9A\u0BCD\u001D?h': '\u0BB7\u0BCD',
    '\u0BB8\u0BCD\u001D?h': '\u0BB6\u0BCD',
    '\u0BB4\u0BCD\u001D?h': '\u0BB4\u0BCD',
    '\u0B95\u0BCD\u001D?S': '\u0B95\u0BCD\u0BB7\u0BCD',
    '\u0B9F\u0BCD\u001D?r': '\u0BB1\u0BCD\u0BB1\u0BCD',
    '_': '\u200B',
    'M': '\u0B82',
    'H': '\u0B83',
    'a': '\u0B85',
    'A': '\u0B86',
    'i': '\u0B87',
    'I': '\u0B88',
    'u': '\u0B89',
    'U': '\u0B8A',
    'e': '\u0B8E',
    'E': '\u0B8F',
    'o': '\u0B92',
    'O': '\u0B93',
    'k': '\u0B95\u0BCD',
    'g': '\u0B95\u0BCD',
    'q': '\u0B95\u0BCD',
    'G': '\u0B95\u0BCD',
    's': '\u0B9A\u0BCD',
    'j': '\u0B9C\u0BCD',
    'J': '\u0B9C\u0BCD',
    't': '\u0B9F\u0BCD',
    'T': '\u0B9F\u0BCD',
    'd': '\u0B9F\u0BCD',
    'D': '\u0B9F\u0BCD',
    'N': '\u0BA3\u0BCD',
    'n': '\u0BA9\u0BCD',
    'p': '\u0BAA\u0BCD',
    'b': '\u0BAA\u0BCD',
    'f': '\u0BAA\u0BCD',
    'm': '\u0BAE\u0BCD',
    'y': '\u0BAF\u0BCD',
    'Y': '\u0BAF\u0BCD',
    'r': '\u0BB0\u0BCD',
    'l': '\u0BB2\u0BCD',
    'L': '\u0BB3\u0BCD',
    'v': '\u0BB5\u0BCD',
    'w': '\u0BB5\u0BCD',
    'S': '\u0BB8\u0BCD',
    'h': '\u0BB9\u0BCD',
    'z': '\u0BB4\u0BCD',
    'R': '\u0BB1\u0BCD',
    'x': '\u0B95\u0BCD\u0BB7\u0BCD',
    '([\u0B95-\u0BB9])\u001D?a': '$1\u0BBE',
    '([\u0B95-\u0BB9])\u001D?i': '$1\u0BC8',
    '([\u0B95-\u0BB9])\u001D?u': '$1\u0BCC',
    '([^\u0B80-\u0BFF]|^)\u0BA9\u0BCD\u001D?a': '$1\u0BA8',
    '([^\u0B80-\u0BFF]|^)\u0BA9\u0BCD\u001D?A': '$1\u0BA8\u0BBE',
    '([^\u0B80-\u0BFF]|^)\u0BA9\u0BCD\u001D?i': '$1\u0BA8\u0BBF',
    '([^\u0B80-\u0BFF]|^)\u0BA9\u0BCD\u001D?I': '$1\u0BA8\u0BC0',
    '([^\u0B80-\u0BFF]|^)\u0BA9\u0BCD\u001D?u': '$1\u0BA8\u0BC1',
    '([^\u0B80-\u0BFF]|^)\u0BA9\u0BCD\u001D?U': '$1\u0BA8\u0BC2',
    '([^\u0B80-\u0BFF]|^)\u0BA9\u0BCD\u001D?e': '$1\u0BA8\u0BC6',
    '([^\u0B80-\u0BFF]|^)\u0BA9\u0BCD\u001D?E': '$1\u0BA8\u0BC7',
    '([^\u0B80-\u0BFF]|^)\u0BA9\u0BCD\u001D?o': '$1\u0BA8\u0BCA',
    '([^\u0B80-\u0BFF]|^)\u0BA9\u0BCD\u001D?O': '$1\u0BA8\u0BCB',
    '\u0BA9\u0BCD\u001D?dha': '\u0BA8\u0BCD\u0BA4',
    '\u0BA9\u0BCD\u001D?dhA': '\u0BA8\u0BCD\u0BA4\u0BBE',
    '\u0BA9\u0BCD\u001D?dhi': '\u0BA8\u0BCD\u0BA4\u0BBF',
    '\u0BA9\u0BCD\u001D?dhI': '\u0BA8\u0BCD\u0BA4\u0BC0',
    '\u0BA9\u0BCD\u001D?dhu': '\u0BA8\u0BCD\u0BA4\u0BC1',
    '\u0BA9\u0BCD\u001D?dhU': '\u0BA8\u0BCD\u0BA4\u0BC2',
    '\u0BA9\u0BCD\u001D?dhe': '\u0BA8\u0BCD\u0BA4\u0BC6',
    '\u0BA9\u0BCD\u001D?dhE': '\u0BA8\u0BCD\u0BA4\u0BC7',
    '\u0BA9\u0BCD\u001D?dho': '\u0BA8\u0BCD\u0BA4\u0BCA',
    '\u0BA9\u0BCD\u001D?dhO': '\u0BA8\u0BCD\u0BA4\u0BCB',
    '([\u0B80-\u0BFF])\u0BA9\u0BCD\u001D?g': '$1\u0B99\u0BCD\u0B95\u0BCD',
    '([\u0B80-\u0BFF])\u0BA9\u0BCD\u001D?j': '$1\u0B9E\u0BCD\u0B9A\u0BCD',
    '([^\u0B80-\u0BFF]|^)\u0BA9\u0BCD\u001D?y': '$1\u0B9E\u0BCD',
    '\u0BA9\u0BCD\u001D?[dt]': '\u0BA3\u0BCD\u0B9F\u0BCD',
    '\u0BA3\u0BCD\u0B9F\u0BCD\u001D?h': '\u0BA8\u0BCD\u0BA4\u0BCD',
    '\u0BA9\u0BCD\u001D?dh': '\u0BA8\u0BCD',
    '\u0BA9\u0BCD\u001D?tr': '\u0BA9\u0BCD\u0B9F\u0BCD\u0BB0\u0BCD',
    '\u0BA3\u0BCD\u0B9F\u0BCD\u001D?r': '\u0BA9\u0BCD\u0BB1\u0BCD'
  },
  'historyPruneRegex': 't|dh|d'
};

// Load the layout and inform the keyboard to switch layout if necessary.
cros_vk_loadme(TA_PHONE_LAYOUT);
