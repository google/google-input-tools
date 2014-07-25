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
goog.provide('i18n.input.chrome.inputview.layouts.RowsOf101');

goog.require('i18n.input.chrome.inputview.layouts.util');


/**
 * Creates the top four rows for 101 keyboard.
 *
 * @return {!Array.<!Object>} The rows.
 */
i18n.input.chrome.inputview.layouts.RowsOf101.create = function() {
  var baseKeySpec = {
    'widthInWeight': 1,
    'heightInWeight': 1
  };

  // Row1
  var keySequenceOf13 = i18n.input.chrome.inputview.layouts.util.
      createKeySequence(baseKeySpec, 13);
  var backspaceKey = i18n.input.chrome.inputview.layouts.util.createKey({
    'widthInWeight': 2
  });
  var row1 = i18n.input.chrome.inputview.layouts.util.createLinearLayout({
    'id': 'row1',
    'children': [keySequenceOf13, backspaceKey]
  });

  // Row2
  var tabKey = i18n.input.chrome.inputview.layouts.util.createKey({
    'widthInWeight': 1.5
  });
  var keySequenceOf12 = i18n.input.chrome.inputview.layouts.util.
      createKeySequence(baseKeySpec, 12);
  var slashKey = i18n.input.chrome.inputview.layouts.util.createKey({
    'widthInWeight': 1.5
  });
  var row2 = i18n.input.chrome.inputview.layouts.util.createLinearLayout({
    'id': 'row2',
    'children': [tabKey, keySequenceOf12, slashKey]
  });

  // Row3
  var capslockKey = i18n.input.chrome.inputview.layouts.util.createKey({
    'widthInWeight': 1.75
  });
  var keySequenceOf11 = i18n.input.chrome.inputview.layouts.util.
      createKeySequence(baseKeySpec, 11);
  var enterKey = i18n.input.chrome.inputview.layouts.util.createKey({
    'widthInWeight': 2.25
  });
  var row3 = i18n.input.chrome.inputview.layouts.util.createLinearLayout({
    'id': 'row3',
    'children': [capslockKey, keySequenceOf11, enterKey]
  });

  // Row4
  var shiftLeftKey = i18n.input.chrome.inputview.layouts.util.createKey({
    'widthInWeight': 2.5
  });
  var keySequenceOf10 = i18n.input.chrome.inputview.layouts.util.
      createKeySequence(baseKeySpec, 10);
  var shiftRightKey = i18n.input.chrome.inputview.layouts.util.createKey({
    'widthInWeight': 2.5
  });
  var row4 = i18n.input.chrome.inputview.layouts.util.createLinearLayout({
    'id': 'row4',
    'children': [shiftLeftKey, keySequenceOf10, shiftRightKey]
  });

  return [row1, row2, row3, row4];
};
