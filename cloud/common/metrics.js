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

/**
 * @fileoverview Defines the metrics constants.
 */

goog.provide('i18n.input.common.Metrics');

goog.require('goog.object');
goog.require('i18n.input.common.CommandType');


/**
 * The metrics param names.
 *
 * @enum {string}
 */
i18n.input.common.Metrics.Param = {
  // Common param names.
  ACTION: 'act',
  SHOW_TIME: 'st',
  // StatusBar param names.
  BAR_LANG_KEY_COUNT: 'ltkc',
  BAR_LANG_CLICK_COUNT: 'ltcc',
  BAR_BC_KEY_COUNT: 'bckc',
  BAR_BC_CLICK_COUNT: 'bccc',
  BAR_PUNC_KEY_COUNT: 'ptkc',
  BAR_PUNC_CLICK_COUNT: 'ptcc',
  BAR_DRAG_COUNT: 'bdc',
  // PPE param names.
  PPE_CANDIDATE_INDEX: 'ci',
  PPE_SOURCE_LENGTH: 'slen',
  PPE_TARGET_LENGTH: 'tlen',
  PPE_SELECT_DURATION: 'dur',
  PPE_BACKSPACE_COUNT: 'bsc',
  PPE_COMMIT_KEY: 'key',
  // Suggest Menu param names.
  SGM_CANDIDATE_INDEX: 'ci',
  SGM_CANDIDATE_CLICK_COUNT: 'ccc',
  // VK param names.
  VK_LAYOUT: 'lay',
  VK_KEY_CLICK_COUNT: 'kcc',
  VK_KEY_KEY_COUNT: 'kkc',
  VK_WORD_COUNT: 'wc',
  VK_DRAG_COUNT: 'kdc',
  VK_MINIMIZED_TIME: 'mt',
  // Network param names.
  QUERY_LATENCY: 'ql',

  // 0 for stroke recognition. 1 for prediction. 2 for default punctuation.
  PREDICTION: 'pre',
  // 0 for online decoder. 1 for Nacl decoder. 2 for cache.
  OFFLINE_DECODER: 'od'
};


/**
 * The TYPE param values.
 *
 * @enum {string}
 */
i18n.input.common.Metrics.Type = {
  POPUP_EDITOR: 'ppe',
  STATUS_BAR: 'bar',
  SUGGEST_MENU: 'sgm',
  VIRTUAL_KEYBOARD: 'vk',
  HANDWRITING: 'hwt'
};


/**
 * The ACTION param values.
 *
 * @enum {string}
 */
i18n.input.common.Metrics.Action = {
  PAGE_UNLOAD: 'ul',
  CLOSE: 'cl',
  SWITCH: 'sw',
  // PPE actions.
  PPE_COMMIT_SOURCE: 'cmts',
  PPE_COMMIT_TARGET: 'cmtt',
  PPE_COMMIT_DELAY: 'cmtd'
};


/**
 * The status bar command and the metrics param names mapping.
 *
 * @type {!Object.<string, !Array.<string>>}
 */
i18n.input.common.Metrics.STATUSBAR_MAP = goog.object.create(
    i18n.input.common.CommandType.TOGGLE_LANGUAGE, [
      i18n.input.common.Metrics.Param.BAR_LANG_KEY_COUNT,
      i18n.input.common.Metrics.Param.BAR_LANG_CLICK_COUNT
    ], i18n.input.common.CommandType.TOGGLE_SBC, [
      i18n.input.common.Metrics.Param.BAR_BC_KEY_COUNT,
      i18n.input.common.Metrics.Param.BAR_BC_CLICK_COUNT
    ], i18n.input.common.CommandType.PUNCTUATION, [
      i18n.input.common.Metrics.Param.BAR_PUNC_KEY_COUNT,
      i18n.input.common.Metrics.Param.BAR_PUNC_CLICK_COUNT]);
