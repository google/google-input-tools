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
 * @fileoverview Defines the Statistics class.
 */

goog.provide('i18n.input.common.Statistics');

goog.require('goog.Disposable');
goog.require('goog.events.EventHandler');
goog.require('goog.events.EventType');
goog.require('i18n.input.common.AbstractStatSession');



/**
 * The statistics class which privide the functionalities to record metrics for
 * user behaviors on ITA UI, and report it to input server.
 *
 * @constructor
 * @extends {goog.Disposable}
 */
i18n.input.common.Statistics = function() {
  /**
   * The sessions, key is type, value is the object used to record the metrics.
   *
   * @type {!Object.<string, !i18n.input.common.AbstractStatSession>}
   * @private
   */
  this.sessions_ = {};

  /**
   * The event handler for page unload.
   *
   * @type {!goog.events.EventHandler}
   * @private
   */
  this.eventHandler_ = new goog.events.EventHandler(this);

  this.eventHandler_.listen(window,
      [goog.events.EventType.BEFOREUNLOAD, goog.events.EventType.UNLOAD],
      this.handleUnload_);
};
goog.inherits(i18n.input.common.Statistics, goog.Disposable);
goog.addSingletonGetter(i18n.input.common.Statistics);


/**
 * Gets a session.
 *
 * @param {string} type The session type.
 * @return {!i18n.input.common.AbstractStatSession} The created session
 *     instance.
 */
i18n.input.common.Statistics.prototype.getSession = function(type) {
  return this.sessions_[type] || new i18n.input.common.AbstractStatSession(
      type);
};


/**
 * Sets a session.
 *
 * @param {string} type The session type.
 * @param {!i18n.input.common.AbstractStatSession} session .
 */
i18n.input.common.Statistics.prototype.setSession = function(type, session) {
  if (this.sessions_[type]) {
    this.sessions_[type].dispose();
  }
  this.sessions_[type] = session;
};


/**
 * Handles the page unload event.
 *
 * @private
 */
i18n.input.common.Statistics.prototype.handleUnload_ = function() {
  this.dispose();
};


/** @override */
i18n.input.common.Statistics.prototype.disposeInternal = function() {
  goog.dispose(this.eventHandler_);

  for (var type in this.sessions_) {
    goog.dispose(this.sessions_[type]);
    delete this.sessions_[type];
  }

  goog.base(this, 'disposeInternal');
};
