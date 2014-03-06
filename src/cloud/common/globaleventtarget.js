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
 * @fileoverview Defines the global event target.
 */

goog.provide('i18n.input.common.events.GlobalEventTarget');

goog.require('goog.events.EventTarget');



/**
 * The global event target used to expose the events from Plugin level to
 * Controller level.
 *
 * @extends {goog.events.EventTarget}
 * @constructor
 */
i18n.input.common.events.GlobalEventTarget = function() {
  goog.base(this);
};
goog.inherits(i18n.input.common.events.GlobalEventTarget,
    goog.events.EventTarget);
goog.addSingletonGetter(i18n.input.common.events.GlobalEventTarget);
