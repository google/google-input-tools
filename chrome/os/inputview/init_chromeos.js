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
goog.require('goog.Uri');
goog.require('goog.object');
goog.require('i18n.input.chrome.inputview.Controller');
goog.require('i18n.input.chrome.message.Name');
goog.require('i18n.input.chrome.message.Type');


(function() {
  window.onload = function() {
    var code, language, passwordLayout, name;

    var fetchHashParam = function() {
      var hash = window.location.hash;
      if (!hash) {
        return false;
      }
      var param = {};
      hash.slice(1).split('&').forEach(function(s) {
        var pair = s.split('=');
        param[pair[0]] = pair[1];
      });
      code = param['id'] || 'us';
      language = param['language'] || 'en';
      passwordLayout = param['passwordLayout'] || 'us';
      name = chrome.i18n.getMessage(param['name'] || 'English');
      return true;
    };

    if (!fetchHashParam()) {
      var uri = new goog.Uri(window.location.href);
      code = uri.getParameterValue('id') || 'us';
      language = uri.getParameterValue('language') || 'en';
      passwordLayout = uri.getParameterValue('passwordLayout') || 'us';
      name = chrome.i18n.getMessage(uri.getParameterValue('name') || 'English');
    }

    var controller = new i18n.input.chrome.inputview.Controller(code,
        language, passwordLayout, name);

    window.onhashchange = function() {
      fetchHashParam();
      controller.resetAll();
      controller.initialize(code, language, passwordLayout, name);
    };

    window.onunload = function() {
      chrome.runtime.sendMessage(goog.object.create(
            i18n.input.chrome.message.Name.TYPE,
            i18n.input.chrome.message.Type.VISIBILITY_CHANGE,
            i18n.input.chrome.message.Name.VISIBILITY,
            false));
      goog.dispose(controller);
    };

    // Methods below are used for automation test.
    window.isKeyboardReady = function() {
      return controller.isKeyboardReady;
    };

    window.startTest = function() {
      controller.resize(true);
    };
  };
})();

