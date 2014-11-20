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
 * @fileoverview The Decoder class provides the candidate list for an input
 * text.
 */

goog.provide('goog.ime.offline.Decoder');
goog.provide('goog.ime.offline.Response');

goog.require('goog.Disposable');
goog.require('goog.array');
goog.require('goog.events.EventHandler');
goog.require('goog.ime.offline.Candidate');
goog.require('goog.ime.offline.DataLoader');
goog.require('goog.ime.offline.EventType');
goog.require('goog.ime.offline.MLDecoder');
goog.require('goog.ime.offline.TokenDecoder');
goog.require('goog.ime.offline.UserDecoder');



/**
 * The response offline decoders provided.
 *
 * @param {Array.<string>} tokens The source tokens.
 * @param {Array.<goog.ime.offline.Candidate>} candidates The candidate list.
 * @constructor
 */
goog.ime.offline.Response = function(tokens, candidates) {
  /**
   * The source tokens with separators.
   *
   * @type {Array.<string>}
   */
  this.tokens = tokens;

  /**
   * The candidate list.
   *
   * @type {Array.<goog.ime.offline.Candidate>}
   */
  this.candidates = candidates;
};



/**
 * The offline decoder can provide a list of candidates for given source
 * text.
 *
 * @param {string} inputTool The input tool of the decoder.
 * @param {Function=} opt_callback The function called after the decoder is
 *     ready.
 * @param {Array.<string>=} opt_fuzzyPairs The fuzzy expansions.
 * @param {boolean=} opt_enableUserDict Whether to enable the user dict.
 * @constructor
 * @extends {goog.Disposable}
 */
goog.ime.offline.Decoder = function(
    inputTool, opt_callback, opt_fuzzyPairs, opt_enableUserDict) {
  /**
   * The input tool implemented by this decoder.
   *
   * @type {string}
   * @private
   */
  this.inputTool_ = inputTool;

  /**
   * The callback function when the decoder is ready.
   *
   * @type {Function}
   * @private
   */
  this.readyCallback_ = opt_callback || null;

  /**
   * The data loader.
   *
   * @type {goog.ime.offline.DataLoader}
   * @private
   */
  this.dataLoader_ = new goog.ime.offline.DataLoader(inputTool);
  this.dataLoader_.loadModelData(goog.bind(this.onDecoderReady_, this));

  /**
   * The token decoder.
   *
   * @type {goog.ime.offline.TokenDecoder}
   * @private
   */
  this.tokenDecoder_ = new goog.ime.offline.TokenDecoder(
      inputTool, this.dataLoader_, opt_fuzzyPairs);

  /**
   * The machine learning based decoder.
   *
   * @type {goog.ime.offline.MLDecoder}
   * @private
   */
  this.mlDecoder_ = new goog.ime.offline.MLDecoder(
      inputTool, this.dataLoader_);

  /**
   * The user dictionary decoder.
   *
   * @type {goog.ime.offline.UserDecoder}
   * @private
   */
  this.userDecoder_ = opt_enableUserDict ?
      new goog.ime.offline.UserDecoder(inputTool) : null;

  /**
   * The event handler.
   *
   * @type {!goog.events.EventHandler}
   * @private
   */
  this.eventHandler_ = new goog.events.EventHandler(this);
  this.eventHandler_.listen(
      this.tokenDecoder_, goog.ime.offline.EventType.CLEAR,
      this.clear);
};
goog.inherits(goog.ime.offline.Decoder, goog.Disposable);


/** @override */
goog.ime.offline.Decoder.prototype.disposeInternal = function() {
  this.dataLoader_.dispose();
};


/**
 * Whether the decoder is ready.
 *
 * @return {boolean} True if the machine learning decoder is ready.
 */
goog.ime.offline.Decoder.prototype.isReady = function() {
  return this.dataLoader_.dataReady;
};


/**
 * Gets the transliterations (without scores) for the source word.
 *
 * @param {string} sourceWord The source word.
 * @param {number} resultsNum The num of the expected results.
 * @return {goog.ime.offline.Response} The result of the decode process,
 * including the tokens and the candidates.
 */
goog.ime.offline.Decoder.prototype.decode = function(sourceWord, resultsNum) {
  if (!this.isReady() || !sourceWord) {
    return null;
  }

  // Finds the shortest token list.
  var tokenPath = this.tokenDecoder_.getBestTokens(sourceWord);
  if (!tokenPath) {
    return null;
  }

  var normalizedTokens = this.tokenDecoder_.getNormalizedTokens(
      tokenPath.tokens);
  var isAllInitials = this.tokenDecoder_.isAllInitials(tokenPath.tokens);
  var translits = this.mlDecoder_.transliterate(
      normalizedTokens, resultsNum, isAllInitials);

  // Filtering the translits.
  var candidates = [];
  var targetWords = [];
  for (var i = 0; i < translits.length; ++i) {
    var translit = translits[i];
    if (targetWords.indexOf(translit.target) < 0) {
      candidates.push(translit);
      targetWords.push(translit.target);
      if (candidates.length >= resultsNum) {
        break;
      }
    }
  }

  // Adds the candidate from user dictionary at the second position.
  if (this.userDecoder_) {
    var userCommitCandidate = this.userDecoder_.getCandidate(sourceWord);
    if (userCommitCandidate) {
      var index = goog.array.findIndex(candidates, function(candidate) {
        return candidate.target == userCommitCandidate;
      });

      if (index > 0) {
        goog.array.removeAt(candidates, index);
      }

      if (index != 0) {
        var candidate = new goog.ime.offline.Candidate(
            sourceWord.length, userCommitCandidate, 0);
        candidates.splice(1, 0, candidate);
      }
    }
  }

  // Also returns the token list.
  var originalTokenList = this.tokenDecoder_.getOriginalTokens(tokenPath);
  return new goog.ime.offline.Response(originalTokenList, candidates);
};


/**
 * The callback function when the language model is ready.
 *
 * @private
 */
goog.ime.offline.Decoder.prototype.onDecoderReady_ = function() {
  if (this.readyCallback_) {
    this.readyCallback_();
  }
};


/**
 * Adds user selected candidates.
 *
 * @param {string} source The source text.
 * @param {string} target The target text commits.
 */
goog.ime.offline.Decoder.prototype.addUserCommits = function(source, target) {
  if (this.userDecoder_) {
    this.userDecoder_.add(source, target);
  }
};


/**
 * Persists the user dictionary.
 */
goog.ime.offline.Decoder.prototype.persist = function() {
  if (this.userDecoder_) {
    this.userDecoder_.persist();
  }
};


/**
 * Clear the decoder.
 */
goog.ime.offline.Decoder.prototype.clear = function() {
  this.tokenDecoder_.clear();
  this.mlDecoder_.clear();
};


/**
 * Updates fuzzy paris.
 *
 * @param {Array.<string>} fuzzyPairs The fuzzy expansions.
 */
goog.ime.offline.Decoder.prototype.updateFuzzyPairs = function(
    fuzzyPairs) {
  this.tokenDecoder_.updateFuzzyPairs(fuzzyPairs);
};


/**
 * Enables/Disables the user dictionary.
 *
 * @param {boolean} enable True, if the user dictionary should be enabled.
 */
goog.ime.offline.Decoder.prototype.enableUserDict = function(enable) {
  if (enable && !this.userDecoder_) {
    this.userDecoder_ = new goog.ime.offline.UserDecoder(this.inputTool_);
  }

  if (!enable) {
    this.userDecoder_ = null;
  }
};
