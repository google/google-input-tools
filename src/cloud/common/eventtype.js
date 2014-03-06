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
 * @fileoverview This file defines control event type, when control status
 * was changed. It will dispatch a control event, then pass to UI component
 * to react to the change.
 */

goog.provide('i18n.input.common.events.EventType');


/**
 * The enum of events which will be used to communicated with the controller.
 *
 * @enum {string}
 * @const
 */
i18n.input.common.events.EventType = {
  CURRENT_INPUT_TOOL_CHANGED: 'citc',
  EXECUTE_COMMAND: 'ecd',
  SHOW_EDITOR: 'se',
  HIDE_EDITOR: 'he',
  SHOW_PAD: 'sp',
  HIDE_PAD: 'hp',
  INPUT_TOOL_LIST_UPDATED: 'itlu',
  LOCALIZE: 'ita_l',
  PAGE_ELEMENT_LIST_UPDATED: 'pelu',
  SETTING_LINK_ACTION: 'slc',
  STATUS_BAR: 'sb',
  INIT_CHEXT: 'ic',
  RESUME_CHEXT: 'rc',
  SUSPEND_CHEXT: 'suc',
  SYNC_CHEXT: 'syc',
  UNSET_CHEXT: 'uc',
  UPDATE_FEATURE: 'uf',
  WIDGET: 'wg',
  PREF_SYNC_ACK: 'psa'
};
