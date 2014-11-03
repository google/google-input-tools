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
goog.provide('i18n.input.chrome.xkb.LatinInputToolCode');

goog.require('goog.object');
goog.require('i18n.input.lang.InputToolCode');


goog.scope(function() {
var InputToolCode = i18n.input.lang.InputToolCode;



/**
 * Object with keys being the set of all latin XKB input component ids.
 *
 * TODO: Find a better way to curate this list (currently just referring to
 *     google doc for Chrome OS Latin Keyboard Layouts).
 *
 * @type {!Object.<InputToolCode, boolean>}
 */
i18n.input.chrome.xkb.LatinInputToolCode = goog.object.createSet(
    InputToolCode.XKB_BE_FRA,
    InputToolCode.XKB_CA_FRA,
    InputToolCode.XKB_CA_ENG_ENG,
    InputToolCode.XKB_CA_MULTIX_FRA,
    InputToolCode.XKB_DE_GER,
    InputToolCode.XKB_DE_NEO_GER,
    InputToolCode.XKB_FI_FIN,
    InputToolCode.XKB_FR_FRA,
    InputToolCode.XKB_GB_DVORAK_ENG,
    InputToolCode.XKB_GB_EXTD_ENG,
    InputToolCode.XKB_IE_GA,
    InputToolCode.XKB_IS_ICE,
    InputToolCode.XKB_IT_ITA,
    InputToolCode.XKB_NO_NOB,
    InputToolCode.XKB_SE_SWE,
    InputToolCode.XKB_US_ENG,
    InputToolCode.XKB_US_FIL,
    InputToolCode.XKB_US_IND,
    InputToolCode.XKB_US_MSA,
    InputToolCode.XKB_US_ALTGR_INTL_ENG,
    InputToolCode.XKB_US_COLEMAK_ENG,
    InputToolCode.XKB_US_DVORAK_ENG,
    InputToolCode.XKB_US_INTL_ENG,
    InputToolCode.XKB_US_INTL_NLD,
    InputToolCode.XKB_US_INTL_POR);
});  // goog.scope
