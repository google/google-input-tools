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
 * @fileoverview VK layout definition for Tamil inscript.
 */

var TA_INSCRIPT_LAYOUT = {
  'id': 'ta_inscript',
  'title': '\u0BA4\u0BAE\u0BBF\u0BB4\u0BCD (INSCRIPT)',
  'mappings': {
    'sl,s': {
      '\u00c012': '\u0b92\u0b8d\u0bc5',
      '3': '\u0bcd\u0bb0',
      '4': '\u0bb0\u0bcd',
      '5': '\u0b9c\u0bcd\u0b9e',
      '6': '\u0ba4\u0bcd\u0bb0',
      '7': '\u0b95\u0bcd\u0bb7',
      '8': '\u0bb6\u0bcd\u0bb0',
      '90m=': '()\u0b83\u0b8b',
      'QWERTYUIOP\u00db\u00dd\u00dc':
          '\u0b94\u0b90\u0b86\u0b88\u0b8a\u0bad\u0b99\u0b98\u0ba7\u0b9d' +
          '\u0ba2\u0b9e\u0b91',
      'ASDFGHJKL;\u00de':
          '\u0b93\u0b8f\u0b85\u0b87\u0b89\u0bab\u0bb1\u0b96\u0ba5\u0b9b\u0ba0',
      'ZXCVBNM\u00bc\u00be\u00bf':
          '\u0b8e\u0b81\u0ba3\u0ba9\u0bb4\u0bb3\u0bb6\u0bb7\u0be4\u0bdf'
    },
    'scl,sc,cl,c': {
      '1234567890=':
          '\u0be7\u0be8\u0be9\u0bea\u0beb\u0bec\u0bed\u0bee\u0bef\u0be6\u0bc4',
      'R': '\u0be3',
      'I': '\u0bda',
      'P': '\u0bdb',
      '\u00db': '\u0bdc',
      'F': '\u0be2',
      'K': '\u0bd8',
      ';': '\u0bd2' ,
      'Z': '\u0bd3',
      'C': '\u0bd4',
      '\u00bc': '\u0bf0',
      '\u00be': '\u0be5'
    },
    'scl,sc': {
      '=': '\u0be0',
      'R': '\u0be1',
      '\u00db': '\u0bdd' ,
      'F': '\u0b8c',
      'H': '\u0bde',
      'K': '\u0bd9',
      '\u00de': '\u0bd1',
      'X': '\u0bd0',
      '\u00be': '\u0bbd'
    },
    'l,': {
      '':
          '\u0bca1234567890-\u0bc3' +
          '\u0bcc\u0bc8\u0bbe\u0bc0\u0bc2\u0bac\u0bb9\u0b97\u0ba6\u0b9c' +
          '\u0ba1\u0bbc\u0bc9' +
          '\u0bcb\u0bc7\u0bcd\u0bbf\u0bc1\u0baa\u0bb0\u0b95\u0ba4\u0b9a\u0b9f' +
          '\u0bc6\u0b82\u0bae\u0ba8\u0bb5\u0bb2\u0bb8,.\u0baf'
    }
  }
};

cros_vk_loadme(TA_INSCRIPT_LAYOUT);
