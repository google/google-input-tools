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
 * @fileoverview Definition of DeferredCallManager.
 */

goog.provide('i18n.input.chrome.vk.DeferredCallManager');



/**
 * The deferred call manager which manages multiple deferred function calls.
 *
 * @constructor
 */
i18n.input.chrome.vk.DeferredCallManager = function() {
  /**
   * The deferred function queue.
   *
   * @type {!Array.<!Function>}
   * @private
   */
  this.funcQueue_ = [];
};
goog.addSingletonGetter(i18n.input.chrome.vk.DeferredCallManager);


/**
 * Adds a function call which will be callback later.
 *
 * @param {!Function} func The function object.
 */
i18n.input.chrome.vk.DeferredCallManager.prototype.addCall = function(func) {
  this.funcQueue_.push(func);
};


/**
 * Executes all function calls in order.
 */
i18n.input.chrome.vk.DeferredCallManager.prototype.execAll = function() {
  for (var i = 0; i < this.funcQueue_.length; i++) {
    this.funcQueue_[i].call();
  }
  this.funcQueue_ = [];
};
