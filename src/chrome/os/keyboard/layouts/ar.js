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
 * @fileoverview VK layout definition for Arabic keyboard.
 */

var AR_LAYOUT = {
  'id': 'ar',
  'title': '\u0644\u0648\u062d\u0629 ' +
      '\u0645\u0641\u0627\u062a\u064a\u062d ' +
      '\u0627\u0644\u0644\u063a\u0629 ' +
      '\u0627\u0644\u0639\u0631\u0628\u064a\u0629',
  'direction': 'rtl',
  'mappings': {
    'scl,sc,sl,s': {
      '\u00c01234567890m=': '\u0651!@#$%^&*)(_+',
      'QWER': '\u064e\u064b\u064f\u064c',
      'T': '\u0644\u0625',
      'YUIOP\u00db\u00dd\u00dc': '\u0625\u2018\u00f7\u00d7\u061b<>|',
      'ASDF': '\u0650\u064d][',
      'G': '\u0644\u0623',
      'HJKL;\u00de': '\u0623\u0640\u060c\"',
      'ZXCV': '~\u0652}{',
      'B': '\u0644\u0622',
      'NM\u00bc\u00be\u00bf': '\u0622\u2019,.\u061f'
    },
    'cl,l,c,': {
      '\u00c01234567890m=':
      '\u0630\u0661\u0662\u0663\u0664\u0665\u0666\u0667\u0668\u0669\u0660-=',
      'QWERTYUIOP\u00db\u00dd\u00dc':
      '\u0636\u0635\u062b\u0642\u0641\u063a' +
      '\u0639\u0647\u062e\u062d\u062c\u062f\\',
      'ASDFGHJKL;\u00de':
      '\u0634\u0633\u064a\u0628\u0644\u0627\u062a\u0646\u0645\u0643\u0637',
      'ZXCV': '\u0626\u0621\u0624\u0631',
      'B': '\u0644\u0627',
      'NM\u00bc\u00be\u00bf': '\u0649\u0629\u0648\u0632\u0638'
    }
  }
};

// Load the layout and inform the keyboard to switch layout if necessary.
cros_vk_loadme(AR_LAYOUT);
