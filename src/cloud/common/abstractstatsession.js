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
goog.provide('i18n.input.common.AbstractStatSession');

goog.require('goog.Disposable');
goog.require('goog.functions');



/**
 * The class for Statistics' internal Sessioni object.
 *
 * @param {string} type The session type.
 * @constructor
 * @extends {goog.Disposable}
 */
i18n.input.common.AbstractStatSession = function(type) {
};
goog.inherits(i18n.input.common.AbstractStatSession, goog.Disposable);


/**
 * The empty function.
 *
 * @param {...*} var_args Optional trailing arguments. These are ignored.
 * @return {*} .
 * @private
 */
i18n.input.common.AbstractStatSession.nullFunction_ = function(var_args) {
};


/**
 * Sets the input tool code.
 *
 * @param {string} code The input tool code.
 */
i18n.input.common.AbstractStatSession.prototype.setInputToolCode =
    goog.functions.NULL;


/**
 * Increases the metrics number by 1.
 *
 * @param {string} param The metrics parameter name.
 */
i18n.input.common.AbstractStatSession.prototype.increase = goog.functions.NULL;


/**
 * Gets the metrics value per given parameter name.
 *
 * @param {string} param The metrics parameter name.
 * @return {string|number|undefined} The metrics value.
 */
i18n.input.common.AbstractStatSession.prototype.get = goog.nullFunction;


/**
 * Sets the metrics number or string value.
 *
 * @param {string} param The metrics parameter name.
 * @param {string|number} value The number or string value.
 */
i18n.input.common.AbstractStatSession.prototype.set = goog.functions.NULL;


/**
 * Sets the const metrics number or string value.
 *
 * @param {string} param The metrics parameter name.
 * @param {string|number} value The number or string value.
 */
i18n.input.common.AbstractStatSession.prototype.setConst = goog.functions.NULL;


/**
 * Sets the begin time of the metrics.
 *
 * @param {string} param The metrics parameter name.
 */
i18n.input.common.AbstractStatSession.prototype.begin = goog.functions.NULL;


/**
 * Sets the end time of the metrics.
 *
 * @param {string} param The metrics parameter name.
 */
i18n.input.common.AbstractStatSession.prototype.end = goog.functions.NULL;


/**
 * Pushes a value to the metrics. The pushed values will be in the format as
 * "x_x_x_...".
 *
 * @param {string} name The metrics parameter name.
 * @param {string|number} value The value to be pushed into the metrics.
 */
i18n.input.common.AbstractStatSession.prototype.push = goog.functions.NULL;


/**
 * Pops a value from the pushed metrics.
 *
 * @param {string} name The metrics parameter name.
 * @return {string} The pop'ed value of the metrics.
 */
i18n.input.common.AbstractStatSession.prototype.pop =
    goog.functions.withReturnValue(goog.nullFunction, '');


/**
 * Replaces the metrics with the given metrics object. It can be used to clean
 * up all the metrics info by replace it with a empty object. And it can
 * also be used to shift out the metrics object so that it can be saved in
 * other place (e.g. IME precommit queue) and later put it back to the stat
 * session.
 *
 * @param {!Object=} opt_newMetrics The new metrics object. If not provided,
 *     empty metrics object will replace the original metrics.
 * @return {!Object} The original metrics object.
 */
i18n.input.common.AbstractStatSession.prototype.replaceMetrics =
    goog.functions.withReturnValue(goog.nullFunction, {});


/**
 * Reports the metrics to the server.
 */
i18n.input.common.AbstractStatSession.prototype.report = goog.functions.NULL;

