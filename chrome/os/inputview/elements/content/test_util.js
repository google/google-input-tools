// Copyright 2016 The ChromeOS IME Authors. All Rights Reserved.
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
goog.provide('i18n.input.chrome.inputview.elements.content.TestUtil');

goog.require('i18n.input.chrome.inputview.events.DragEvent');


/**
 * Creates a drag event for testing.
 *
 * @param {number} x The x coordinate.
 * @param {number} y The y coordinate.
 * @param {number=} opt_identifier The identifier for the drag event.
 * @return {!i18n.input.chrome.inputview.events.DragEvent} The created drag
 *     event.
 */
i18n.input.chrome.inputview.elements.content.TestUtil.makeDragEvent =
    function(x, y, opt_identifier) {
  var identifier = -1;
  if (opt_identifier != undefined) {
    identifier = opt_identifier;
  }
  return new i18n.input.chrome.inputview.events.DragEvent(
      null, 0, null, x, y, 1, 1, identifier);
};
