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
 * @fileoverview goog.ime.offline.DataLoader is implemented to load required
 * data in offline transliterator.
 */
goog.provide('goog.ime.offline.DataLoader');

goog.require('goog.Disposable');
goog.require('goog.ime.offline.InputToolCode');



/**
 * goog.ime.offline.DataLoader provides the functions to load token
 * dictionary, generation model and dictionary for the offline transliterator.
 *
 * @param {string} inputTool The input tool code.
 * @constructor
 * @extends {goog.Disposable}
 */
goog.ime.offline.DataLoader = function(inputTool) {
  /**
   * The current input tool.
   * @type {string}
   */
  this.inputTool = inputTool;

  /**
   * The source map for the model data.
   *
   * @type {Object}
   */
  this.sourceMap;

  /**
   * The target map for the model data.
   *
   * @type {Array}
   */
  this.targetMap;

  /**
   * The number used to normalize probabilities.
   *
   * @type {Object}
   */
  this.defaultProb;

  /**
   * The encoded source segments.
   *
   * @type {Array}
   */
  this.sourceSegments;

  /**
   * The encoded target segments.
   *
   * @type {Array}
   */
  this.targetSegments;

  /**
   * The probabilities for target segments. The scores for the corresponding
   * target segemnts in this.targetSegments.
   *
   * @type {Array.<*>}
   */
  this.targetProbs;

  /**
   * The offsets of the first target segment for all source segments.
   *
   * @type {Array.<*>}
   */
  this.targetPositions;

  /**
   * The full tokens for the decoder, separated by '|'.
   *
   * @type {string}
   */
  this.tokens;

  /**
   * The tokens without tones, separated by '|'.
   *
   * @type {string}
   */
  this.untoneTokens;

  /**
   * The initial tones, separated by '|'.
   *
   * @type {string}
   */
  this.initialTokens = '';

  /**
   * Whether the data is ready now.
   *
   * @type {boolean}
   */
  this.dataReady = false;
};
goog.inherits(goog.ime.offline.DataLoader, goog.Disposable);


/**
 * The variables from the model data.
 *
 * @enum {string}
 */
goog.ime.offline.DataLoader.PARAMS = {
  INPUT_TOOL: 'chosInputTool',
  SOURCE_MAP: 'sourceMap',
  TARGET_MAP: 'targetMap',
  DEFAULT_PROB: 'defaultProb',
  SOURCE_SEGS: 'sourceSegments',
  TARGET_SEGS: 'targetSegments',
  SOURCE_PROS: 'sourceProbs',
  TARGET_PROS: 'targetProbs',
  TARGET_POS: 'targetPositions',
  TOKENS: 'chosTokens'
};


/**
 * Loads the model data.
 *
 * @param {Function} callBackFn The function to call when the data is loaded.
 */
goog.ime.offline.DataLoader.prototype.loadModelData = function(callBackFn) {
  if (this.dataReady) {
    callBackFn();
    return;
  }

  if (window[goog.ime.offline.DataLoader.PARAMS.INPUT_TOOL] == this.inputTool) {
    this.loadModelDataInternal_();
    this.dataReady = true;
    callBackFn();
    return;
  }
};


/**
 * Loads the model data internal.
 *
 * @private
 */
goog.ime.offline.DataLoader.prototype.loadModelDataInternal_ = function() {
  var param = goog.ime.offline.DataLoader.PARAMS;
  this.sourceMap = window[param.SOURCE_MAP];
  this.targetMap = window[param.TARGET_MAP];
  this.defaultProb = window[param.DEFAULT_PROB];
  this.tokens = window[param.TOKENS];

  this.sourceSegments = window[param.SOURCE_SEGS];
  this.targetSegments = window[param.TARGET_SEGS];
  this.targetProbs = window[param.TARGET_PROS];
  this.targetPositions = window[param.TARGET_POS];

  this.buildTokens_();
  this.dataReady = true;
};


/**
 * Builds the untone tokens and initial tokens.
 *
 * @private
 */
goog.ime.offline.DataLoader.prototype.buildTokens_ = function() {
  var code = goog.ime.offline.InputToolCode;
  if (this.inputTool == code.INPUTMETHOD_ZHUYIN_CHINESE_TRADITIONAL) {
    var untoneTokens = [];
    var fullTokens = this.tokens.split('|');
    for (var i = 0; i < fullTokens.length; ++i) {
      var untoneToken = fullTokens[i].slice(0, fullTokens[i].length - 1);
      if (untoneTokens.indexOf(untoneToken) < 0) {
        untoneTokens.push(untoneToken);
      }
    }
    this.untoneTokens = untoneTokens.join('|');

    var initialTokens = [];
    for (var i = 0x3105; i <= 0x3119; ++i) {
      initialTokens.push(String.fromCharCode(i));
    }
    this.initialTokens = initialTokens.join('|');
  } else if (this.inputTool == code.INPUTMETHOD_PINYIN_CHINESE_SIMPLIFIED ||
      this.inputTool == code.INPUTMETHOD_PINYIN_CHINESE_TRADITIONAL) {
    this.initialTokens = 'b|p|m|f|d|t|n|l|k|g|h|j|q|x|zh|ch|sh|r|z|c|s|y|w';
  }
};
