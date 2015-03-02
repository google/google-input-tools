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
goog.require('goog.testing.AsyncTestCase');
goog.require('i18n.input.chrome.inputview.Model');
goog.require('i18n.input.chrome.message.ContextType');


var asyncTestCase;
var isA11yMode = false;
var featureList = [];
var controller;
var ElementType, EventType, Command;
var Name, Type;
var ContextType;


function waitForKeysetAndVerify(c, keyset, verifyFunc) {
  var verify = function() {
    try {
      view = c.container_.keysetViewMap[keyset];
      assertTrue('waiting for keyset ' + keyset + ' failed',
          !!view);
      verifyFunc();
    } catch (e) {
      // Wait when failure for 500ms to avoid one case failure causes other
      // cases' failures.
      // TODO(shuchen): investigate why need to wait on failure.
      asyncTestCase.waitForAsync('waiting for failure reset');
      goog.Timer.callOnce(function() {asyncTestCase.continueTesting();}, 500);
    }
  };
  if (c.model_.loadingCount <= 0) {
    verify();
    return;
  }
  asyncTestCase.waitForAsync('waiting for layout switching');
  c.model_.onLoaded = function() {
    if (!c || c.isDisposed() || c.model_ != this) {
      return;
    }
    if (c.model_.loadingCount > 0) {
      asyncTestCase.waitForAsync('waiting for layout switching');
    } else {
      goog.Timer.callOnce(function() {
        asyncTestCase.continueTesting();
        verify();
      });
    }
  };
}


/**
 * Mock the configuration loding process.
 */
i18n.input.chrome.inputview.ConfigLoaderMock.init = function() {
  asyncTestCase = goog.testing.AsyncTestCase.createAndInstall();
  var ContextType = i18n.input.chrome.message.ContextType;
  var root = '/google3/i18n/input/javascript/chos/inputview/';

  inputview.getKeyboardConfig = function(callback) {
    callback({'a11ymode': isA11yMode, 'features': featureList});
  };
  i18n.input.chrome.inputview.Model.prototype.loadingCount = 0;
  i18n.input.chrome.inputview.Model.prototype.onLoaded = function() {};
  i18n.input.chrome.inputview.Model.prototype.loadConfig = function(code) {
    var configId = code.replace(/\..*$/, '');
    var url = root + 'config/' + configId + '.js';
    if (!goog.array.contains(this.loadingResources_, url)) {
      this.loadingResources_.push(url);
      ++this.loadingCount;
      var deferred = goog.net.jsloader.load(url);
      deferred.addCallback(function() {
        // Async call onLoaded to avoid loadingCount reduced to 0 unexpectely,
        // because when config is loaded, controller may trigger layout loading.
        goog.Timer.callOnce((function() {
          --this.loadingCount;
          this.onLoaded();
        }).bind(this));
      }, this);
    }
  };
  i18n.input.chrome.inputview.Model.prototype.loadLayout = function(layout) {
    var url = root + 'layouts/' + layout + '.js';
    if (!goog.array.contains(this.loadingResources_, url)) {
      this.loadingResources_.push(url);
      ++this.loadingCount;
      var deferred = goog.net.jsloader.load(url);
      deferred.addCallback(function() {
        goog.Timer.callOnce((function() {
          --this.loadingCount;
          this.onLoaded();
        }).bind(this));
      }, this);
    }
  };
  i18n.input.chrome.vk.Model.loadLayoutScript_ = function(layoutCode) {
    // loadLayoutScript_() maybe called in controller's constructor, so use
    // async loader to make sure controller instance is not undefined.
    goog.Timer.callOnce(function() {
      var model = controller.model_;
      ++model.loadingCount;
      var deferred = goog.net.jsloader.load(
          root + '../vk/layouts/' + layoutCode + '.js');
      deferred.addCallback(function() {
        goog.Timer.callOnce((function() {
          --this.loadingCount;
          this.onLoaded();
        }).bind(this));
      }, model);
    });
  };
};
