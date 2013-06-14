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
 * @fileoverview Defines the model configs.
 */
goog.provide('goog.ime.chrome.os.ConfigFactory');

goog.require('goog.ime.chrome.os.Config');
goog.require('goog.ime.chrome.os.CangjieConfig');
goog.require('goog.ime.chrome.os.PinyinConfig');
goog.require('goog.ime.chrome.os.ZhuyinConfig');
goog.require('goog.ime.offline.InputToolCode');
goog.require('goog.object');



/**
 * The input method config factory.
 *
 * @constructor
 */
goog.ime.chrome.os.ConfigFactory = function() {
  /**
   * The current input tool code.
   *
   * @type {string}
   * @private
   */
  this.inputToolCode_ = '';


  /**
   * The map of input tool code to the config object.
   *
   * @type {!Object.<string, !goog.ime.chrome.os.Config>}
   * @private
   */
  this.map_ = {};

  /**
   * The default config.
   *
   * @type {goog.ime.chrome.os.Config}
   * @private
   */
  this.defaultConfig_ = null;
};
goog.addSingletonGetter(goog.ime.chrome.os.ConfigFactory);


/**
 * Sets the current input tool by the given input tool code.
 *
 * @param {string} inputToolCode The input tool code.
 */
goog.ime.chrome.os.ConfigFactory.prototype.setInputTool = function(
    inputToolCode) {
  this.inputToolCode_ = inputToolCode;
};


/**
 * Gets the current input tool by the given input tool code.
 *
 * @return {string} The input tool code.
 */
goog.ime.chrome.os.ConfigFactory.prototype.getInputTool = function() {
  return this.inputToolCode_;
};


/**
 * Gets the config for a given input tool code.
 *
 * @param {!string} inputToolCode the input tool code.
 * @return {goog.ime.chrome.os.Config} The config.
 */
goog.ime.chrome.os.ConfigFactory.prototype.getConfig = function(
    inputToolCode) {
  if (goog.object.isEmpty(this.map_)) {
    this.buildConfigs_();
  }
  if (this.map_[inputToolCode]) {
    return this.map_[inputToolCode];
  }
  return null;
};


/**
 * Gets the config for the current input tool.
 *
 * @return {!goog.ime.chrome.os.Config} The config.
 */
goog.ime.chrome.os.ConfigFactory.prototype.getCurrentConfig = function() {
  if (goog.object.isEmpty(this.map_)) {
    this.buildConfigs_();
  }
  var code = this.inputToolCode_;
  if (code && this.map_[code]) {
    return this.map_[code];
  }
  if (!this.defaultConfig_) {
    this.defaultConfig_ = new goog.ime.chrome.os.Config();
  }
  return this.defaultConfig_;
};


/**
 * Builds input method configs.
 *
 * @private
 */
goog.ime.chrome.os.ConfigFactory.prototype.buildConfigs_ = function() {
  var code = goog.ime.offline.InputToolCode;

  var pinyinConfig = new goog.ime.chrome.os.PinyinConfig();
  this.map_[code.INPUTMETHOD_PINYIN_CHINESE_SIMPLIFIED] = pinyinConfig;

  var zhuyinConfig = new goog.ime.chrome.os.ZhuyinConfig();
  this.map_[code.INPUTMETHOD_ZHUYIN_CHINESE_TRADITIONAL] = zhuyinConfig;

  var cangjieConfig = new goog.ime.chrome.os.CangjieConfig();
  this.map_[code.INPUTMETHOD_CANGJIE87_CHINESE_TRADITIONAL] = cangjieConfig;
};
