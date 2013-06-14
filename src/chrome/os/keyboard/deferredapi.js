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
 * @fileoverview Definition of deferred input API.
 */

goog.provide('goog.ime.chrome.vk.DeferredApi');

goog.require('goog.ime.chrome.vk.DeferredCallManager');


/**
 * Commits the text to input box.
 *
 * @param {number} contextId The context ID.
 * @param {string} text The text to be inserted into the input box.
 */
goog.ime.chrome.vk.DeferredApi.commitText = function(contextId, text) {
  goog.ime.chrome.vk.DeferredCallManager.getInstance().addCall(function() {
    chrome.input.ime.commitText({
      'contextID' : contextId,
      'text': text});
  });
};


/**
 * Sets composition.
 *
 * @param {number} contextId The context ID.
 * @param {string} text The text to be inserted into the input box.
 * @param {number} cursor The cursor of the composition.
 */
goog.ime.chrome.vk.DeferredApi.setComposition = function(
    contextId, text, cursor) {
  goog.ime.chrome.vk.DeferredCallManager.getInstance().addCall(function() {
    chrome.input.ime.setComposition({
      'contextID' : contextId,
      'text': text,
      'cursor': cursor});
  });
};


/**
 * Clears composition.
 *
 * @param {number} contextId The context ID.
 */
goog.ime.chrome.vk.DeferredApi.clearComposition = function(contextId) {
  goog.ime.chrome.vk.DeferredCallManager.getInstance().addCall(function() {
    chrome.input.ime.clearComposition({
      'contextID' : contextId}, function(b) {});
  });
};


/**
 * Deletes surrounding text.
 *
 * @param {string} engineId The engine ID.
 * @param {number} contextId The context ID.
 * @param {number} back The backspace count.
 * @param {string} text The text to be inserted into the input box.
 */
goog.ime.chrome.vk.DeferredApi.deleteSurroundingText = function(
    engineId, contextId, back, text) {
  goog.ime.chrome.vk.DeferredCallManager.getInstance().addCall(function() {
    chrome.input.ime.deleteSurroundingText({
      'engineID' : engineId,
      'contextID' : contextId,
      'offset': -back,
      'length': back}, function() {
      chrome.input.ime.commitText({
        'contextID' : contextId,
        'text': text});
    });
  });
};
