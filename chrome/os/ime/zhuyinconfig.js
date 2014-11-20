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
 * @fileoverview Defines the zhuyin model configs.
 */
goog.provide('goog.ime.chrome.os.ZhuyinConfig');

goog.require('goog.ime.chrome.os.ChineseConfig');
goog.require('goog.ime.chrome.os.KeyboardLayouts');



/**
 * The input method config.
 *
 * @constructor
 * @extends {goog.ime.chrome.os.ChineseConfig}
 */
goog.ime.chrome.os.ZhuyinConfig = function() {
  goog.base(this);

  /**
   * The bopomofo map.
   *
   * @type {Object.<string, Object.<string, string> >}
   * @private
   */
  this.bopomofoMap_ = {};

  /**
   * The vows in zhuyin.
   *
   * @type {!RegExp}
   * @private
   */
  this.vow_ = /[\u3110-\u3129]/;

  /**
   * The tons in zhuyin, which can only append after a vow.
   *
   * @type {!RegExp}
   * @private
   */
  this.ton_ = /[\u02c7\u02c9-\u02cb\u02d9]/;

  this.init_();
};
goog.inherits(goog.ime.chrome.os.ZhuyinConfig,
    goog.ime.chrome.os.ChineseConfig);


/**
 * The phonetic characters for zhuyin.
 *
 * @enum {string}
 */
goog.ime.chrome.os.ZhuyinConfig.Phonetics = {
  B: '\u3105',
  P: '\u3106',
  M: '\u3107',
  F: '\u3108',
  D: '\u3109',
  T: '\u310a',
  N: '\u310b',
  L: '\u310c',
  K: '\u310e',
  G: '\u310d',
  H: '\u310f',
  J: '\u3110',
  Q: '\u3111',
  X: '\u3112',
  ZH: '\u3113',
  CH: '\u3114',
  SH: '\u3115',
  R: '\u3116',
  Z: '\u3117',
  C: '\u3118',
  S: '\u3119',
  I: '\u3127',
  U: '\u3128',
  UE: '\u3129',
  A: '\u311a',
  O: '\u311b',
  ER: '\u311c',
  E: '\u311d',
  AI: '\u311e',
  EI: '\u311f',
  AO: '\u3120',
  OU: '\u3121',
  AN: '\u3122',
  EN: '\u3123',
  ANG: '\u3124',
  ENG: '\u3125',
  ERR: '\u3126',
  TONE1: '\u02c9',
  TONE2: '\u02ca',
  TONE3: '\u02c7',
  TONE4: '\u02cb',
  TONE5: '\u02d9'
};


/**
 * Initialize the Zhuyin config.
 *
 * @private
 */
goog.ime.chrome.os.ZhuyinConfig.prototype.init_ = function() {
  var layouts = goog.ime.chrome.os.KeyboardLayouts;
  var Phonetics = goog.ime.chrome.os.ZhuyinConfig.Phonetics;
  this.bopomofoMap_[layouts.STANDARD] = {
    '1': Phonetics.B,
    'q': Phonetics.P,
    'a': Phonetics.M,
    'z': Phonetics.F,
    '2': Phonetics.D,
    'w': Phonetics.T,
    's': Phonetics.N,
    'x': Phonetics.L,
    'e': Phonetics.G,
    'd': Phonetics.K,
    'c': Phonetics.H,
    'r': Phonetics.J,
    'f': Phonetics.Q,
    'v': Phonetics.X,
    '5': Phonetics.ZH,
    't': Phonetics.CH,
    'g': Phonetics.SH,
    'b': Phonetics.R,
    'y': Phonetics.Z,
    'h': Phonetics.C,
    'n': Phonetics.S,
    'u': Phonetics.I,
    'j': Phonetics.U,
    'm': Phonetics.UE,
    '8': Phonetics.A,
    'i': Phonetics.O,
    'k': Phonetics.ER,
    ',': Phonetics.E,
    '9': Phonetics.AI,
    'o': Phonetics.EI,
    'l': Phonetics.AO,
    '.': Phonetics.OU,
    '0': Phonetics.AN,
    'p': Phonetics.EN,
    ';': Phonetics.ANG,
    '/': Phonetics.ENG,
    '-': Phonetics.ERR,
    '3': Phonetics.TONE3,
    '4': Phonetics.TONE4,
    '6': Phonetics.TONE2,
    '7': Phonetics.TONE5,
    ' ': Phonetics.TONE1,
    '=': '='
  };

  this.bopomofoMap_[layouts.IBM] = {
    '1': Phonetics.B,
    '2': Phonetics.P,
    '3': Phonetics.M,
    '4': Phonetics.F,
    '5': Phonetics.D,
    '6': Phonetics.T,
    '7': Phonetics.N,
    '8': Phonetics.L,
    '9': Phonetics.G,
    '0': Phonetics.K,
    '-': Phonetics.H,
    'q': Phonetics.J,
    'w': Phonetics.Q,
    'e': Phonetics.X,
    'r': Phonetics.ZH,
    't': Phonetics.CH,
    'y': Phonetics.SH,
    'u': Phonetics.R,
    'i': Phonetics.Z,
    'o': Phonetics.C,
    'p': Phonetics.S,
    'a': Phonetics.I,
    's': Phonetics.U,
    'd': Phonetics.UE,
    'f': Phonetics.A,
    'g': Phonetics.O,
    'h': Phonetics.ER,
    'j': Phonetics.E,
    'k': Phonetics.AI,
    'l': Phonetics.EI,
    ';': Phonetics.AO,
    'z': Phonetics.OU,
    'x': Phonetics.AN,
    'c': Phonetics.EN,
    'v': Phonetics.ANG,
    'b': Phonetics.ENG,
    'n': Phonetics.ERR,
    ' ': Phonetics.TONE1,
    'm': Phonetics.TONE2,
    ',': Phonetics.TONE3,
    '.': Phonetics.TONE4,
    '/': Phonetics.TONE5
  };

  this.bopomofoMap_[layouts.ETEN] = {
    'b': Phonetics.B,
    'p': Phonetics.P,
    'm': Phonetics.M,
    'f': Phonetics.F,
    'd': Phonetics.D,
    't': Phonetics.T,
    'n': Phonetics.N,
    'l': Phonetics.L,
    'v': Phonetics.G,
    'k': Phonetics.K,
    'h': Phonetics.H,
    'g': Phonetics.J,
    '7': Phonetics.Q,
    'c': Phonetics.X,
    ',': Phonetics.ZH,
    '.': Phonetics.CH,
    '/': Phonetics.SH,
    'j': Phonetics.R,
    ';': Phonetics.Z,
    '\'': Phonetics.C,
    's': Phonetics.S,
    'e': Phonetics.I,
    'x': Phonetics.U,
    'u': Phonetics.UE,
    'a': Phonetics.A,
    'o': Phonetics.O,
    'r': Phonetics.ER,
    'w': Phonetics.E,
    'i': Phonetics.AI,
    'q': Phonetics.EI,
    'z': Phonetics.AO,
    'y': Phonetics.OU,
    '8': Phonetics.AN,
    '9': Phonetics.EN,
    '0': Phonetics.ANG,
    '-': Phonetics.ENG,
    '=': Phonetics.ERR,
    ' ': Phonetics.TONE1,
    '2': Phonetics.TONE2,
    '3': Phonetics.TONE3,
    '4': Phonetics.TONE4,
    '1': Phonetics.TONE5
  };

  this.bopomofoMap_[layouts.GINYIEH] = {
    '\'': Phonetics.UE,
    ',': Phonetics.E,
    '-': Phonetics.I,
    '.': Phonetics.OU,
    '/': Phonetics.ENG,
    '0': Phonetics.AN,
    '1': Phonetics.TONE5,
    '2': Phonetics.B,
    '3': Phonetics.D,
    '6': Phonetics.ZH,
    '8': Phonetics.A,
    '9': Phonetics.AI,
    ';': Phonetics.ANG,
    '=': Phonetics.ERR,
    '[': Phonetics.U,
    'a': Phonetics.TONE3,
    'b': Phonetics.X,
    'c': Phonetics.L,
    'd': Phonetics.N,
    'e': Phonetics.T,
    'f': Phonetics.K,
    'g': Phonetics.Q,
    'h': Phonetics.SH,
    'i': Phonetics.O,
    'j': Phonetics.C,
    'k': Phonetics.ER,
    'l': Phonetics.AO,
    'm': Phonetics.S,
    'n': Phonetics.R,
    'o': Phonetics.EI,
    'p': Phonetics.EN,
    'q': Phonetics.TONE2,
    'r': Phonetics.G,
    's': Phonetics.M,
    't': Phonetics.J,
    'u': Phonetics.Z,
    'v': Phonetics.H,
    'w': Phonetics.P,
    'x': Phonetics.F,
    'y': Phonetics.CH,
    'z': Phonetics.TONE4
  };

  this.bopomofoMap_[layouts.HSU] = {
    'b': Phonetics.B,
    'p': Phonetics.P,
    'm': Phonetics.M + Phonetics.AN,
    'f': Phonetics.F + Phonetics.TONE3,
    'd': Phonetics.D + Phonetics.TONE2,
    't': Phonetics.T,
    'n': Phonetics.N + Phonetics.EN,
    'l': Phonetics.L + Phonetics.ENG + Phonetics.ERR,
    'g': Phonetics.G + Phonetics.ER,
    'k': Phonetics.K + Phonetics.ANG,
    'h': Phonetics.H + Phonetics.O,
    'j': Phonetics.J + Phonetics.ZH + Phonetics.TONE4,
    'v': Phonetics.Q + Phonetics.CH,
    'c': Phonetics.X + Phonetics.SH,
    'r': Phonetics.R,
    'z': Phonetics.Z,
    'a': Phonetics.C + Phonetics.EI,
    's': Phonetics.S + Phonetics.TONE5,
    'e': Phonetics.I + Phonetics.E,
    'x': Phonetics.U,
    'u': Phonetics.UE,
    'y': Phonetics.A,
    'i': Phonetics.AI,
    'w': Phonetics.AO,
    'o': Phonetics.OU
  };

  this.bopomofoMap_[layouts.ETEN26] = {
    'b': Phonetics.B,
    'p': Phonetics.P + Phonetics.OU,
    'm': Phonetics.M + Phonetics.AN,
    'f': Phonetics.F + Phonetics.TONE2,
    'd': Phonetics.D + Phonetics.TONE5,
    't': Phonetics.T + Phonetics.ANG,
    'n': Phonetics.N + Phonetics.EN,
    'l': Phonetics.L + Phonetics.ENG,
    'v': Phonetics.G + Phonetics.Q,
    'k': Phonetics.K + Phonetics.TONE4,
    'h': Phonetics.H + Phonetics.ERR,
    'g': Phonetics.ZH + Phonetics.J,
    'c': Phonetics.SH + Phonetics.X,
    'y': Phonetics.CH,
    'j': Phonetics.R + Phonetics.TONE3,
    'q': Phonetics.Z + Phonetics.EI,
    'w': Phonetics.C + Phonetics.E,
    's': Phonetics.S,
    'e': Phonetics.I,
    'x': Phonetics.U,
    'u': Phonetics.UE,
    'a': Phonetics.A,
    'o': Phonetics.O,
    'r': Phonetics.ER,
    'i': Phonetics.AI,
    'z': Phonetics.AO
  };

  this.editorCharReg = /[a-z0-9\-;,.\/ ]/;
  this.punctuationReg = /xyz/;
  this.autoHighlight = false;
};


/** @override */
goog.ime.chrome.os.ZhuyinConfig.prototype.transform = function(context, c) {
  var trans = this.bopomofoMap_[this.layout][c];
  if (!trans) {
    return '';
  }

  if (trans.length == 1 && trans.match(this.ton_)) {
    var prefix = context.charAt(context.length - 1);
    if (!prefix.match(this.vow_)) {
      return '';
    }
  }

  return trans;
};
