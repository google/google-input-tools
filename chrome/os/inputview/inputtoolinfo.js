// Copyright 2015 The ChromeOS IME Authors. All Rights Reserved.
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
goog.provide('i18n.input.lang.InputToolInfo');

goog.require('goog.array');
goog.require('i18n.input.lang.InputToolsData');
goog.require('proto.i18n_input.InputToolsProto');



/**
 * Input tool information.
 *
 * @constructor
 */
i18n.input.lang.InputToolInfo = function() {};


/**
 * The input tool data.
 *
 * @type {!Object.<string, !proto.i18n_input.InputToolsProto>}
 */
i18n.input.lang.InputToolInfo.DATA = {};


/**
 * Input tool native name and input tool code mapping.
 *
 * @type {!Object.<string, string>}
 */
i18n.input.lang.InputToolInfo.INPUT_TOOL_NAME = {};


/**
 * Language code and supported input tool list mapping.
 *
 * @type {!Object.<string, !Array.<!proto.i18n_input.InputToolsProto>>}
 */
i18n.input.lang.InputToolInfo.LANGUGAGE_INPUT_TOOL = {};


(function() {
  goog.array.forEach(i18n.input.lang.InputToolsData[0], function(entry) {
    var info = new proto.i18n_input.InputToolsProto(entry);
    var code = info.getCode();
    if (code) {
      i18n.input.lang.InputToolInfo.DATA[code] = info;
      var name = info.getDisplayNameList()[0].getValue();
      if (name) {
        i18n.input.lang.InputToolInfo.INPUT_TOOL_NAME[code] = name;
      }
    }
    goog.array.forEach(info.getSupportedLanguageList(), function(lang) {
      if (lang == 'zh-TW') {
        lang = 'zh-Hant';
      } else if (lang == 'zh-CN') {
        lang = 'zh-Hans';
      }
      if (!i18n.input.lang.InputToolInfo.LANGUGAGE_INPUT_TOOL[lang]) {
        i18n.input.lang.InputToolInfo.LANGUGAGE_INPUT_TOOL[lang] = [];
      }
      i18n.input.lang.InputToolInfo.LANGUGAGE_INPUT_TOOL[lang].push(info);
    });
  });
})();
