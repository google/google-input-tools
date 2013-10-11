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
 * @fileoverview VK layout definition for Tamil typewriter.
 */

var TA_TYPEWRITER_LAYOUT = {
  'id': 'ta_typewriter',
  'title': '\u0BA4\u0BAE\u0BBF\u0BB4\u0BCD (TYPE-WRITER)',
  'mappings': {
    ',sl': {
      '': '\u0b831234567890/=' +
          '{{\u0ba3\u0bc1}}\u0bb1\u0ba8\u0b9a\u0bb5\u0bb2\u0bb0\u0bc8' +
          '{{\u0b9f\u0bbf}}\u0bbf\u0bc1\u0bb9{{\u0b95\u0bcd\u0bb7}}' +
          '\u0baf\u0bb3\u0ba9\u0b95\u0baa\u0bbe\u0ba4\u0bae\u0b9f\u0bcd\u0b99' +
          '\u0ba3\u0b92\u0b89\u0b8e\u0bc6\u0bc7\u0b85\u0b87,.'
    },
    's,l': {
      '': '\'\u0bb8"%\u0b9c\u0bb6\u0bb7{{}}{{}}(){{\u0bb8\u0bcd\u0bb0\u0bc0}}' +
          '+{{}}{{\u0bb1\u0bc1}}{{\u0ba8\u0bc1}}{{\u0ba9\u0bc1}}' +
          '{{\u0b95\u0bc2}}{{\u0bb2\u0bc1}}{{\u0bb0\u0bc1}}\u0b90' +
          '{{\u0b9f\u0bc0}}\u0bc0\u0bc2\u0bcc\u0bf8' +
          '{{}}{{\u0bb3\u0bc1}}{{\u0ba9\u0bc1}}{{\u0b95\u0bc1}}' +
          '{{\u0bb4\u0bc1}}\u0bb4{{\u0ba4\u0bc1}}{{\u0bae\u0bc1}}' +
          '{{\u0b9f\u0bc1}}\\\u0b9e' +
          '\u0bb7\u0b93\u0b8a\u0b8f{{\u0b95\u0bcd\u0bb7}}{{\u0b9a\u0bc2}}' +
          '\u0b86\u0b88?-'
    }
  }
};

// Loads the layout and inform the keyboard to switch layout if necessary.
cros_vk_loadme(TA_TYPEWRITER_LAYOUT);
