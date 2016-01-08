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
goog.provide('i18n.input.chrome.xkb.JsDataSource');

goog.require('goog.array');
goog.require('goog.object');
goog.require('i18n.input.chrome.DataSource');
goog.require('i18n.input.chrome.message.Name');
goog.require('i18n.input.offline.latin.Decoder');

goog.scope(function() {
var Name = i18n.input.chrome.message.Name;



/**
 * The js data source.
 *
 * @param {number} numOfCandidate The number of canddiate to fetch.
 * @param {function(string, !Array.<!Object>)} candidatesCallback .
 * @param {function(!Object)} gestureCallback .
 * @constructor
 * @extends {i18n.input.chrome.DataSource}
 */
i18n.input.chrome.xkb.JsDataSource = function(numOfCandidate,
    candidatesCallback, gestureCallback) {
  goog.base(this, numOfCandidate, candidatesCallback, gestureCallback);
};
var JsDataSource = i18n.input.chrome.xkb.JsDataSource;
goog.inherits(JsDataSource, i18n.input.chrome.DataSource);


/** @private {!i18n.input.offline.latin.Decoder} */
JsDataSource.prototype.decoder_;


/** @override */
JsDataSource.prototype.setLanguage = function(language) {
  goog.base(this, 'setLanguage', language);

  this.decoder_ = new i18n.input.offline.latin.Decoder(language + '-t-i0-und',
      this.onLoaded_.bind(this));
};


/**
 * Callback when data source is loaded.
 *
 * @private
 */
JsDataSource.prototype.onLoaded_ = function() {
  this.ready = true;
};


/** @override */
JsDataSource.prototype.sendCompletionRequest = function(query, context,
    opt_spatialData) {
  var candidates = this.decoder_.decode(query, this.numOfCandidate);
  candidates = goog.array.map(candidates, function(candidate, index) {
    return goog.object.create(Name.CANDIDATE, candidate,
        Name.CANDIDATE_ID, index,
        Name.MATCHED_LENGTHS, query.length,
        Name.IS_EMOJI, false);
  });
  this.candidatesCallback(query, candidates || []);
};


/** @override */
JsDataSource.prototype.sendPredictionRequest = function(context) {
  this.candidatesCallback('', []);
};


/**
 * @param {?Object} keyboardLayout .
 */
JsDataSource.prototype.sendKeyboardLayout = function(keyboardLayout) {
};


/**
 * @param {!Array.<!Object>} gestureData .
 * @param {string} precedingText .
 */
JsDataSource.prototype.decodeGesture = function(gestureData, precedingText) {
};


/** @override */
JsDataSource.prototype.clear = function() {};


(function() {
  goog.exportSymbol('xkb.Model', JsDataSource);
  var modelProto = JsDataSource.prototype;
  goog.exportProperty(modelProto, 'setLanguage', modelProto.
      setLanguage);
  goog.exportProperty(modelProto, 'sendCompletionRequest',
      modelProto.sendCompletionRequest);
  goog.exportProperty(modelProto, 'sendPredictionRequest',
      modelProto.sendPredictionRequest);
  goog.exportProperty(modelProto, 'setCorrectionLevel',
      modelProto.setCorrectionLevel);
  goog.exportProperty(modelProto, 'setEnableUserDict',
      modelProto.setEnableUserDict);
  goog.exportProperty(modelProto, 'changeWordFrequency',
      modelProto.changeWordFrequency);
  goog.exportProperty(modelProto, 'clear',
      modelProto.clear);
  goog.exportProperty(modelProto, 'sendKeyboardLayout',
      modelProto.sendKeyboardLayout);
  goog.exportProperty(modelProto, 'decodeGesture',
      modelProto.decodeGesture);
  goog.exportProperty(modelProto, 'isReady',
      modelProto.isReady);
}) ();


});  // goog.scope

