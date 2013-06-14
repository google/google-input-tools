// Copyright 2013 The ChromeOS VK Authors. All Rights Reserved.
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
 * @fileoverview Definition of DeferredCallManager.
 */

goog.provide('goog.ime.chrome.vk.DeferredCallManager');



/**
 * The deferred call manager which manages multiple deferred function calls.
 *
 * @constructor
 */
goog.ime.chrome.vk.DeferredCallManager = function() {
  /**
   * The deferred function queue.
   *
   * @type {!Array.<!Function>}
   * @private
   */
  this.funcQueue_ = [];
};
goog.addSingletonGetter(goog.ime.chrome.vk.DeferredCallManager);


/**
 * Adds a function call which will be callback later.
 *
 * @param {!Function} func The function object.
 */
goog.ime.chrome.vk.DeferredCallManager.prototype.addCall = function(func) {
  this.funcQueue_.push(func);
};


/**
 * Executes all function calls in order.
 */
goog.ime.chrome.vk.DeferredCallManager.prototype.execAll = function() {
  for (var i = 0; i < this.funcQueue_.length; i++) {
    this.funcQueue_[i].call();
  }
  this.funcQueue_ = [];
};
