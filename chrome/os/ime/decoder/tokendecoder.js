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
 * @fileoverview The token decoder class provides token paths for an input
 * source text.
 */

goog.provide('goog.ime.offline.LatticeNode');
goog.provide('goog.ime.offline.TokenDecoder');
goog.provide('goog.ime.offline.TokenPath');

goog.require('goog.array');
goog.require('goog.events.EventTarget');
goog.require('goog.ime.offline.DataLoader');
goog.require('goog.ime.offline.EventType');



/**
 * The lattice node contains the index of each start end of all in-edges, and
 * the minimum number of initials in paths to this node.
 *
 * @constructor
 */
goog.ime.offline.LatticeNode = function() {
  /**
   * The minimum initial numbers from paths to this node.
   *
   * @type {number}
   */
  this.initNum = 0;

  /**
   * The start ends for all in-edges.
   *
   * @type {Array.<number>}
   */
  this.edges = [];
};



/**
 * The token path generated from token decoders. It contains the information for
 * tokens and separators.
 *
 * @param {Array.<string>} tokens The tokens.
 * @param {Array.<boolean>} separators Whether the corresponding token ends with
 * a separator.
 * @constructor
 */
goog.ime.offline.TokenPath = function(tokens, separators) {
  /**
   * The token list.
   *
   * @type {Array.<string>}
   */
  this.tokens = tokens;

  /**
   * A list of booleans to show whether the corresponding token ends with
   * separators.
   *
   * @type {Array.<boolean>}
   */
  this.separators = separators;
};



/**
 * The token decoder can generates different token paths for an input text, it
 * also selects the best one from them.
 *
 * @param {string} inputTool The input tool.
 * @param {!goog.ime.offline.DataLoader=} opt_dataLoader The data loader.
 * @param {Array.<string>=} opt_fuzzyPairs The fuzzy expansions.
 * @constructor
 * @extends {goog.events.EventTarget}
 */
goog.ime.offline.TokenDecoder = function(
    inputTool, opt_dataLoader, opt_fuzzyPairs) {
  goog.base(this);

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
   * The fuzzy expansion pairs.
   *
   * @type {Object.<string, Array.<string>>}
   * @private
   */
  this.fuzzyMap_ = null;

  /**
   * The token regular expression.
   *
   * @type {RegExp}
   * @private
   */
  this.tokenReg_ = /xyz/g;

  /**
   * The tones in zhuyin input method.
   *
   * @type {string}
   * @private
   */
  this.tones_ = '\u02c9\u02ca\u02c7\u02cb\u02d9';

  /**
   * The invalid tokens in pinyin input method.
   *
   * @type {string}
   * @private
   */
  this.invalidChars_ = 'iuv';

  /**
   * The initial character map, it maps an initial character to all possible
   * normalized tokens.
   *
   * @type {Object.<string, Array.<string> >}
   * @private
   */
  this.initialMap_ = {};

  /**
   * The current source string, without separators.
   *
   * @type {string}
   * @private
   */
  this.currentStr_ = '';

  /**
   * The separator positions, ordered from smaller to greater.
   *
   * @type {Array.<number>}
   * @private
   */
  this.separators_ = [];

  /**
   * The separator character.
   *
   * @type {string}
   * @private
   */
  this.separatorChar_ = '\'';

  /**
   * The lattice nodes.
   *
   * @type {Array.<goog.ime.offline.LatticeNode>}
   * @private
   */
  this.lattice_ = [];

  this.dataLoader_.loadModelData(
      goog.bind(this.init_, this, opt_fuzzyPairs));
};
goog.inherits(goog.ime.offline.TokenDecoder, goog.events.EventTarget);


/**
 * The init number given to invalid path, to make sure it is larger than any
 * valid path.
 *
 * @type {number}
 * @private
 */
goog.ime.offline.TokenDecoder.INVALID_PATH_INIT_NUM_ = 100;


/**
 * Initialize the token decoder
 *
 * @param {Array.<string>=} opt_fuzzyPairs The fuzzy expansions.
 * @private
 */
goog.ime.offline.TokenDecoder.prototype.init_ = function(opt_fuzzyPairs) {
  var tokens = this.dataLoader_.tokens;
  var untoneTokens = this.dataLoader_.untoneTokens;
  var initialTokens = this.dataLoader_.initialTokens;

  if (untoneTokens) {
    this.tokenReg_ = new RegExp(
        '^(' + tokens + '|' + untoneTokens + '|' + initialTokens + ')$');
  } else {
    this.tokenReg_ = new RegExp(
        '^(' + tokens + '|' + initialTokens + ')$');
  }

  var initials = initialTokens.split('|');
  for (var i = 0; i < initials.length; i++) {
    this.initialMap_[initials[i]] = [];
  }

  var tokenVector = tokens.split('|');
  for (var i = 0; i < tokenVector.length; i++) {
    var token = tokenVector[i];
    for (var j = 1; j <= 2; ++j) {
      var prefix = token.slice(0, j);
      if (this.initialMap_[prefix] && token.length > 1) {
        this.initialMap_[prefix].push(token);
      }
    }
  }

  if (opt_fuzzyPairs) {
    this.updateFuzzyPairs(opt_fuzzyPairs);
  }

  this.clear();
};


/**
 * Updates fuzzy-pinyin expansion paris. For each fuzzy-pinyin expansion pairs,
 * the syllables are separated by '_', such as 'z_zh'.
 *
 * @param {Array.<string>} fuzzyPairs The fuzzy expansions.
 */
goog.ime.offline.TokenDecoder.prototype.updateFuzzyPairs = function(
    fuzzyPairs) {
  this.fuzzyMap_ = {};
  for (var i = 0; i < fuzzyPairs.length; ++i) {
    var fuzzyPair = fuzzyPairs[i];
    var syllables = fuzzyPair.split('_');
    // The length of syllabels should be 2.
    for (var j = 0; j < 2; ++j) {
      if (!this.fuzzyMap_[syllables[j]]) {
        this.fuzzyMap_[syllables[j]] = [];
      }
      this.fuzzyMap_[syllables[j]].push(syllables[1 - j]);
    }
  }
};


/**
 * Whether the tokens are all initials.
 *
 * @param {Array.<string>} tokens The token list.
 * @return {boolean} True, if all these tokens are initials.
 */
goog.ime.offline.TokenDecoder.prototype.isAllInitials = function(tokens) {
  for (var i = 0; i < tokens.length; ++i) {
    if (!this.initialMap_[tokens[i]]) {
      return false;
    }
  }
  return true;
};


/**
 * Gets all possible normalized tokens for a token list.
 *
 * @param {Array.<string>} tokens The original tokens, which may be initials.
 * @return {Array.<Array.<string>>} The all possible tokens, including the
 * original tokens, initial expansions and fuzzy expansions.
 */
goog.ime.offline.TokenDecoder.prototype.getNormalizedTokens = function(tokens) {
  var ret = [];
  for (var i = 0; i < tokens.length; ++i) {
    ret.push(this.getNormalizedToken(tokens[i]));
  }
  return ret;
};


/**
 * Gets all potential matched tokens for one token.
 *
 * @param {string} token The original token, which may be an initial.
 * @return {Array.<string>} The potential tokens, including the original ones,
 * initial expansions and fuzzy expansions.
 */
goog.ime.offline.TokenDecoder.prototype.getNormalizedToken = function(token) {
  var segments = [token];
  var toneReg = new RegExp('[' + this.tones_ + ']');

  if (this.initialMap_[token]) {
    // It is an initial character
    segments = segments.concat(
        goog.array.clone(this.initialMap_[token]));
  } else if (this.dataLoader_.untoneTokens &&
      !token.slice(-1).match(toneReg)) {
    for (var i = 0; i < this.tones_.length; ++i) {
      segments.push(token + this.tones_[i]);
    }
  }

  if (this.fuzzyMap_) {
    for (var syllable in this.fuzzyMap_) {
      if (token.match(syllable)) {
        if (segments.length == 1) {
          // Since if a segment has more than one syllable, the first one will
          // be considered as an identifier, it needs to push the prefix itself
          // to the segment, if the length of the segment is one.
          segments.push(token);
        }
        var replacers = this.fuzzyMap_[syllable];
        for (var i = 0; i < replacers.length; ++i) {
          segments.push(token.replace(syllable, replacers[i]));
        }
      }
    }
  }

  return segments;
};


/**
 * Gets all possible token paths.
 *
 * @param {string} source The source word.
 * @return {Array.<goog.ime.offline.TokenPath>} The list of token paths.
 */
goog.ime.offline.TokenDecoder.prototype.getTokens = function(source) {
  var originalStr = this.getOriginalStr_();
  if (source.indexOf(originalStr) == 0) {
    // Appends chars.
    this.append_(source.slice(originalStr.length));
  }

  var range = this.getRange_(source);
  if (range) {
    // source is a substring of the current string.
    var paths = this.getPaths_(range.start, range.end);
    var tokenLists = [];
    for (var i = 0; i < paths.length; ++i) {
      var path = paths[i];
      var tokenList = [];
      var separatorList = [];
      var index = range.start;
      for (var j = 0; j < path.length; ++j) {
        tokenList.push(this.currentStr_.slice(index, path[j]));
        separatorList.push(this.separators_.indexOf(path[j]) >= 0);
        index = path[j];
      }
      tokenLists.push(new goog.ime.offline.TokenPath(
          tokenList, separatorList));
    }
    if (tokenLists.length == 0) {
      // There is no fully matched one.
      return this.getTokens(source.slice(0, -1));
    }
    return tokenLists;
  }

  // If the current string does not contain the source, it means the source text
  // has be modified, then clears the all decoder.
  this.dispatchEvent(goog.ime.offline.EventType.CLEAR);
  return this.getTokens(source);
};


/**
 * Gets the best token path.
 *
 * @param {string} source The source text.
 * @return {goog.ime.offline.TokenPath} The best token path.
 */
goog.ime.offline.TokenDecoder.prototype.getBestTokens = function(source) {
  var tokenLists = this.getTokens(source);
  if (tokenLists.length == 0) {
    return null;
  }

  if (tokenLists.length == 1) {
    return tokenLists[0];
  }

  var tokenNum = goog.array.reduce(
      tokenLists, function(minTokenNum, tokenList) {
        return Math.min(minTokenNum, tokenList.tokens.length);
      }, Infinity);

  tokenLists = goog.array.filter(
      tokenLists, function(tokenList) {
        return tokenList.tokens.length <= tokenNum;
      });

  if (tokenLists.length == 1) {
    return tokenLists[0];
  }

  var vowel = /[aeiou]/;
  var vowelNums = [];
  var vowelStartNum = goog.array.reduce(
      tokenLists, function(minVowelNum, tokenList, index) {
        var vowelNum = goog.array.reduce(
            tokenList.tokens, function(ret, token) {
              return token.slice(0, 1).match(vowel) ? ret + 1 : ret;
            }, 0);
        vowelNums[index] = vowelNum;
        return Math.min(minVowelNum, vowelNum);
      }, Infinity);

  var index = vowelNums.indexOf(vowelStartNum);
  return tokenLists[index];
};


/**
 * Gets the original token list.
 *
 * @param {goog.ime.offline.TokenPath} tokenPath The token path.
 * @return {Array.<string>} The tokens with separators.
 */
goog.ime.offline.TokenDecoder.prototype.getOriginalTokens = function(
    tokenPath) {
  var separators = tokenPath.separators;
  var tokens = goog.array.clone(tokenPath.tokens);
  for (var i = 0; i < tokens.length; ++i) {
    if (separators[i]) {
      tokens[i] += this.separatorChar_;
    }
  }
  return tokens;
};


/**
 * Clears the lattice and current string.
 */
goog.ime.offline.TokenDecoder.prototype.clear = function() {
  this.currentStr_ = '';
  this.separators_ = [];
  this.lattice_ = [];
  this.lattice_[0] = {};
  this.lattice_[0].initNum = 0;
};


/**
 * Appends string after the current string.
 *
 * @param {string} source The source string.
 * @private
 */
goog.ime.offline.TokenDecoder.prototype.append_ = function(source) {
  for (var i = 0; i < source.length; ++i) {
    var ch = source.slice(i, i + 1);
    var lastIndex = this.currentStr_.length;
    if (ch == this.separatorChar_) {
      // Records the separator position.
      this.separators_.push(lastIndex);
      continue;
    }
    this.currentStr_ = this.currentStr_ + ch;
    lastIndex++;

    // Finds the paths with minumum initials.
    var suffixes = this.getSuffixTokens_(this.currentStr_);
    var initNums = [];
    var initNum = goog.array.reduce(
        suffixes, function(minInitNum, suffix, index) {
          if (!this.inSameRange_(lastIndex - suffix.length, lastIndex)) {
            initNums[index] = Infinity;
            return minInitNum;
          }
          var preInitNum = this.lattice_[lastIndex - suffix.length].initNum;
          initNums[index] = this.initialMap_[suffix] ?
              preInitNum + 1 : preInitNum;
          if (this.invalidChars_.indexOf(suffix) >= 0) {
            initNums[index] +=
                goog.ime.offline.TokenDecoder.INVALID_PATH_INIT_NUM_;
          }
          return Math.min(minInitNum, initNums[index]);
        }, Infinity, this);

    if (initNum == Infinity) {
      // No available suffix
      continue;
    }

    this.lattice_[lastIndex] = new goog.ime.offline.LatticeNode();
    this.lattice_[lastIndex].initNum = initNum;
    this.lattice_[lastIndex].edges = [];
    for (var j = 0; j < suffixes.length; ++j) {
      var suffix = suffixes[j];
      if (initNums[j] > initNum) {
        continue;
      }
      this.lattice_[lastIndex].edges.push(lastIndex - suffix.length);
    }
  }
};


/**
 * Gets all paths from a start node, and ends before the given end node.
 *
 * @param {number} start The start node.
 * @param {number} end The end node.
 * @return {Array.<Array.<number>>} The list of paths, for each path, it is an
 * array contains all nodes in the path from the start to the end.
 * @private
 */
goog.ime.offline.TokenDecoder.prototype.getPathsFrom_ = function(start, end) {
  var paths = [];
  for (var i = start + 1; i <= end; ++i) {
    var edges = this.lattice_[i].edges;
    for (var j = 0; j < edges.length; ++j) {
      if (edges[j] == start) {
        paths.push([i]);
        var forwardPaths = goog.array.clone(this.getPathsFrom_(i, end));
        for (var k = 0; k < forwardPaths.length; ++k) {
          paths.push([i].concat(forwardPaths[k]));
        }
        break;
      }
    }
  }
  return paths;
};


/**
 * Gets all paths from a start node to an end node.
 *
 * @param {number} start The start position.
 * @param {number} end The end position.
 * @return {Array.<Array.<number>>} The list of paths, for each path, it is an
 * array contains all nodes in the path from the start to the end.
 * @private
 */
goog.ime.offline.TokenDecoder.prototype.getPaths_ = function(start, end) {
  if (!this.lattice_[end] || !this.lattice_[end].edges) {
    return [];
  }

  var paths = [];
  var edges = this.lattice_[end].edges;
  for (var j = 0; j < edges.length; ++j) {
    if (edges[j] < start) {
      continue;
    }

    if (edges[j] == start) {
      paths.push([end]);
    } else {
      var forwardPaths = goog.array.clone(this.getPaths_(start, edges[j]));
      for (var k = 0; k < forwardPaths.length; ++k) {
        forwardPaths[k].push(end);
        paths.push(forwardPaths[k]);
      }
    }
  }

  return paths;
};


/**
 * Gets the source string with separators.
 *
 * @return {string} The original string with separators.
 * @private
 */
goog.ime.offline.TokenDecoder.prototype.getOriginalStr_ = function() {
  var ret = '';
  var index = 0;
  for (var i = 0; i < this.separators_.length; ++i) {
    var subStr = this.currentStr_.slice(index, this.separators_[i]);
    ret += subStr + this.separatorChar_;
    index = this.separators_[i];
  }
  ret += this.currentStr_.slice(index);
  return ret;
};


/**
 * Gets the range of the current string for an input string.
 *
 * @param {string} source
 * @return {Object.<string, number>}
 * @private
 */
goog.ime.offline.TokenDecoder.prototype.getRange_ = function(source) {
  var originalStr = this.getOriginalStr_();
  var start = originalStr.indexOf(source);
  if (start < 0) {
    return null;
  }

  var end = start + source.length;
  var separatorNumBeforeStart =
      originalStr.slice(0, start).split(this.separatorChar_).length - 1;
  var separatorNumBeforeEnd =
      originalStr.slice(0, end).split(this.separatorChar_).length - 1;

  var range = {};
  range.start = start - separatorNumBeforeStart;
  range.end = end - separatorNumBeforeEnd;
  return range;
};


/**
 * Whether the start and end node are in not separatored by separators.
 *
 * @param {number} start The start node.
 * @param {number} end The end node.
 * @return {boolean} Whether they are in the same range separated by separators.
 * @private
 */
goog.ime.offline.TokenDecoder.prototype.inSameRange_ = function(start, end) {
  var startPos = goog.array.binarySearch(this.separators_, start);
  var endPos = goog.array.binarySearch(this.separators_, end);

  if (startPos == endPos) {
    return true;
  }

  if (startPos >= 0 && startPos + endPos == -2) {
    return true;
  }

  if (endPos >= 0 && startPos + endPos == -1) {
    return true;
  }

  if (startPos >= 0 && endPos >= 0 && endPos == startPos + 1) {
    return true;
  }

  return false;
};


/**
 * Gets all the suffix tokens.
 *
 * @param {string} source The source.
 * @return {Array.<string>} The suffix tokens.
 * @private
 */
goog.ime.offline.TokenDecoder.prototype.getSuffixTokens_ = function(source) {
  var ret = [];
  for (var i = 1; i <= 5 && i <= source.length; ++i) {
    var suffix = this.currentStr_.slice(-i);
    if (suffix.match(this.tokenReg_)) {
      ret.push(suffix);
    }
  }

  if (ret.length == 0) {
    // If there is no matched suffix, for example, "i" in Pinyin, push the last
    // character to the result.
    ret.push(source.slice(-1));
  }
  return ret;
};
