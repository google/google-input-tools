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
goog.require('i18n.input.chrome.inputview.Controller');


(function() {
  window.onload = function() {
    var uri = new goog.Uri(window.location.href);
    var code = uri.getParameterValue('id') || 'us';
    var language = uri.getParameterValue('language') || 'en';
    var passwordLayout = uri.getParameterValue('passwordLayout') || 'us';
    var name = chrome.i18n.getMessage(
        uri.getParameterValue('name') || 'English');

    var controller = new i18n.input.chrome.inputview.Controller(code,
        language, passwordLayout, name);
    window.unload = function() {
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

