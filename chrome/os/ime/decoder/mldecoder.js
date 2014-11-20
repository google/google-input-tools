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
 * @fileoverview The MLDecoder class computes transliterations for a
 * given source language word in the target language using a machine learning
 * based approach, which relies on learnt probabilities for source -> target
 * language segment mappings.
 */

goog.provide('goog.ime.offline.MLDecoder');

goog.require('goog.ime.offline.Candidate');
goog.require('goog.ime.offline.DataLoader');
goog.require('goog.ime.offline.DataParser');
goog.require('goog.ime.offline.Heap');



/**
 * The goog.ime.offline.DataParser implements the machine learning based
 * transliterator.
 *
 * @param {string} inputTool The input tool.
 * @param {!goog.ime.offline.DataLoader=} opt_dataLoader The data loader.
 * @constructor
 */
goog.ime.offline.MLDecoder = function(inputTool, opt_dataLoader) {
  /**
   * The input tool code for this static decoder.
   *
   * @type {string}
   * @private
   */
  this.inputTool_ = inputTool;

  /**
   * The data loader.
   *
   * @type {goog.ime.offline.DataLoader}
   * @private
   */
  this.dataLoader_ = opt_dataLoader ||
      new goog.ime.offline.DataLoader(inputTool);

  /**
   * The machine learning training data parser.
   * The data loader starts to load the model data at the creation of this
   * parser.
   *
   * @type {goog.ime.offline.DataParser}
   * @private
   */
  this.parser_ = new goog.ime.offline.DataParser(inputTool, this.dataLoader_);

  /**
   * The sub-translit cache.
   *
   * @type {Object.<string, goog.ime.offline.Heap>}
   * @private
   */
  this.subTranslitCache_ = {};

  /**
   * The pre-translit cache.
   *
   * @type {Object.<string, goog.ime.offline.Heap>}
   * @private
   */
  this.preTranslitCache_ = {};

  /**
   * The maximum size of each heap in cache.
   *
   * @type {number}
   * @private
   */
  this.pruneNum_ = 1;
};


/**
 * Maximum length of source word that MLDecoder accepts.
 *
 * @type {number}
 * @const
 */
goog.ime.offline.MLDecoder.MAX_SOURCE_LENGTH = 20;


/**
 * Default number of candidates to return.
 *
 * @type {number}
 * @const
 */
goog.ime.offline.MLDecoder.DEFAULT_RESULTS_NUM = 50;


/**
 * Factor to punish multi-segments matches.
 *
 * @type {number}
 * @const
 */
goog.ime.offline.MLDecoder.MULTI_SEGMENT_FACTOR = -0.7;


/**
 * Transliterate the given source word, computing the target word candidates
 * with scores.
 *
 * @param {Array.<Array.<string>>} tokens The list of token list.
 * @param {number} resultsNum The expected number of target word candidates.
 * @param {boolean=} opt_isAllInitials The tokens are all initials.
 * @return {Array.<goog.ime.offline.Candidate>} The list of candidates.
 */
goog.ime.offline.MLDecoder.prototype.transliterate = function(
    tokens, resultsNum, opt_isAllInitials) {
  if (tokens.length > goog.ime.offline.MLDecoder.MAX_SOURCE_LENGTH) {
    return [];
  }

  if (resultsNum < 0) {
    resultsNum = goog.ime.offline.MLDecoder.DEFAULT_RESULTS_NUM;
  }

  // Generates the original transliterations for the source text, it matches the
  // all text.
  var transliterations = this.generateTransliterations_(
      tokens, opt_isAllInitials);

  // Generates the prefix matched target words.
  var prefixCandidates = this.generatePrefixTransliterations_(
      tokens, resultsNum, opt_isAllInitials);
  // Returns the original transliterations according to soundex rules, target
  // word length and dictionary.
  return this.rankTransliterations_(
      tokens, resultsNum, transliterations, prefixCandidates);
};


/**
 * Generates transliterations for tokens with associated scores.
 *
 * @param {Array.<Array.<string>>} tokens The source word tokens.
 * @param {boolean=} opt_isAllInitials The tokens are all initials.
 * @return {goog.ime.offline.Heap} The heap for the target words and its score.
 * @private
 */
goog.ime.offline.MLDecoder.prototype.generateTransliterations_ = function(
    tokens, opt_isAllInitials) {
  if (this.subTranslitCache_[this.getKey_(tokens)]) {
    return this.subTranslitCache_[this.getKey_(tokens)];
  }

  var transliterations = new goog.ime.offline.Heap();
  var sentence = this.parser_.getTargetMappings(tokens, opt_isAllInitials);
  this.updateScoresForTargets_(sentence, transliterations, 1);

  // Try out transliterations for all possible ways of breaking the word into
  // a prefix + suffix word. The prefix is looked up for in known Prefix or
  // Segment mappings. The function is called recursively for the suffix, but we
  // do memoization to ensure O(n^2) time.
  var len = tokens.length;
  for (var i = len - 1; i > 0; i--) {
    var sourcePrefix = tokens.slice(0, i);
    var sourceSuffix = tokens.slice(i);

    var targetPrefixScores = new goog.ime.offline.Heap();
    var targetSuffixScores = new goog.ime.offline.Heap();

    var targetSuffixes = this.parser_.getTargetMappings(
        sourceSuffix, opt_isAllInitials);
    // Update the targetPrefixScores for all words in targetPrefixes. Also
    // multiply each score by the probability of this source prefix segment
    // actually being a prefix.
    this.updateScoresForTargets_(targetSuffixes, targetSuffixScores, 1);

    if (targetSuffixScores.getCount() == 0) {
      continue;
    }

    // If the transliterations for source_word have already been found, then
    // simply return them.
    if (this.subTranslitCache_[this.getKey_(sourcePrefix)]) {
      targetPrefixScores = this.subTranslitCache_[this.getKey_(sourcePrefix)];
    } else {
      targetPrefixScores = this.generateTransliterations_(sourcePrefix);
    }

    // Combines sourceSegment and suffixSegment.
    var prefixValues = targetPrefixScores.getValues();
    var prefixKeys = targetPrefixScores.getKeys();
    var suffixValues = targetSuffixScores.getValues();
    var suffixKeys = targetSuffixScores.getKeys();
    for (var j = 0; j < prefixKeys.length; ++j) {
      var prefixScore = Number(prefixKeys[j]);
      var prefixSegment = prefixValues[j];
      for (var k = 0; k < suffixKeys.length; ++k) {
        var suffixScore = Number(suffixKeys[k]);
        var suffixSegment = suffixValues[k];
        var targetWord = prefixSegment + suffixSegment;
        var score = prefixScore + suffixScore +
            goog.ime.offline.MLDecoder.MULTI_SEGMENT_FACTOR;
        transliterations.increase(score, targetWord);
        if (transliterations.getCount() > this.pruneNum_) {
          transliterations.remove();
        }
      }
    }
  }
  // Cache the results.
  this.subTranslitCache_[this.getKey_(tokens)] = transliterations;
  return transliterations;
};


/**
 * Generates the prefix matched target words
 *
 * @param {Array.<Array.<string>>} tokens The source tokens.
 * @param {number} resultsNum The number of the reisults.
 * @param {boolean=} opt_isAllInitials The tokens are all initials.
 * @return {goog.ime.offline.Heap} The candidates, for each
 *     candidate, it includes
 *     - range - the number of source tokens transliterated.
 *     - target - the target word.
 * @private
 */
goog.ime.offline.MLDecoder.prototype.generatePrefixTransliterations_ =
    function(tokens, resultsNum, opt_isAllInitials) {
  if (this.preTranslitCache_[this.getKey_(tokens)]) {
    return this.preTranslitCache_[this.getKey_(tokens)];
  }

  var candidates = new goog.ime.offline.Heap();
  var length = tokens.length;
  for (var i = 1; i <= length; i++) {
    var sourcePrefix = tokens.slice(0, i);
    if (this.preTranslitCache_[this.getKey_(sourcePrefix)]) {
      candidates.clear();
      candidates.insertAll(this.preTranslitCache_[this.getKey_(sourcePrefix)]);
    } else {
      var targetPrefixes = this.parser_.getTargetMappings(
          sourcePrefix, opt_isAllInitials);
      for (var j = 0; j < targetPrefixes.length; ++j) {
        var targetPrefix = targetPrefixes[j];
        var candidate = new goog.ime.offline.Candidate(
            sourcePrefix.length, targetPrefix.segment, targetPrefix.prob);
        candidates.insert(candidate.score, candidate);
      }
      if (candidates.getCount() > resultsNum) {
        candidates.remove();
      }
      this.preTranslitCache_[this.getKey_(sourcePrefix)] = candidates;
    }
  }
  return candidates;
};


/**
 * Updates the map of scores for target language segments. If the target segment
 * has more than one character, we boost its score.
 *
 * @param {Array.<goog.ime.offline.Target>} targetWords The array of
 * target words to update scores.
 * @param {goog.ime.offline.Heap} scores The heap containing the target
 * segments and their scores.
 * @param {number} boostFactor The boost factor.
 * @private
 */
goog.ime.offline.MLDecoder.prototype.updateScoresForTargets_ = function(
    targetWords, scores, boostFactor) {
  for (var i = 0; i < targetWords.length; ++i) {
    var targetSegment = targetWords[i].segment;
    var targetScore = targetWords[i].prob;
    scores.increase(targetScore, targetSegment);
    if (scores.getCount() > this.pruneNum_) {
      scores.remove();
    }
  }
};


/**
 * Ranks the generated transliterations.
 *
 * @param {Array.<Array.<string>>} tokens The source tokens.
 * @param {number} resultsNum The expected number of results.
 * @param {goog.ime.offline.Heap} transliterations The original
 *     transliterations.
 * @param {goog.ime.offline.Heap} prefixTransliterations The
 *     candidates for the prefix word.
 * @return {Array.<goog.ime.offline.Candidate>} The candidates generated.
 * @private
 */
goog.ime.offline.MLDecoder.prototype.rankTransliterations_ = function(
    tokens, resultsNum, transliterations, prefixTransliterations) {
  var candidates = [];

  // Dont' remove transliterations, since they are in cache and will be used in
  // the future.
  var translits = transliterations.clone();
  var prefixCandidates = prefixTransliterations.clone();
  while (translits.getCount() > 1 && prefixCandidates.getCount() > 0) {
    var candidate;
    if (translits.peekKey() < prefixCandidates.peekKey()) {
      var score = Number(translits.peekKey());
      var target = translits.remove().toString();
      candidate = new goog.ime.offline.Candidate(
          tokens.length, target, score);
    } else {
      candidate = prefixCandidates.remove();
    }
    candidates.unshift(candidate);
  }

  while (prefixCandidates.getCount() > 0) {
    candidates.unshift(prefixCandidates.remove());
  }

  while (translits.getCount() > 0) {
    var score = Number(translits.peekKey());
    var target = translits.remove().toString();
    var candidate = new goog.ime.offline.Candidate(
        tokens.length, target, score);
    candidates.unshift(candidate);
  }
  return candidates;
};


/**
 * Clear the cache.
 */
goog.ime.offline.MLDecoder.prototype.clear = function() {
  for (var key in this.subTranslitCache_) {
    this.subTranslitCache_[key].clear();
  }
  this.subTranslitCache_ = {};

  for (var key in this.preTranslitCache_) {
    this.preTranslitCache_[key].clear();
  }
  this.preTranslitCache_ = {};
};


/**
 * Gets the key for a list of token lists in the cache.
 *
 * @param {Array.<Array.<string>>} tokens The tokens.
 * @return {string}
 * @private
 */
goog.ime.offline.MLDecoder.prototype.getKey_ = function(tokens) {
  var ret = '';
  for (var i = 0; i < tokens.length; ++i) {
    ret += tokens[i][0];
  }
  return ret;
};
