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
goog.provide('i18n.input.chrome.xkb.Model');

goog.require('goog.array');
goog.require('goog.events.EventHandler');
goog.require('goog.object');
goog.require('i18n.input.chrome.DataSource');
goog.require('i18n.input.chrome.MessageKey');
goog.require('i18n.input.chrome.message.Name');
goog.require('i18n.input.dom.NaclModule');



goog.scope(function() {
var EventHandler = goog.events.EventHandler;
var MessageKey = i18n.input.chrome.MessageKey;
var NaclModule = i18n.input.dom.NaclModule;
var Name = i18n.input.chrome.message.Name;



/**
 * The model, which manages communications with the backend decoder.
 *
 * @param {number} numOfCandidate The number of candidate to fetch.
 * @param {function(string, !Array.<!Object>)} candidatesCallback .
 * @param {function(!Array.<string>)} gestureCallback .
 * @constructor
 * @extends {i18n.input.chrome.DataSource}
 */
i18n.input.chrome.xkb.Model = function(numOfCandidate, candidatesCallback,
    gestureCallback) {
  goog.base(this, numOfCandidate, candidatesCallback, gestureCallback);

  /**
   * The NaCl module for language model.
   *
   * @type {!NaclModule}
   * @private
   */
  this.naclModule_ = NaclModule.getInstance();

  /**
   * The spatial cache.
   *
   * @type {!Object.<string, !Object.<string, !Array.<number|string>>>}
   * @private
   */
  this.spatialCache_ = {};

  /**
   * The candidates.
   *
   * @type {!Array.<!Object>}
   */
  this.candidates = [];

  /**
   * The event handler.
   *
   * @private {!EventHandler}
   */
  this.eventHandler_ = new EventHandler(this);
};
goog.inherits(i18n.input.chrome.xkb.Model,
    i18n.input.chrome.DataSource);
var Model = i18n.input.chrome.xkb.Model;


/**
 * The current source.
 *
 * @type {string}
 * @private
 */
Model.prototype.context_ = '';


/**
 * The current context.
 *
 * @type {string}
 * @private
 */
Model.prototype.source_ = '';


/** @override */
Model.prototype.setLanguage = function(language) {
  goog.base(this, 'setLanguage', language);

  this.eventHandler_.
      listen(this.naclModule_, NaclModule.EventType.LOAD, this.onNaclLoaded_).
      listen(this.naclModule_, NaclModule.EventType.CRASH, this.onNaclCrash_);

  if (this.naclModule_.isAlive()) {
    this.onNaclLoaded_();
  }

  this.updateNaclSettings_();
};


/** @override */
Model.prototype.sendCompletionRequestForWord =
    function(word, context) {
  if (!this.isReady()) {
    return;
  }
  var messageObj = {};
  messageObj[MessageKey.SOURCE] = word;
  messageObj[MessageKey.APPEND] = word;
  messageObj[MessageKey.CONTEXT] = context.trim();
  messageObj[MessageKey.MULTI] = false;
  this.naclModule_.postMessage(
      messageObj, this.onCandidatesResponse_.bind(this));
};


/**
 * Callback when nacl module crashed.
 *
 * @private
 */
Model.prototype.onNaclCrash_ = function() {
  this.ready = false;
};


/**
 * Callback for NACL loaded.
 *
 * @private
 */
Model.prototype.onNaclLoaded_ = function() {
  this.ready = true;
  this.updateNaclSettings_();
};


/** @override */
Model.prototype.sendCompletionRequest =
    function(source, context, opt_spatialData) {
  if (!this.isReady()) {
    return;
  }
  var messageObj = {};
  messageObj[MessageKey.SOURCE] = source;

  if (opt_spatialData) {
    // Sets spatial character map.
    var fuzzyCh = {};
    fuzzyCh['source'] = opt_spatialData['sources'];
    fuzzyCh['score'] = opt_spatialData['possibilities'];
    this.spatialCache_[source.slice(-1)] =
        /** @type {!Object.<string, !Array.<number|string>>} */ (fuzzyCh);

    // Appends the characters.
    messageObj[MessageKey.MULTI_APPEND] = this.spatialCache_[source.slice(-1)];
    if (source.length == 1) {
      messageObj[MessageKey.CONTEXT] = context.trim();
    }
  } else {
    // Reverts the source.
    messageObj[MessageKey.REVERT] = 1;
  }
  this.naclModule_.postMessage(
      messageObj, this.onCandidatesResponse_.bind(this));
};


/** @override */
Model.prototype.sendPredictionRequest = function(context) {
  if (this.isReady()) {
    this.clear();
    var messageObj = {};
    messageObj[MessageKey.CONTEXT] = context.trim();
    messageObj[MessageKey.PREDICT] = true;
    this.naclModule_.postMessage(
        messageObj, this.onCandidatesResponse_.bind(this));
  }
};


/** @override **/
Model.prototype.changeWordFrequency = function(word, frequency) {
  if (this.isReady()) {
    var messageObj = {};
    messageObj[MessageKey.COMMIT_WORD] = word;
    messageObj[MessageKey.FREQUENCY] = frequency;
    this.naclModule_.postMessage(messageObj);
  }
};


/**
 * Callback for when response is received.
 *
 * @param {!Array} data The response.
 * @private
 */
Model.prototype.onCandidatesResponse_ = function(data) {
  if (data.length >= 2) {
    var source = data[1][0][0];
    var items = data[1][0][2];
    var candidates = items ? items.map(function(item) {
      return item[0];
    }) : [];
    var matchedLengths = items ? items.map(function(item) {
      return item[1];
    }) : [];
    // Removes duplicated candidates.
    goog.array.removeDuplicates(candidates, undefined, function(candidate) {
      return candidate.replace(/ $/, '');
    });
    this.candidates = goog.array.map(candidates, function(candidate, index) {
      return goog.object.create(
          Name.CANDIDATE, candidate,
          Name.CANDIDATE_ID, index,
          Name.MATCHED_LENGTHS, matchedLengths[index],
          Name.IS_EMOJI, false);
    });
    // Convert Emoji candiates.
    this.candidates = goog.array.map(this.candidates, function(candidate,
        index) {
          var c = candidate[Name.CANDIDATE];
          if (c.indexOf('@Emoji@') == 0) {
            candidate[Name.IS_EMOJI] = true;
            c = c.slice(7);
            if (c.indexOf('0x') == 0) {
              // Multiple Unicode Emoji.
              var ret = '';
              var pos = 0;
              while (pos < c.length) {
                var ch = c.slice(pos, pos + 6);
                ret += String.fromCharCode(parseInt(ch, 16));
                pos += 6;
              }
              c = ret;
            }
            candidate[Name.CANDIDATE] = c;
          }

          return candidate;
        });
    if (source) {
      // Removes the candidates whose matched length doesn't equal to the length
      // of source.
      this.candidates = goog.array.filter(this.candidates, function(c, i) {
        // Engine returns wrong length if source contains ' or -.
        // So remove the ' and - in source and then compare the length with
        // matched_length. If matched_lengths[i] is undefined, don't filter
        // out the candidate.
        return matchedLengths[i] == undefined ||
            matchedLengths[i] == source.length;
      });
      if (source.toLowerCase() != source) {
        var allCaps = source.length > 1 && source.toUpperCase() == source;
        var initCap = !allCaps &&
            source.charAt(0).toUpperCase() == source.charAt(0);
        // Makes sure the candidates align with upper case pattern as source.
        this.candidates = goog.array.map(this.candidates, function(candidate,
            index) {
              var c = candidate[Name.CANDIDATE];
              if (allCaps) {
                candidate[Name.CANDIDATE] = c.toUpperCase();
              } else if (initCap) {
                // Initial caps.
                candidate[Name.CANDIDATE] = c.charAt(0).toUpperCase() +
                    c.slice(1);
              }
              return candidate;
            });
        goog.array.removeDuplicates(this.candidates, undefined, function(v) {
          return v[Name.CANDIDATE];
        });
        // Adjusts the top 2 candidates' order for special uppercase cases.
        if (!allCaps && !initCap && this.candidates.length > 1 &&
            this.candidates[0][Name.CANDIDATE] != source) {
          var first = this.candidates[0][Name.CANDIDATE];
          var second = this.candidates[1][Name.CANDIDATE];
          if (source.toLowerCase() == second.toLowerCase()) {
            this.candidates[1][Name.CANDIDATE] = first;
            this.candidates[0][Name.CANDIDATE] = source;
          } else if (source.toLowerCase() == first.toLowerCase()) {
            this.candidates[0][Name.CANDIDATE] = source;
          }
        }
      }
      // More cases to put source as the top candidate.
      if (this.candidates.length > 0 &&
          this.candidates[0][Name.CANDIDATE] != source && (
          /^en-?/.test(this.language) && !/^[a-zA-Z'\-]+$/.test(source) ||
          this.correctionLevel == 0)) {
        var sourceCandidate = goog.object.clone(this.candidates[0]);
        sourceCandidate[Name.CANDIDATE] = source;
        this.candidates.unshift(sourceCandidate);
        goog.array.removeDuplicates(this.candidates, undefined, function(v) {
          return v[Name.CANDIDATE];
        });
      }
    }

    this.candidatesCallback(source, this.candidates);
  }
};


/**
 * Callback for when gesture decoder response is received.
 *
 * @param {!Array} data The response.
 * @private
 */
Model.prototype.onGestureResponse_ = function(data) {
  if (data.length < 2) {
    return;
  }

  // Extract just the words.
  var results = [];
  var resultsArray = data[1];
  for (var i = 0; i < resultsArray.length; i++) {
    // The zero-th item is the decoded word. The other two elements are
    // decoding scores which are not of concern to the front end.
    results.push(resultsArray[i][0]);
  }

  this.gestureCallback(results);
};


/** @override */
Model.prototype.clear = function() {
  var messageObj = {};
  messageObj[MessageKey.CLEAR] = true;
  this.naclModule_.postMessage(messageObj);
};


/** @override */
Model.prototype.setCorrectionLevel = function(level) {
  goog.base(this, 'setCorrectionLevel', level);

  this.clear();
  var messageObj = {};
  messageObj[MessageKey.IME] = this.getInputToolCode();
  messageObj[MessageKey.CORRECTION] = this.correctionLevel;
  this.naclModule_.postMessage(messageObj);
};


/** @override */
Model.prototype.setEnableUserDict = function(enabled) {
  goog.base(this, 'setEnableUserDict', enabled);

  this.clear();
  var messageObj = {};
  messageObj[MessageKey.IME] = this.getInputToolCode();
  messageObj[MessageKey.ENABLE_USER_DICT] = this.enableUserDict;
  this.naclModule_.postMessage(messageObj);
};


/**
 * Updates the settings of the nacl module.
 *
 * @private
 */
Model.prototype.updateNaclSettings_ = function() {
  this.clear();
  var messageObj = {};
  messageObj[MessageKey.IME] = this.getInputToolCode();
  messageObj[MessageKey.CORRECTION] = this.correctionLevel;
  messageObj[MessageKey.ENABLE_USER_DICT] = this.enableUserDict;
  this.naclModule_.postMessage(messageObj);
};


/**
 * Sends a keyboard layout to the gesture typing decoder.
 *
 * @param {?Object} keyboardLayout .
 */
Model.prototype.sendKeyboardLayout = function(keyboardLayout) {
  var messageObj = {};
  // TODO verify that keyboardLayout has the correct fields
  this.clear();

  messageObj[MessageKey.IME] = this.getInputToolCode();
  messageObj[MessageKey.KEYBOARD_LAYOUT] = keyboardLayout;
  this.naclModule_.postMessage(messageObj);
};


/**
 * Sends a gesture typing trail to the backend for decoding.
 *
 * @param {!Array.<!Object>} gestureData .
 */
Model.prototype.decodeGesture = function(gestureData) {
  var messageObj = {};
  messageObj[MessageKey.IME] = this.getInputToolCode();
  messageObj[MessageKey.DECODE_GESTURE] = {
    'touch_data': {
      'touch_points': gestureData
    }
  };
  this.naclModule_.postMessage(messageObj, this.onGestureResponse_.bind(this));
};


/** @override */
Model.prototype.isReady = function() {
  // XKB model may not catch onNaclLoad/onNaclCrash events from NaclModule
  // because NaclModule is a singleton which is also used by Latin controller.
  // Therefore, XKB model's isReady() cannot use its |ready| flag, instead,
  // should use NaclModule.isAlive() all the time.
  return this.naclModule_.isAlive();
};


/** @override */
Model.prototype.disposeInternal = function() {
  goog.dispose(this.eventHandler_);
  goog.base(this, 'disposeInternal');
};


(function() {
  goog.exportSymbol('xkb.Model', Model);
  var modelProto = Model.prototype;
  goog.exportProperty(modelProto, 'setLanguage', modelProto.
      setLanguage);
  goog.exportProperty(modelProto, 'sendCompletionRequest',
      modelProto.sendCompletionRequest);
  goog.exportProperty(modelProto, 'sendCompletionRequestForWord',
      modelProto.sendCompletionRequestForWord);
  goog.exportProperty(modelProto, 'sendPredictionRequest',
      modelProto.sendPredictionRequest);
  goog.exportProperty(modelProto, 'setCorrectionLevel',
      modelProto.setCorrectionLevel);
  goog.exportProperty(modelProto, 'setEnableUserDict',
      modelProto.setEnableUserDict);
  goog.exportProperty(modelProto, MessageKey.CLEAR,
      modelProto.clear);
  goog.exportProperty(modelProto, 'changeWordFrequency', modelProto.
      changeWordFrequency);
  goog.exportProperty(modelProto, 'sendKeyboardLayout',
      modelProto.sendKeyboardLayout);
  goog.exportProperty(modelProto, 'decodeGesture',
      modelProto.decodeGesture);
  goog.exportProperty(modelProto, 'isReady',
      modelProto.isReady);
}) ();


});  // goog.scope

