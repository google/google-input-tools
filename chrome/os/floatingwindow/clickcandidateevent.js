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
goog.provide('i18n.input.chrome.ClickCandidateEvent');

goog.require('goog.events.Event');
goog.require('i18n.input.chrome.EventType');



/**
 * The clicking candiate event, which is fired by candidate window.
 *
 * @param {number} index The candidate's index in the current page.
 * @constructor
 * @extends {goog.events.Event}
 */
i18n.input.chrome.ClickCandidateEvent = function(index) {
  goog.base(this, i18n.input.chrome.EventType.CLICK_CANDIDATE_EVENT);

  /**
   * The index of the candidate in the current page.
   *
   * @type {number}
   */
  this.index = index;
};
goog.inherits(i18n.input.chrome.ClickCandidateEvent, goog.events.Event);
