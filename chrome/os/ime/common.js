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
 * @fileoverview Defines the eventtype, key event value, model status.
 */

goog.provide('goog.ime.chrome.os.EventType');
goog.provide('goog.ime.chrome.os.Key');
goog.provide('goog.ime.chrome.os.KeyboardLayouts');
goog.provide('goog.ime.chrome.os.MessageKey');
goog.provide('goog.ime.chrome.os.Modifier');
goog.provide('goog.ime.chrome.os.StateID');
goog.provide('goog.ime.chrome.os.Status');
goog.provide('goog.ime.chrome.os.TransID');


/**
 * The EventType for event in chrome os extension.
 *
 * @enum {string}
 */
goog.ime.chrome.os.EventType = {
  KEYDOWN: 'keydown',
  KEYUP: 'keyup',
  COMMIT: 'commit',
  MODELUPDATED: 'update',
  CLOSING: 'close',
  OPENING: 'open'
};


/**
 * The key value for chrome os extension.
 *
 * @enum {string}
 */
goog.ime.chrome.os.Key = {
  UP: 'Up',
  DOWN: 'Down',
  PAGE_UP: 'Page_up',
  PAGE_DOWN: 'Page_down',
  SPACE: ' ',
  ENTER: 'Enter',
  BACKSPACE: 'Backspace',
  ESC: 'Esc',
  LEFT: 'Left',
  RIGHT: 'Right',
  INVALID: '\ufffd'
};


/**
 * The modifiers for chrome os extension.
 *
 * @enum {string}
 */
goog.ime.chrome.os.Modifier = {
  SHIFT: 'Shift',
  CTRL: 'Control',
  ALT: 'Alt'
};


/**
 * The keyboard layouts for zhuyin.
 *
 * @enum {string}
 */
goog.ime.chrome.os.KeyboardLayouts = {
  STANDARD: 'Default',
  GINYIEH: 'Gin Yieh',
  ETEN: 'Eten',
  IBM: 'IBM',
  HSU: 'Hsu',
  ETEN26: 'Eten 26'
};


/**
 * The input method model status.
 *
 * @enum {number}
 */
goog.ime.chrome.os.Status = {
  INIT: 0,
  FETCHING: 1,
  FETCHED: 2,
  SELECT: 3
};


/**
 * The states ids.
 *
 * @enum {string}
 */
goog.ime.chrome.os.StateID = {
  LANG: 'lang',
  SBC: 'sbc',
  PUNC: 'punc'
};


/**
 * The key for the transform result.
 *
 * @enum {number}
 */
goog.ime.chrome.os.TransID = {
  BACK: 0,
  TEXT: 1,
  INSTANT: 2
};


/**
 * The message keys.
 *
 * @enum {string}
 */
goog.ime.chrome.os.MessageKey = {
  SOURCE: 'source',
  HIGHLIGHT: 'highlight',
  APPEND: 'append',
  DELETE: 'delete',
  REVERT: 'revert',
  CLEAR: 'clear',
  IME: 'ime',
  SELECT_HIGHLIGHT: 'select_highlight',
  SELECT: 'select',
  COMMIT: 'commit',
  MULTI: 'multi',
  FUZZY_PAIRS: 'fuzzy_pairs',
  USER_DICT: 'user_dict',
  COMMIT_MARK: '|'
};
