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
goog.provide('i18n.input.chrome.xkb.Controller');

goog.require('goog.array');
goog.require('goog.events.EventHandler');
goog.require('goog.events.EventType');
goog.require('goog.object');
goog.require('i18n.input.chrome.AbstractController');
goog.require('i18n.input.chrome.DataSource');
goog.require('i18n.input.chrome.inputview.Statistics');
goog.require('i18n.input.chrome.inputview.events.KeyCodes');
goog.require('i18n.input.chrome.inputview.util');
goog.require('i18n.input.chrome.message.Name');
goog.require('i18n.input.chrome.message.Type');
goog.require('i18n.input.chrome.xkb.Correction');


goog.scope(function() {
var EventType = i18n.input.chrome.DataSource.EventType;
var Name = i18n.input.chrome.message.Name;
var Type = i18n.input.chrome.message.Type;
var KeyCodes = i18n.input.chrome.inputview.events.KeyCodes;
var util = i18n.input.chrome.inputview.util;
var AbstractController = i18n.input.chrome.AbstractController;


/**
 * The regex of characters need to convert two following spaces to period.
 *
 * @type {!RegExp}
 */
var REGEX_DOUBLE_SPACE_PERIOD_CHARACTERS =
    /[a-z0-9\u00E0-\u00F6\u00F8-\u017F'"%\)\]\}\>\+]\s\s$/i;


/**
 * The background page.
 *
 * @extends {i18n.input.chrome.AbstractController}
 * @constructor
 */
i18n.input.chrome.xkb.Controller = function() {
  /** @private {!goog.events.EventHandler} */
  this.handler_ = new goog.events.EventHandler(this);

  if (window.xkb && xkb.DataSource) {
    this.dataSource_ = new xkb.DataSource(5,
        this.onCandidatesBack_.bind(this));
    this.dataSource_.setCorrectionLevel(this.correctionLevel);
  }

  /**
   * The statistics object for recording metrics values.
   *
   * @type {!i18n.input.chrome.inputview.Statistics}
   * @private
   */
  this.statistics_ = i18n.input.chrome.inputview.Statistics.getInstance();

  /**
   * Weather or not enable double space to period feature.
   * @private {boolean}
   */
  // TODO: Double space to peroid feature should be controlled by IME
  // setting. Remove this flag once setting page is ready.
  this.enableDoubleSpacePeriod_ = true;

  /** @private {!Array.<string>} */
  this.candidates_ = [];
};
var Controller = i18n.input.chrome.xkb.Controller;
goog.inherits(Controller, i18n.input.chrome.AbstractController);


/**
 * The correction level.
 * 0: No auto-correction.
 * 1: Auto-correct only confident.
 * 2: Always auto-correct.
 *
 * @type {number}
 */
Controller.prototype.correctionLevel = 1;


/**
 * The last correction item.
 *
 * @private {i18n.input.chrome.xkb.Correction}
 */
Controller.prototype.lastCorrection_ = null;


/**
 * The data source.
 *
 * @private {xkb.DataSource}
 */
Controller.prototype.dataSource_ = null;


/**
 * The text before cursor.
 *
 * @type {string}
 * @private
 */
Controller.prototype.textBeforeCursor_ = '';


/**
 * The composing text.
 *
 * @private {string}
 */
Controller.prototype.compositionText_ = '';


/**
 * The dead key.
 *
 * @private {string}
 */
Controller.prototype.deadKey_ = '';


/**
 * The commit text.
 *
 * @type {string}
 * @private
 */
Controller.prototype.committedText_ = '';


/**
 * The surrounding text.
 *
 * @private {string}
 */
Controller.prototype.surroundingText_;


/**
 * The type of the last commit.
 *
 * @private {number}
 */
Controller.prototype.lastCommitType_ = -1;


/** @private {string} */
Controller.prototype.engineID_;


/** @override */
Controller.prototype.onSurroundingTextChanged = function(
    engineID, surroundingInfo) {
  var text = surroundingInfo.text.slice(
      0, Math.min(surroundingInfo.anchor, surroundingInfo.focus));
  this.surroundingText_ = text;
  if (!this.compositionText_) {
    if (this.lastCorrection_ && !goog.string.endsWith(text,
        this.lastCorrection_.target)) {
      // Clears the last correct if the cursor changed.
      this.lastCorrection_ = null;
    }
    if (this.enableDoubleSpacePeriod_ &&
        REGEX_DOUBLE_SPACE_PERIOD_CHARACTERS.test(text)) {
      this.deleteSurroudingText_(-2, 2, (function() {
        this.commitText_('. ', false, 4);
        this.lastCorrection_ = new i18n.input.chrome.xkb.Correction(' ',
            '. ');
      }).bind(this));
      return;
    }

    chrome.runtime.sendMessage(goog.object.create(
        Name.MSG_TYPE, Type.SURROUNDING_TEXT_CHANGED,
        Name.TEXT, text));
  }
};


/**
 * Deletes the surrounding text.
 *
 * @param {number} offset .
 * @param {number} length .
 * @param {!Function} callback .
 * @private
 */
Controller.prototype.deleteSurroudingText_ = function(offset, length,
    callback) {
  var parameters = {};
  parameters['engineID'] = this.engineID_;
  parameters['contextID'] = this.context.contextID;
  parameters['offset'] = offset;
  parameters['length'] = length;
  chrome.input.ime.deleteSurroundingText(parameters, callback);
};


/** @override */
Controller.prototype.activate = function(inputToolCode) {
  this.engineID_ = inputToolCode;
};


/** @override */
Controller.prototype.unregister = function() {
  goog.base(this, 'unregister');

  this.compositionText_ = '';
  this.committedText_ = '';
  this.textBeforeCursor_ = '';
  this.lastCommitType_ = -1;
  this.deadKey_ = '';
  chrome.runtime.sendMessage(goog.object.create(
      Name.MSG_TYPE, Type.CONTEXT_BLUR
      ));
};


/** @override */
Controller.prototype.reset = function() {
  goog.base(this, 'reset');

  this.commitText_('', false, 1);
  this.compositionText_ = '';
  this.committedText_ = '';
};


/** @override */
Controller.prototype.register = function(context) {
  goog.base(this, 'register', context);

  this.sendContextFocusEvent_();
};


/**
 * Sends the CONTEXT_FOCUS event.
 *
 * @private
 */
Controller.prototype.sendContextFocusEvent_ = function() {
  if (!this.context) {
    return;
  }

  var contextType = this.context.type;
  if (this.isPasswdBox_()) {
    contextType = 'password';
  }
  chrome.runtime.sendMessage(goog.object.create(
      Name.MSG_TYPE, Type.CONTEXT_FOCUS,
      Name.CONTEXT_TYPE, contextType
      ));
};


/**
 * Commits the text.
 *
 * @param {string} text .
 * @param {boolean} append .
 * @param {number} triggerType The trigger type:
 *     0: BySpace; 1: ByReset; 2: ByCandidate; 3: BySymbolOrNumber;
 *     4: ByDoubleSpaceToPeriod.
 * @private
 */
Controller.prototype.commitText_ = function(text, append, triggerType) {
  var textToCommit;
  if (!this.context) {
    return;
  }

  this.lastCommitType_ = triggerType;
  if (text && append) {
    textToCommit = this.compositionText_ + text;
  } else if (text && !append) {
    textToCommit = text;
  } else {
    textToCommit = this.compositionText_;
  }
  if (this.context.contextID == 0 || !textToCommit) {
    return;
  }

  var contextID = this.context.contextID;
  chrome.input.ime.commitText(goog.object.create(
      Name.CONTEXT_ID, contextID,
      Name.TEXT, textToCommit
      ));
  if (this.dataSource_ && this.supportPrediction_()) {
    this.dataSource_.sendPredictionRequest(textToCommit);
  }

  var source = this.compositionText_;
  var target = textToCommit.trim();
  this.statistics_.recordCommit(
      source.length, target.length,
      goog.array.indexOf(this.candidates_, target),
      this.statistics_.getTargetType(source, target), triggerType);

  this.compositionText_ = '';
  if (textToCommit != ' ') {
    // If it is a space, don't reset the committed text.
    this.committedText_ = textToCommit;
  }
};


/**
 * Sets the composition text.
 *
 * @param {string} text .
 * @param {boolean} append .
 * @param {boolean} normalizable .
 * @param {!Object=} opt_spatialData .
 * @private
 */
Controller.prototype.setComposition_ = function(text, append, normalizable,
    opt_spatialData) {
  if (!this.context) {
    return;
  }

  var deadKey = this.deadKey_;
  this.deadKey_ = '';
  if (util.DISPLAY_MAPPING[text]) {
    // If this is a dead key.
    if (deadKey == text) {
      // If the previous deadkey is equal to current deadkey, show
      // the visible character for the dead key.
      text = util.DISPLAY_MAPPING[text];
    } else {
      this.deadKey_ = text;
    }
  } else if (normalizable && deadKey) {
    if (!util.supportDeadKey(text)) {
      return;
    }
    // Jscompiler doesn't recognize normalize method, use quote first.
    text = (text + deadKey)['normalize']();
  }

  this.committedText_ = '';
  if (append) {
    text = this.compositionText_ + text;
  }
  this.compositionText_ = text;
  chrome.input.ime.setComposition(goog.object.create(
      Name.CONTEXT_ID, this.context.contextID,
      Name.TEXT, text,
      Name.CURSOR, text.length
      ));

  if (text) {
    this.statistics_.recordCommit(text.length, text.length, -1, 0, 1);
    this.dataSource_ && this.dataSource_.sendCompletionRequest(text,
        opt_spatialData);
  } else {
    chrome.runtime.sendMessage(goog.object.create(
        Name.MSG_TYPE, Type.CANDIDATES_BACK,
        Name.MSG, goog.object.create(
        Name.SOURCE, [],
        Name.CANDIDATES, [],
        Name.MATCHED_LENGTHS, []
        )));
  }
};


/**
 * Callback for set the language.
 *
 * @param {*} msg .
 * @private
 */
Controller.prototype.processSetLanguage_ = function(msg) {
  this.dataSource_ && this.dataSource_.setLanguage(msg[Name.LANGUAGE]);
};


/**
 * Callback when candidates is fetched back.
 *
 * @param {string} source .
 * @param {!Array.<string>} candidates .
 * @param {!Array.<number>} matchedLengths .
 * @private
 */
Controller.prototype.onCandidatesBack_ = function(source, candidates,
    matchedLengths) {
  this.candidates_ = [];
  if (source == this.compositionText_) {
    for (var i = 0; i < candidates.length; i++) {
      this.candidates_.push(goog.object.create(Name.CANDIDATE, candidates[i],
          Name.ID, i));
    }
  }

  if (source) {
    // Removes the candidates whose matched length doesn't equal to the length
    // of source.
    this.candidates_ = goog.array.filter(this.candidates_, function(c, i) {
      // Engine returns wrong length if source contains ' or -.
      // So remove the ' and - in source and then compare the length with
      // matched_length. If matched_lengths[i] is undefined, don't filter
      // out the candidate.
      return matchedLengths[i] == undefined ||
          matchedLengths[i] == source.length;
    });
    // Makes sure the candidates align with the upper case pattern as source.
    this.candidates_ = goog.array.map(this.candidates_, function(candidate,
        index) {
          var c = candidate[Name.CANDIDATE];
          if (c.toLowerCase().indexOf(source.toLowerCase()) == 0) {
            candidate[Name.CANDIDATE] = source + c.slice(source.length);
          } else if (source.toUpperCase() == source) {
            candidate[Name.CANDIDATE] = c.toUpperCase();
          } else {
            var ch = source.charAt(0);
            if (ch.toUpperCase() == ch) {
              candidate[Name.CANDIDATE] = c.charAt(0).toUpperCase() +
                  c.slice(1);
            }
          }
          return candidate;
        });
  }
  chrome.runtime.sendMessage(goog.object.create(
      Name.MSG_TYPE, Type.CANDIDATES_BACK,
      Name.MSG, goog.object.create(
          Name.SOURCE, source,
          Name.CANDIDATES, this.candidates_,
          Name.MATCHED_LENGTHS, matchedLengths
      )));
};


/**
 * True if the current text box is url field.
 *
 * @private
 * @return {boolean}
 */
Controller.prototype.isUrlBox_ = function() {
  return !!this.context && !!this.context.type && this.context.type == 'url';
};


/**
 * Is a password box.
 *
 * @return {boolean} .
 * @private
 */
Controller.prototype.isPasswdBox_ = function() {
  return !!this.context && (!this.context.type ||
      this.context.type == 'password');
};


/**
 * True if the text box supports prediction.
 *
 * @return {boolean} .
 * @private
 */
Controller.prototype.supportPrediction_ = function() {
  return !this.isUrlBox_() && !this.isPasswdBox_();
};


/**
 * If the previous text is not whitespace, then add a space automatically.
 *
 * @private
 */
Controller.prototype.maybeAutoSpace_ = function() {
  if (!this.compositionText_ && this.lastCommitType_ == 2) {
    this.commitText_(' ', false, 0);
  }
};


/** @override */
Controller.prototype.handleNonCharacterKeyEvent = function(keyData) {
  var code = keyData[Name.CODE];
  var type = keyData[Name.MSG_TYPE];
  if (code == KeyCodes.BACKSPACE && type == goog.events.EventType.KEYDOWN) {
    if (this.compositionText_) {
      this.compositionText_ = this.compositionText_.substring(
          0, this.compositionText_.length - 1);
      this.setComposition_(this.compositionText_, false, false);
      return;
    }

    if (this.lastCorrection_) {
      var offset = this.lastCorrection_.target.length;
      this.deleteSurroudingText_(offset, offset, (function() {
        this.commitText_(this.lastCorrection_.source, false, 1);
        this.lastCorrection_ = null;
      }).bind(this));
      return;
    }

    chrome.runtime.sendMessage(goog.object.create(
        Name.MSG_TYPE, Type.CANDIDATES_BACK,
        Name.MSG, goog.object.create(
            Name.SOURCE, [],
            Name.CANDIDATES, [],
            Name.MATCHED_LENGTHS, []
        )));
  }

  if (code != KeyCodes.BACKSPACE) {
    this.commitText_('', false, 1);
  }

  if (goog.array.contains(AbstractController.FUNCTIONAL_KEYS, code)) {
    this.statistics_.recordSpecialKey(code, !!this.compositionText_);
  }

  this.lastCommitType_ = -1;
  goog.base(this, 'handleNonCharacterKeyEvent', keyData);
};


/** @override */
Controller.prototype.handleCharacterKeyEvent = function(keyData) {
  if (keyData[Name.MSG_TYPE] == goog.events.EventType.KEYUP) {
    var ch = keyData[Name.KEY];
    var isSpaceKey = keyData[Name.CODE] == KeyCodes.SPACE;
    if (isSpaceKey) {
      // Commits the first candidate, if not, commits the current composition.
      var firstCandidate = this.candidates_.length > 0 ? this.candidates_[0][
          Name.CANDIDATE] : '';
      if (firstCandidate && this.compositionText_ &&
          firstCandidate != this.compositionText_) {
        this.lastCorrection_ = new i18n.input.chrome.xkb.Correction(
            this.compositionText_ + ' ', firstCandidate + ' ');
        this.commitText_(firstCandidate + ' ', false, 0);
      } else {
        this.commitText_(' ', true, 0);
      }
      return;
    }

    var commit = util.isCommitCharacter(ch);
    if (commit || !this.dataSource_ || !this.dataSource_.isReady() ||
        this.isPasswdBox_() || this.isUrlBox_()) {
      this.commitText_(ch, true, commit ? 3 : 1);
    } else {
      this.maybeAutoSpace_();
      this.setComposition_(ch, true, true, keyData[Name.SPATIAL_DATA] || {});
    }
  }
};


/** @override */
Controller.prototype.processMessage = function(message, sender, sendResponse) {
  var type = message[Name.MSG_TYPE];
  switch (type) {
    case Type.SELECT_CANDIDATE:
      var candidate = message[Name.CANDIDATE];
      this.maybeAutoSpace_();
      this.commitText_(candidate[Name.CANDIDATE], false, 2);
      // TODO: A hack to workaround the IME bug: surrounding text event won't
      // trigger when composition text is equal to committed text. Remove this
      // line when bug is fixed.
      this.surroundingText_ = candidate[Name.CANDIDATE];
      return;
    case Type.SET_LANGUAGE:
      this.processSetLanguage_(message);
      return;
    case Type.CONNECT:
      var contextType = '';
      if (this.context) {
        contextType = this.context.type;
        if (this.isPasswdBox_()) {
          contextType = 'password';
        }
      }
      this.updateSettings({
        'autoSpace': true,
        'autoCapital': true,
        'supportCompact': true,
        'contextType': contextType
      });
      return;
    case Type.SEND_KEY_EVENT:
      var keyData = /** @type {!Array.<!ChromeKeyboardEvent>} */
          (message[Name.KEY_DATA]);
      if (keyData.length >= 2 && keyData[0][Name.CODE] == 'Toggle') {
        // 1C means switch to Japanese IME keyboard.
        keyData[0][Name.CODE] = keyData[1][Name.CODE] = 'Backquote';
        keyData[0][Name.REQUEST_ID] = String(this.requestId++);
        keyData[0][Name.KEYCODE] = 0x1C;
        keyData[1][Name.REQUEST_ID] = String(this.requestId++);
        keyData[1][Name.KEYCODE] = 0x1C;
        chrome.input.ime.sendKeyEvents(goog.object.create(
            Name.CONTEXT_ID, 0,
            Name.KEY_DATA, keyData
            ));
        return;
      }
      if (window.xkb && xkb.handleSendKeyEvent) {
        xkb.handleSendKeyEvent(keyData);
        return;
      }
  }

  goog.base(this, 'processMessage', message, sender, sendResponse);
};

});  // goog.scope
