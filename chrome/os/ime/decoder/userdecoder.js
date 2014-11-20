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
 * @fileoverview The user dictionary decoder.
 */

goog.provide('goog.ime.offline.UserDecoder');

goog.require('goog.ime.offline.Heap');
goog.require('goog.object');



/**
 * The decoder can access user dictionary, fetching user committed candidates.
 * It also add entries and persist the user dictionary.
 *
 * @param {string} inputTool The input tool.
 * @constructor
 */
goog.ime.offline.UserDecoder = function(inputTool) {
  /**
   * The input tool code for this decoder.
   *
   * @type {string}
   * @private
   */
  this.inputTool_ = inputTool;

  /**
   * The latest input entries.
   *
   * @type {Object.<string, Object.<string, number>>}
   * @private
   */
  this.latestMap_ = {};

  /**
   * The latest map size.
   *
   * @type {number}
   * @private
   */
  this.latestMapSize_ = 0;

  /**
   * The permant input entries.
   *
   * @type {Object.<string, Object.<string,number>>}
   * @private
   */
  this.permantMap_ = {};

  this.init_();
};


/**
 * The maximum size for the permant map.
 *
 * @type {number}
 * @const
 */
goog.ime.offline.UserDecoder.MAX_PERMANT_SIZE = 1500;


/**
 * The maximum size for the temporarial map.
 *
 * @type {number}
 * @const
 */
goog.ime.offline.UserDecoder.MAX_LATEST_SIZE = 300;


/**
 * The boost factor for the recent committed candidates.
 *
 * @type {number}
 * @const
 */
goog.ime.offline.UserDecoder.LATEST_FACTOR = 5;


/**
 * Initialize the user decoder, reading data from local storage
 *
 * @private
 */
goog.ime.offline.UserDecoder.prototype.init_ = function() {
  if (localStorage) {
    var permantMapStr = localStorage.getItem(this.getKey_());
    if (permantMapStr) {
      this.permantMap_ = /** @type {Object.<string, Object.<string,number>>} */
          (JSON.parse(permantMapStr));
    }
  }
};


/**
 * Adds an item to the user decoder.
 *
 * @param {string} source
 * @param {string} target
 */
goog.ime.offline.UserDecoder.prototype.add = function(source, target) {
  if (!this.latestMap_[source]) {
    this.latestMap_[source] = {};
    this.latestMap_[source][target] = 1;
    this.latestMapSize_++;
  } else {
    if (this.latestMap_[source][target]) {
      this.latestMap_[source][target]++;
    } else {
      this.latestMap_[source][target] = 1;
      this.latestMapSize_++;
    }
  }
  if (this.latestMapSize_ > goog.ime.offline.UserDecoder.MAX_LATEST_SIZE) {
    this.updatePermantMap_();
  }

};


/**
 * Gets a candidate from the user decoder according to a given source.
 *
 * @param {string} source
 * @return {string} The most frequently used target word.
 */
goog.ime.offline.UserDecoder.prototype.getCandidate = function(source) {
  var latestTargets = this.latestMap_[source];
  var permantTargets = this.permantMap_[source];

  if (!latestTargets) {
    latestTargets = {};
  }

  if (!permantTargets) {
    permantTargets = {};
  }

  var bestCandidate = '';
  var bestScore = 0;

  for (var key in latestTargets) {
    var score = goog.ime.offline.UserDecoder.LATEST_FACTOR * latestTargets[key];
    if (permantTargets[key]) {
      score += permantTargets[key];
    }
    if (score > bestScore) {
      bestCandidate = key;
      bestScore = score;
    }
  }

  for (var key in permantTargets) {
    var score = permantTargets[key];
    if (latestTargets[key]) {
      score += goog.ime.offline.UserDecoder.LATEST_FACTOR * latestTargets[key];
    }
    if (score > bestScore) {
      bestCandidate = key;
      bestScore = score;
    }
  }

  return bestCandidate;
};


/**
 * Saves the user dictionary to the local storage.
 */
goog.ime.offline.UserDecoder.prototype.persist = function() {
  if (localStorage) {
    this.updatePermantMap_();
    var permantMapStr = JSON.stringify(this.permantMap_);
    localStorage.setItem(this.getKey_(), permantMapStr);
  }
};


/**
 * Gets the local storage key for the user dictionary.
 *
 * @return {string}
 * @private
 */
goog.ime.offline.UserDecoder.prototype.getKey_ = function() {
  return this.inputTool_ + '_user_dictionary';
};


/**
 * Updates the permant map.
 *
 * @private
 */
goog.ime.offline.UserDecoder.prototype.updatePermantMap_ = function() {
  for (var key in this.latestMap_) {
    var targetItems = this.latestMap_[key];
    if (this.permantMap_[key]) {
      var permantItems = this.permantMap_[key];
      for (var targetKey in targetItems) {
        if (permantItems[targetKey]) {
          permantItems[targetKey] += targetItems[targetKey];
        } else {
          permantItems[targetKey] = targetItems[targetKey];
        }
      }
    } else {
      this.permantMap_[key] = goog.object.clone(targetItems);
    }
  }

  var heap = new goog.ime.offline.Heap();
  for (var key in this.permantMap_) {
    var targetItems = this.permantMap_[key];
    for (var targetKey in targetItems) {
      heap.insert(targetItems[targetKey], 0);
      if (heap.getCount() > goog.ime.offline.UserDecoder.MAX_PERMANT_SIZE) {
        heap.remove();
      }
    }
  }

  if (heap.getCount() < goog.ime.offline.UserDecoder.MAX_PERMANT_SIZE) {
    heap.clear();
    return;
  }

  var threshold = Number(heap.peekKey());
  heap.clear();
  for (var key in this.permantMap_) {
    var targetItems = this.permantMap_[key];
    for (var targetKey in targetItems) {
      if (targetItems[targetKey] < threshold) {
        delete targetItems[targetKey];
      }
    }
    if (goog.object.isEmpty(targetItems)) {
      delete this.permantMap_[key];
    }
  }

  this.latestMap_ = {};
  this.latestMapSize_ = 0;
};
