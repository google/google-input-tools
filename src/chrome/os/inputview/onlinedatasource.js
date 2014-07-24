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
goog.provide('i18n.input.chrome.inputview.OnlineDataSource');

goog.require('goog.array');
goog.require('i18n.input.chrome.DataSource');
goog.require('i18n.input.net.JsonpService');



/**
 * The offline data source.
 *
 * @param {number} numOfCandidate The number of canddiate to fetch.
 * @constructor
 * @extends {i18n.input.chrome.DataSource}
 */
i18n.input.chrome.inputview.OnlineDataSource = function(numOfCandidate) {
  goog.base(this, numOfCandidate);

  /**
   * The jsonp service.
   *
   * @type {!i18n.input.net.JsonpService}
   * @private
   */
  this.jsonp_ = new i18n.input.net.JsonpService(
      i18n.input.chrome.DataSource.SERVER_URI_);
};
goog.inherits(i18n.input.chrome.inputview.OnlineDataSource,
    i18n.input.chrome.DataSource);


/**
 * The jsonp service.
 *
 * @type {string}
 * @private
 */
i18n.input.chrome.DataSource.SERVER_URI_ =
    'https://inputtools.google.com/';


/**
 * The path for the auto-complete.
 *
 * @type {string}
 * @private
 */
i18n.input.chrome.DataSource.AUTO_COMPLETE_PATH_ = 'request';


/**
 * The path for the auto-prediction.
 *
 * @type {string}
 * @private
 */
i18n.input.chrome.DataSource.AUTO_PREDICTION_PATH_ = 'predict';


/** @override */
i18n.input.chrome.inputview.OnlineDataSource.prototype.isReady = function() {
  return true;
};


/** @override */
i18n.input.chrome.inputview.OnlineDataSource.prototype.sendCompletionRequest =
    function(query) {
  var payload = this.createCommonPayload();
  payload['text'] = query;
  this.jsonp_.send(i18n.input.chrome.DataSource.AUTO_COMPLETE_PATH_,
      payload, goog.bind(this.onResponse_, this, true));
};


/** @override */
i18n.input.chrome.inputview.OnlineDataSource.prototype.sendPredictionRequest =
    function(query) {
  var payload = this.createCommonPayload();
  payload['text'] = query;
  this.jsonp_.send(i18n.input.chrome.DataSource.AUTO_PREDICTION_PATH_,
      payload, goog.bind(this.onResponse_, this, false));
};


/**
 * Callback for when response of auto-complete is back.
 *
 * @param {boolean} isAutoComplete True if this request is auto-complete.
 * @param {boolean} isSuccess True if the request succeed.
 * @param {Object} responseJSON The response.
 * @private
 */
i18n.input.chrome.inputview.OnlineDataSource.prototype.onResponse_ = function(
    isAutoComplete, isSuccess, responseJSON) {
  var data = responseJSON;
  var candidates = [];
  // Pushes the orginal query as the first candidate.
  if (data.length >= 2) {
    candidates = data[1][0][1];
    candidates = goog.array.splice(candidates, 0, this.numOfCandidate);
    if (isAutoComplete) {
      goog.array.insertAt(candidates, data[1][0][0], 0);
      this.dispatchEvent(new i18n.input.chrome.DataSource.
          CompletionBackEvent(candidates));
    } else {
      this.dispatchEvent(new i18n.input.chrome.DataSource.
          PredictionBackEvent(candidates));
    }

  }
};

