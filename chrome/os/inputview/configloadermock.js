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
goog.provide('i18n.input.chrome.inputview.ConfigLoaderMock');

goog.require('goog.Timer');
goog.require('goog.array');
goog.require('goog.net.jsloader');
goog.require('i18n.input.chrome.inputview.Controller');
goog.require('i18n.input.chrome.inputview.Model');
goog.require('i18n.input.chrome.message.ContextType');


var isA11yMode = false;
var featureList = [];

goog.scope(function() {
var queue;
var loadingResources;
var loadingCount;


/**
 * Load the specific URL. increase tick before loading, decrease the tick after
 * loading done.
 *
 * @param {string} url .
 * @param {i18n.input.chrome.inputview.Model} model .
 * @private
 */
i18n.input.chrome.inputview.ConfigLoaderMock.loadUrl_ = function(url, model) {
  if (!goog.array.contains(loadingResources, url)) {
    loadingResources.push(url);
    ++loadingCount;
    queue.pause();
    var deferred = goog.net.jsloader.load(url);
    deferred.addCallback(function() {
      // Async call onLoaded to avoid loadingCount reduced to 0 unexpectely,
      // because when config is loaded, controller may trigger layout loading.
      goog.Timer.callOnce(function() {
        if (--loadingCount == 0) {
          queue.resume();
        }
      });
    });
  }
};


/**
 * Mock the configuration loading process.
 *
 * @param {!i18n.input.testing.AsynTaskQueue} qu .
 */
i18n.input.chrome.inputview.ConfigLoaderMock.init = function(qu) {
  queue = qu;
  var ContextType = i18n.input.chrome.message.ContextType;
  var root = '/google3/i18n/input/javascript/chos/inputview/';

  inputview.getKeyboardConfig = function(callback) {
    callback({'a11ymode': isA11yMode, 'features': featureList});
  };

  // Override the function to reset the ticker. Because will re-create Model
  // in this funciton.
  var tempFunc = i18n.input.chrome.inputview.Controller.prototype.initialize;
  i18n.input.chrome.inputview.Controller.prototype.initialize = function(
      keyset, languageCode, passwordLayout, title) {
    loadingResources = [];
    loadingCount = [];
    tempFunc.apply(this, [keyset, languageCode, passwordLayout, title]);
  };


  i18n.input.chrome.inputview.Model.prototype.loadConfig = function(code) {
    var configId = code.replace(/\..*$/, '');
    var url = root + 'config/' + configId + '.js';
    i18n.input.chrome.inputview.ConfigLoaderMock.loadUrl_(url, this);
  };

  i18n.input.chrome.inputview.Model.prototype.loadLayout = function(layout) {
    var url = root + 'layouts/' + layout + '.js';
    i18n.input.chrome.inputview.ConfigLoaderMock.loadUrl_(url, this);
  };

  i18n.input.chrome.vk.Model.loadLayoutScript_ = function(layoutCode) {
    var url = root + '../vk/layouts/' + layoutCode + '.js';
    i18n.input.chrome.inputview.ConfigLoaderMock.loadUrl_(url);
  };
};
});  // goog.scope
