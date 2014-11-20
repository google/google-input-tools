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
 * @fileoverview Defines the candidate generated in offline decoders.
 */

goog.provide('goog.ime.offline.Candidate');



/**
 * The candidate generated in offline decoders.
 *
 * @param {number} range The number of source tokens transliterated.
 * @param {string} target The target word.
 * @param {number} score The score of the target word.
 * @constructor
 */
goog.ime.offline.Candidate = function(range, target, score) {
  /**
   * The number of source tokens transliterated.
   *
   * @type {number}
   */
  this.range = range;

  /**
   * The target word.
   *
   * @type {string}
   */
  this.target = target;

  /**
   * The score.
   *
   * @type {number}
   */
  this.score = score;
};
