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
var controller;

function registerFunctionMock(path, opt_fn) {
  var parts = path.split('.');
  var base = window;
  var part = null;
  var fn = opt_fn;
  if (!fn) {
    fn = function() {
      var prettyprint = function(arg) {
        if (arg instanceof Array) {
          var terms = [];
          for (var i = 0; i < arg.length; i++) {
            terms.push(prettyprint(arg[i]));
          }
          return '[' + terms.join(', ') + ']';
        } else if (typeof arg == 'object') {
          var properties = [];
          for (var key in arg) {
             properties.push(key + ': ' + prettyprint(arg[key]));
          }
          return '{' + properties.join(', ') + '}';
        } else {
          return arg;
        }
      };
      // The property 'arguments' is an array-like object. Convert to a true
      // array for prettyprinting.
      var args = Array.prototype.slice.call(arguments);
      console.log('Call to ' + path + ': ' + prettyprint(args));
    };
  }
  for (var i = 0; i < parts.length - 1; i++) {
    part = parts[i];
    if (!base[part]) {
      base[part] = {};
    }
    base = base[part];
  }
  base[parts[parts.length - 1]] = fn;
}

registerFunctionMock('chrome.runtime.onMessage.addListener');
registerFunctionMock('chrome.i18n.getMessage', function() {
  return arguments[0];
});
registerFunctionMock('chrome.runtime.getBackgroundPage', function() {
  var callback = arguments[0];
  callback();
});
registerFunctionMock('chrome.input.ime.hideInputView');
registerFunctionMock('chrome.runtime.sendMessage');

window.onload = function() {
  var params = {};
  var matches = window.location.href.match(/[#?].*$/);
  if (matches && matches.length > 0) {
    matches[0].slice(1).split('&').forEach(function(s) {
      var pair = s.split('=');
      params[pair[0]] = pair[1];
    });
  }

  var code = params['id'] || 'us';
  var language = params['language'] || 'en';
  var passwordLayout = params['passwordLayout'] || 'us';
  var name = params['name'] || 'English';

  i18n.input.chrome.inputview.Controller.DEV = true;
  controller = new i18n.input.chrome.inputview.Controller(code,
      language, passwordLayout, name);

  // Poll status of the keyboard to determine when loading is complete.
  var keyboardPollReadyCallback = function() {
    if (controller.isSettingReady &&
        Object.keys(controller.layoutDataMap_).length > 2) {
      controller.resize(true);
    } else {
      pollKeyboardReady(keyboardPollReadyCallback);
    }
  };
  var pollKeyboardReady = function(callback) {
    window.setTimeout(function() {
      callback();
    }, 100);
  };
  pollKeyboardReady(keyboardPollReadyCallback);
};
