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
/**
 * Provides mock implementation of Chrome extension APIs for testing.
 */

goog.provide('i18n.input.chrome.inputview.testing.MockExtensionApi');

if (!chrome) {
  /**
   * @type {!Object}
   */
  chrome = {};
}

/**
 * Adds mocks for chrome extension API calls.
 *
 * @param {goog.testing.MockControl} mockControl Controller for validating
 *     calls with expectations.
 */
function mockExtensionApis(mockControl) {

  var addMocks = function(base, name, methods) {
    base[name] = {};
    methods.forEach(function(m) {
      var fn = base[name][m] = mockControl.createFunctionMock(m);
      // Method to arm triggering a callback function with the specified
      // arguments. As it is awkward to require an exact signature match for
      // methods with a callback, the verification process is relaxed to simply
      // require that the sole argument is a function for each recorded call.
      fn.$callbackData = function(args) {
        fn().$anyTimes().$does(function(callback) {
          callback.call(undefined, args);
        });
        fn.$verifyCall = function(currentExpectation, name, args) {
          return args.length == 1 && typeof args[0] == 'function';
        };
      };
    });
  };

  addMocks(chrome, 'virtualKeyboardPrivate', ['getKeyboardConfig']);
  addMocks(chrome, 'inputMethodPrivate', ['getInputMethods',
      'setCurrentInputMethod']);
  addMocks(chrome, 'runtime', ['getBackgroundPage']);
  addMocks(chrome.runtime, 'onMessage', ['addListener']);
  // Ignore the sendMessage first, added it back when we need to check the
  // message calls.
  chrome.runtime.sendMessage = function(msg) {};
  // Ignore calls to addListener. Reevaluate if important to properly track the
  // flow of events.
  chrome.runtime.onMessage.addListener = function() {};
}

