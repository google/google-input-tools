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
 * @fileoverview VK layout definition for Tamil ta99.
 */

var TA_TAMIL99_LAYOUT = {
  'id': 'ta_tamil99',
  'title': '\u0BA4\u0BAE\u0BBF\u0BB4\u0BCD (TAMIL99)',
  'mappings': {
    ',c': {
      '': '`1234567890-=' +
          '\u0B86\u0B88\u0B8A\u0B90\u0B8F\u0BB3\u0BB1\u0BA9\u0B9F\u0BA3\u0B9A' +
          '\u0B9E\\' +
          '\u0B85\u0B87\u0B89\u0BCD\u0B8E\u0B95\u0BAA\u0BAE\u0BA4\u0BA8\u0BAF' +
          '\u0B94\u0B93\u0B92\u0BB5\u0B99\u0BB2\u0BB0,.\u0BB4'
    },
    's,sc': {
      '': '~!@#$%^&*()_+' +
          '\u0BB8\u0BB7\u0B9C\u0BB9{{\u0BB8\u0BCD\u0BB0\u0BC0}}' +
          '{{\u0B95\u0BCD\u0BB7}}{{}}{{}}[]{}|' +
          '\u0BF9\u0BFA\u0BF8\u0B83{{}}{{}}{{}}":;\'' +
          '\u0BF3\u0BF4\u0BF5\u0BF6\u0BF7{{}}/<>?'
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
  'transform': {
    '([\u0B95-\u0BB9])\u0B85': '$1\u200D',
    '([\u0B95-\u0BB9])\u0B86': '$1\u0BBE',
    '([\u0B95-\u0BB9])\u0B87': '$1\u0BBF',
    '([\u0B95-\u0BB9])\u0B88': '$1\u0BC0',
    '([\u0B95-\u0BB9])\u0B89': '$1\u0BC1',
    '([\u0B95-\u0BB9])\u0B8A': '$1\u0BC2',
    '([\u0B95-\u0BB9])\u0B8E': '$1\u0BC6',
    '([\u0B95-\u0BB9])\u0B8F': '$1\u0BC7',
    '([\u0B95-\u0BB9])\u0B90': '$1\u0BC8',
    '([\u0B95-\u0BB9])\u0B92': '$1\u0BCA',
    '([\u0B95-\u0BB9])\u0B93': '$1\u0BCB',
    '([\u0B95-\u0BB9])\u0B94': '$1\u0BCC'
  }

};

// Load the layout and inform the keyboard to switch layout if necessary. cibu
cros_vk_loadme(TA_TAMIL99_LAYOUT);
