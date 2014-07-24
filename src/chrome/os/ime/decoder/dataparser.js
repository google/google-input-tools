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
// Copyright 2013 The ChromeOS IME Authors. All Rights Reserved.
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
 * @fileoverview goog.ime.offline.DataParser is implemented to parse the
 * encoded data files.
 */

goog.provide('goog.ime.offline.DataParser');
goog.provide('goog.ime.offline.Long');
goog.provide('goog.ime.offline.Target');

goog.require('goog.array');
goog.require('goog.object');



/**
 * Constructs a 64-bit two's-complement integer, given its low and high 32-bit
 * values as unsigned integers.
 *
 * @param {number} low  The low 32 bits of the long.
 * @param {number} high  The high 32 bits of the long.
 * @constructor
 */
goog.ime.offline.Long = function(low, high) {
  /**
   * @type {number}
   * @private
   */
  this.low_ = low | 0;  // force into 32 signed bits.

  /**
   * @type {number}
   * @private
   */
  this.high_ = high | 0;  // force into 32 signed bits.
};


/**
 * @type {number}
 * @private
 */
goog.ime.offline.Long.TWO_PWR_32_DBL_ =
    (1 << 16) * (1 << 16);


/**
 * Returns a Long representing the given (32-bit) integer value.
 * @param {number} value The 32-bit integer in question.
 * @return {!goog.ime.offline.Long} The corresponding Long value.
 */
goog.ime.offline.Long.fromInt = function(value) {
  var obj = new goog.ime.offline.Long(value, 0);
  return obj;
};


/**
 * Returns the bitwise-OR of this Long and the given one.
 *
 * @param {goog.ime.offline.Long} other The Long with which to OR.
 * @return {!goog.ime.offline.Long} The bitwise-OR of this and the other.
 */
goog.ime.offline.Long.prototype.or = function(other) {
  return new goog.ime.offline.Long(
      this.low_ | other.low_,
      this.high_ | other.high_);
};


/**
 * Returns this Long with bits shifted to the left by the given amount.
 *
 * @param {number} numBits The number of bits by which to shift.
 * @return {!goog.ime.offline.Long} This shifted to the left by the given
 *     amount.
 */
goog.ime.offline.Long.prototype.shiftLeft = function(numBits) {
  var ret = this;
  numBits &= 63;
  if (numBits == 0) {
    ret = this;
  } else {
    var low = this.low_;
    if (numBits < 32) {
      var high = this.high_;
      var x = high << numBits;
      var y = low >>> (32 - numBits);
      ret = new goog.ime.offline.Long(low << numBits, x + y);
    } else {
      ret = new goog.ime.offline.Long(0, low << (numBits - 32));
    }
  }
  return ret;
};


/** @return {number} The closest floating-point representation to this value. */
goog.ime.offline.Long.prototype.toNumber = function() {
  return this.high_ * goog.ime.offline.Long.TWO_PWR_32_DBL_ +
      this.getLowBitsUnsigned();
};


/** @return {number} The low 32-bits as an unsigned value. */
goog.ime.offline.Long.prototype.getLowBitsUnsigned = function() {
  return (this.low_ >= 0) ?
      this.low_ : goog.ime.offline.Long.TWO_PWR_32_DBL_ + this.low_;
};



/**
 * The target element parsed from model data.
 *
 * @param {string} segment The target segment.
 * @param {number} prob The target score.
 * @constructor
 */
goog.ime.offline.Target = function(segment, prob) {
  /**
   * The target segment.
   *
   * @type {string}
   */
  this.segment = segment;

  /**
   * The target score.
   *
   * @type {number}
   */
  this.prob = prob;
};



/**
 * The parser to parse the model data. It is used to find all the target
 * segments and their corresponding scores for a given source segment.
 *
 * @param {string} inputTool The input tool.
 * @param {goog.ime.offline.DataLoader} dataLoader The data loader.
 * @constructor
 */
goog.ime.offline.DataParser = function(inputTool, dataLoader) {
  /**
   * The input tool of this data parser.
   *
   * @type {string}
   * @private
   */
  this.inputTool_ = inputTool;

  /**
   * The data loader.
   *
   * @type {goog.ime.offline.DataLoader}
   * @protected
   */
  this.dataLoader = dataLoader;

  /**
   * The encoding map for source language. The source alphabet is encoded by
   * Huffman code in the model data. The source map provides the encoding map
   * for source strings.
   *
   * In the map, each key is a source character, while the value is a
   * two-dimensinal array of numbers, [x, y]. The mapping code is the last y
   * bits in the number x.
   *
   * For instance, if {'a': [3,2], 'b': [0,1]}, then 'ab' is encoded as 6(110).

   * @type {Object}
   * @protected
   */
  this.sourceMap = null;

  /**
   * The encoding map for target language. The target alphabet is encoded by
   * Huffman code in the model data. The target map provides the decoding map
   * for the target strings.
   *
   * The array contains two arrays. The first one is the target map for all
   * characters whose code start at 0, the second one is the target map for all
   * characters whose code start at 1.
   *
   * For instance, if ['b', ['c', 'a']], then 0 -> b, 2(10) -> 'c' and 3(11) ->
   * 'a'.
   *

   * @type {Array}
   * @protected
   */
  this.targetMap = null;

  /**
   * The source segments. The array of Huffman code of each source
   * segment. The segments are ordered, so a segment can be binary searched by
   * its code.
   *
   * @type {Array.<number|Array.<number> >}
   * @private
   */
  this.sourceSegments_ = [];

  /**
   * The corresponding target offsets for a source segment.
   *
   * @type {Array.<number>}
   * @private
   */
  this.targetOffsets_ = [];

  /**
   * The target segments. The array of Huffman code of each target
   * segment.
   *
   * @type {Array.<number|Array.<number> >}
   * @private
   */
  this.targetSegments_ = [];

  /**
   * The target probabilities. The scores for the corresponding
   * target segemnts in this.targetSegments.
   *
   * @type {Array.<number>}
   * @private
   */
  this.targetProbs_ = [];

  /**
   * The default score. If the target score is undefined, sets it the default
   * score.
   *
   * @type {number}
   * @private
   */
  this.defaultProb_ = 1;

  /**
   * The maxium length to find all possible token sequeses.
   *
   * @type {number}
   * @private
   */
  this.maxTokenSize_ = 300;

  /**
   * Whether the data is ready.
   *
   * @type {boolean}
   */
  this.ready = false;

  this.dataLoader.loadModelData(goog.bind(this.loadComplete_, this));
};


/**
 * Loads the model data completed.
 *
 * @private
 */
goog.ime.offline.DataParser.prototype.loadComplete_ = function() {
  var data = this.dataLoader;

  this.sourceSegments_ = data.sourceSegments;
  this.targetSegments_ = data.targetSegments;
  this.targetProbs_ = data.targetProbs;
  this.targetOffsets_ = data.targetPositions;

  this.sourceMap = data.sourceMap;
  this.targetMap = /** @type {Array} */ (data.targetMap);
  this.defaultProb_ = Number(data.defaultProb);

  this.ready_ = true;
};


/**
 * Since in the training data, the probability is stored as that times the
 * default probability, normalizes it.
 *
 * @param {number} prob The stored probability.
 * @return {number} The normalized probability.
 */
goog.ime.offline.DataParser.prototype.normalizeProb = function(prob) {
  if (prob == undefined) {
    return 1;
  }
  return prob / this.defaultProb_;
};


/**
 * Gets the position of the encoded source word in the source word array.
 *
 * @param {Array.<string>} source The source tokens.
 * @return {number} The position of the source word.
 * @protected
 */
goog.ime.offline.DataParser.prototype.getSourcePos = function(source) {
  var sourceIndex = this.encodeUnicodeString(source);
  return goog.array.binarySearch(
      this.sourceSegments_, sourceIndex, this.compareFn);
};


/**
 * Given the source word, gets the range of the corresponding target words in
 * the target word array.
 *
 * @param {Array.<string>} source The source tokens.
 * @return {Object.<string, number>} The range of the target words.
 *     - start: The start position of the range in the target words array.
 *     - end: The end position of the range in the target words array.
 * @protected
 */
goog.ime.offline.DataParser.prototype.getTargetPos = function(source) {
  var targetPos = {};
  var sourcePos = this.getSourcePos(source);
  if (sourcePos < 0) {
    targetPos.start = 0;
    targetPos.end = -1;
    return targetPos;
  }
  var currentOffset = this.targetOffsets_[sourcePos];
  var nextOffset = sourcePos < this.targetOffsets_.length - 1 ?
      this.targetOffsets_[sourcePos + 1] :
      this.targetSegments_.length;
  targetPos.start = currentOffset;
  targetPos.end = nextOffset;
  return targetPos;
};


/**
 * Gets the target mappings from a source word.
 *
 * @param {Array.<Array.<string>>} tokens The source tokens.
 * @param {boolean=} opt_isAllInitials The tokens are all initials.
 * @return {Array.<goog.ime.offline.Target>} The target mappings.
 *     - segment - The target segment.
 *     - prob - The target probability.
 */
goog.ime.offline.DataParser.prototype.getTargetMappings = function(
    tokens, opt_isAllInitials) {
  var sources = this.getTokenSequeces(tokens, opt_isAllInitials);
  var targetMappings = [];
  for (var j = 0; j < sources.length; ++j) {
    var source = sources[j];
    var targetSegments = this.targetSegments_;
    var targetProbs = this.targetProbs_;
    var targetPos = this.getTargetPos(source);
    var startOffset = targetPos.start;
    var endOffset = targetPos.end;
    for (var i = startOffset; i < endOffset; i++) {
      var segment = this.decodeUnicodeString(targetSegments[i]);
      var prob = targetProbs[i];
      var targetMapping = new goog.ime.offline.Target(
          segment, this.normalizeProb(prob));
      targetMappings.push(targetMapping);
    }
    if (opt_isAllInitials && j == 0 && startOffset < endOffset) {
      break;
    }
  }
  return targetMappings;
};


/**
 * Encode a string to a number or a list of numbers.
 *
 * @param {Array.<string>} tokens The source tokens.
 * @return {number|Array.<number>} The encoded number represented the string.
 * @protected
 */
goog.ime.offline.DataParser.prototype.encodeUnicodeString = function(
    tokens) {
  var map = this.sourceMap;

  var bufferArray = [];
  var buffer = goog.ime.offline.Long.fromInt(0);
  var sign = goog.ime.offline.Long.fromInt(1);
  var currentSize = 0;

  var len = tokens.length;
  for (var i = 0; i < len; i++) {
    var token = tokens[i];
    var digit = 0;
    if (!goog.object.containsKey(map, token)) {
      return 0;
    }
    var enc_bit = goog.ime.offline.Long.fromInt(Number(map[token][0]));
    var enc_length = Number(map[token][1]);

    if (currentSize + enc_length >= 63) {
      var enc_number = buffer.or(sign.shiftLeft(currentSize));
      bufferArray.unshift(enc_number.toNumber());
      buffer = enc_bit;
      currentSize = enc_length;
    } else {
      buffer = buffer.or(enc_bit.shiftLeft(currentSize));
      currentSize += enc_length;
    }
  }

  if (currentSize > 0) {
    var enc_number = buffer.or(sign.shiftLeft(currentSize));
    bufferArray.unshift(enc_number.toNumber());
  }

  return bufferArray.length == 1 ? bufferArray[0] : bufferArray;
};


/**
 * Decode a number or a list of numbers to a string.
 *
 * @param {number|Array.<number>} num The encoded number.
 * @return {string} The decoded string.
 * @protected
 */
goog.ime.offline.DataParser.prototype.decodeUnicodeString = function(num) {
  var map = this.targetMap;
  if (goog.isArray(num)) {
    // It is an array of encoded numbers
    var str = '';
    for (var i = 0; i < num.length; i++) {
      var substr = this.decodeUnicodeString(num[i]);
      str = substr + str;
    }
    return str;
  }
  var str = '';
  var pos = map;
  while (num != 1) {
    var bit = num % 2;
    num = (num - bit) / 2;
    pos = pos[bit];

    if (!goog.isArray(pos)) {
      // Got the value
      str = str + pos;
      pos = map;
    }
  }
  return str;
};


/**
 * Gets the list of token sequece for all possible token combinations according
 * to a list of tokens.
 *
 * @param {Array.<Array.<string>>} tokens The token tables.
 * @param {boolean=} opt_isAllInitials The tokens are all initials.
 * @return {Array.<Array.<string>>} The list for token paths.
 */
goog.ime.offline.DataParser.prototype.getTokenSequeces = function(
    tokens, opt_isAllInitials) {
  var size = 1;
  for (var i = 0; i < tokens.length; ++i) {
    size *= tokens[i].length;
  }

  var path = [];
  for (var i = 0; i < tokens.length; ++i) {
    path.push(tokens[i][0]);
  }

  if (size == 1 || size > this.maxTokenSize_) {
    return [path];
  }

  var ret = [[]];
  for (var i = 0; i < tokens.length; i++) {
    var length = ret.length;
    for (var j = 0; j < length; j++) {
      var prefix = ret.shift();
      for (var k = tokens[i].length > 1 ? 1 : 0;
          k < tokens[i].length; k++) {
        ret.push(prefix.concat(tokens[i][k]));
      }
    }
  }
  return opt_isAllInitials ? [path].concat(ret) : ret;
};


/**
 * The compare function used to binary search.
 *
 * @param {number|Array.<number>} val1 The first value.
 * @param {number|Array.<number>} val2 The second value.
 * @return {number} Greater than zero if val1 is greater than val2, Less than
 * zero if val1 is less than val2, otherwise they equal to each other.
 * @protected
 */
goog.ime.offline.DataParser.prototype.compareFn = function(val1, val2) {
  if (goog.isArray(val1) && goog.isArray(val2)) {
    if (val1.length == val2.length) {
      return goog.array.compare3(val1, val2);
    }
    return val1.length - val2.length;
  }

  if (goog.isArray(val1)) {
    return 1;
  }

  if (goog.isArray(val2)) {
    return -1;
  }

  return val1 - val2;
};
