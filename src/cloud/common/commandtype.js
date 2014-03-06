// Copyright 2014 The Cloud Input Tools Authors. All Rights Reserved.
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
 * @fileoverview The plugins supported command type.
 *
 */

goog.provide('i18n.input.common.CommandType');


/**
 * Enumeration of plugins' command types.
 *
 * @enum {string}
 */
i18n.input.common.CommandType = {
  CHANGE_STATE: 'changeState',
  CHANGE_FOCUS: 'cfx',
  CHANGE_DIRECTION: 'cd',
  TOGGLE_ITA: 'tita',
  HIDE_EDITOR: 'he',

  // For popupeditor plugin.
  // In Simplified Chinese or Traditional Chinese, numbers and punctuation has
  // two kind of shape width. One is the same as character width, called DBC.
  // Other is half of character width, called SBC. The switch from SBC or DBC.
  TOGGLE_SBC: 'tsbc',
  // The switch for inputting English or Chinese.
  TOGGLE_LANGUAGE: 'tlang',
  // In Chinese, there are two kind of punctuation. One is English punctuation,
  // other is Chinese punctuation. Such as "<" and "《"
  PUNCTUATION: 'pun',
  LOAD_CONFIG: 'lc',

  // For keyboard plugin.
  SHOW_KEYBOARD: 'sk',
  MINIMIZE_KEYBOARD: 'mk',
  LOAD_LAYOUT: 'll',

  // For keyboard and handwriting.
  REPOSITION_ELEMENT: 're',

  // For chrome os extension.
  COMMIT: 'cm',

  // Shows status bar
  SHOW_STATUSBAR: 'ss'
};
