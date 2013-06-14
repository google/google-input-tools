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
 * @fileoverview VK layout definition for Persian.
 */

var FA_LAYOUT = {
  'id': 'fa',
  'title': '\u0641\u0627\u0631\u0633\u06cc',
  'direction': 'rtl',
  'mappings': {
    ',l': {
      '': '\u200D\u06F1\u06F2\u06F3\u06F4\u06F5\u06F6\u06F7\u06F8\u06F9' +
          '\u06F0-=' +
          '\u0636\u0635\u062B\u0642\u0641\u063A\u0639\u0647\u062E\u062D' +
          '\u062C\u0686\\' +
          '\u0634\u0633\u06CC\u0628\u0644\u0627\u062A\u0646\u0645\u06A9' +
          '\u06AF' +
          '\u0638\u0637\u0632\u0631\u0630\u062F\u067E\u0648./'
    },
    's,sl': {
      '123': '!\u066C\u066B',
      '4': '\u0631\u06CC\u0627\u0644',
      '567890m=': '\u066A\u00D7\u060C*)(\u0640+',
      'QWERTYUIOP\u00db\u00dd\u00dc': '\u0652\u064C\u064D\u064B\u064F\u0650' +
          '\u064E\u0651][}{|',
      'ASDFGHJKL;\u00de': '\u0624\u0626\u064A\u0625\u0623\u0622\u0629\u00BB' +
          '\u00AB:\u061B',
      'Z': '\u0643',
      'C': '\u0698',
      'B': '\u200C',
      'M\u00bc\u00be\u00bf': '\u0621><\u061F'
    },
    'c': {
      '\u00c01234567890m=': '\u007E\u0060\u0040\u0023\u0024\u0025\u005E\u0026' +
          '\u2022\u200E\u200F\u005F\u2212',
      'QEIOP\u00db\u00dd\u00dc': '\u00B0\u20AC\u202D\u202E\u202C\u202A\u202B' +
          '\u2010',
      'DHKL;\u00de': '\u0649\u0671\uFD3E\uFD3F\u003B\u0022',
      'VBNM\u00bc\u00be\u00bf': '\u0656\u200D\u0655\u2026\u002C\u0027\u003F',
      'Z': '\u00A0'  // In Persian spec, this should be 'AltGr + Space'.
    }
  }
};

// Load the layout and inform the keyboard to switch layout if necessary.
cros_vk_loadme(FA_LAYOUT);
