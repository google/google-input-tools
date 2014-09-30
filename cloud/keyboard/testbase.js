// Copyright 2014 The Cloud Input Tools Authors. All Rights Reserved.
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
// All Rights Reserved.

/**
 * @fileoverview The test base which should be included in *_test.html files.
 *     This testbase.js is a temporary solution for keyboard unit tests.
 *     It will be replaced by the testbase.js under //i18n/input/javascript/.
 *     TODO(shuchen): remove this when we move //i18n/t13n/javascript to
 *     //i18n/input/javascript.
 */

(function() {
  var nocache = (window.location.href.toLowerCase().indexOf('nocache') != -1);

  var writeTestScript = function(src) {
    if (nocache) {
      var a = new Date();
      src += '?a=' + a.getTime();
    }
    document.write(
        '<script type="text/javascript" src="' + src + '"></script>');
  };

  var scripts = document.getElementsByTagName('script');
  var testbaseDir = './';
  var testbaseFilename = 'testbase.js';
  var testbaseFilenameLen = testbaseFilename.length;
  for (var i = 0; i < scripts.length; i++) {
    var src = scripts[i].src;
    var len = src.length;
    if (src.indexOf('/testbase.js') != -1) {
      testbaseDir = src.replace(/testbase.js.*/, '');
      break;
    }
  }
  var VK_JS_PATH = testbaseDir;
  var GOOGLE3_PATH = testbaseDir + '../../../../';
  var CLOSURE_BASE_PATH = GOOGLE3_PATH + 'javascript/closure/';
  var VK_JSBIN_PATH = GOOGLE3_PATH +
      'blaze-bin/i18n/input/javascript/keyboard/';

  writeTestScript(CLOSURE_BASE_PATH + 'base.js');
  writeTestScript(VK_JSBIN_PATH + 'deps.js');
  writeTestScript(VK_JS_PATH + 'deps-runfiles.js');
})();
