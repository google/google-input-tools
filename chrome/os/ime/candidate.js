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
 * @fileoverview The candidate items for editor.
 */
goog.provide('goog.ime.chrome.os.Candidate');



/**
 * The Candidate represent a suggestion of transliteration.
 *
 * @param {string} target The target.
 * @param {number} range The number of tokens for this candidate.
 * @param {string=} opt_annotation The annotation.
 * @constructor
 */
goog.ime.chrome.os.Candidate = function(target, range, opt_annotation) {
  /**
   * The target of the candidate.
   *
   * @type {string}
   */
  this.target = target;

  /**
   * The number of tokens which is transliterated by this candidate.
   *
   * @type {number}
   */
  this.range = range;

  /**
   * The annotation.
   *
   * @type {string}
   */
  this.annotation = opt_annotation || '';
};
