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
goog.require('goog.string');
goog.require('i18n.input.chrome.AbstractController');
goog.require('i18n.input.chrome.DataSource');
goog.require('i18n.input.chrome.inputview.Statistics');
goog.require('i18n.input.chrome.inputview.events.KeyCodes');
goog.require('i18n.input.chrome.inputview.util');
goog.require('i18n.input.chrome.message.Name');
goog.require('i18n.input.chrome.message.Type');
goog.require('i18n.input.chrome.options.OptionStorageHandlerFactory');
goog.require('i18n.input.chrome.options.OptionType');
goog.require('i18n.input.chrome.xkb.Correction');


goog.scope(function() {
var AbstractController = i18n.input.chrome.AbstractController;
var EventType = i18n.input.chrome.DataSource.EventType;
var Name = i18n.input.chrome.message.Name;
var Type = i18n.input.chrome.message.Type;
var KeyCodes = i18n.input.chrome.inputview.events.KeyCodes;
var OptionType = i18n.input.chrome.options.OptionType;
var OptionStorageHandlerFactory =
    i18n.input.chrome.options.OptionStorageHandlerFactory;
var util = i18n.input.chrome.inputview.util;


/**
 * The regex of characters need to convert two following spaces to period.
 *
 * @type {!RegExp}
 */
var REGEX_DOUBLE_SPACE_PERIOD_CHARACTERS =
    /[a-z0-9\u00E0-\u00F6\u00F8-\u017F'"%\)\]\}\>\+]\s$/i;



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
   * Whether or not to enable double space to period feature.
   *
   * @private {boolean}
   */
  this.enableDoubleSpacePeriod_ = false;

  /** @private {!Array.<!Object>} */
  this.candidates_ = [];

  if (window.inputview && inputview.getKeyboardConfig) {
    inputview.getKeyboardConfig((function(config) {
      this.enableEmojiCandidate_ = !!config['experimental'];
    }).bind(this));
  }
};
var Controller = i18n.input.chrome.xkb.Controller;
goog.inherits(Controller, i18n.input.chrome.AbstractController);


/** @private {boolean} */
Controller.prototype.enableEmojiCandidate_ = false;


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
Controller.prototype.surroundingText_ = '';


/**
 * The type of the last commit.
 *
 * @private {number}
 */
Controller.prototype.lastCommitType_ = -1;


/**
 * Whehter the candidates are predicted.
 *
 * @private {boolean}
 */
Controller.prototype.isPredict_ = false;


/** @private {string} */
Controller.prototype.engineID_;


/** @override */
Controller.prototype.onSurroundingTextChanged = function(
    engineID, surroundingInfo) {
  var text = surroundingInfo.text.slice(
      0, Math.min(surroundingInfo.anchor, surroundingInfo.focus));
  this.surroundingText_ = text;
  if (!this.compositionText_) {
    if (this.lastCorrection_ && !this.lastCorrection_.reverting &&
        this.lastCorrection_.shouldCancel(text)) {
      // Clears the last correct if the cursor changed.
      this.lastCorrection_ = null;
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
  if (!this.context) {
    return;
  }
  var parameters = {};
  parameters['engineID'] = this.engineID_;
  parameters['contextID'] = this.context.contextID;
  parameters['offset'] = offset;
  parameters['length'] = length;
  chrome.input.ime.deleteSurroundingText(parameters, callback);
};


/** @override */
Controller.prototype.activate = function(inputToolCode) {
  Controller.base(this, 'activate', inputToolCode);
  this.engineID_ = inputToolCode;
};


/** @override */
Controller.prototype.updateOptions = function(inputToolCode) {
  var optionStorageHandler =
      OptionStorageHandlerFactory.getInstance().getHandler(inputToolCode);
  var doubleSpacePeriod = /** @type {boolean} */
      (optionStorageHandler.get(OptionType.DOUBLE_SPACE_PERIOD));
  var soundOnKeypress = /** @type {boolean} */
      (optionStorageHandler.get(OptionType.SOUND_ON_KEYPRESS));
  this.enableDoubleSpacePeriod_ = doubleSpacePeriod;
  this.updateSettings({
    'doubleSpacePeriod': doubleSpacePeriod,
    'soundOnKeypress': soundOnKeypress
  });
};


/** @override */
Controller.prototype.unregister = function() {
  goog.base(this, 'unregister');

  this.compositionText_ = '';
  this.committedText_ = '';
  this.textBeforeCursor_ = '';
  this.lastCommitType_ = -1;
  this.deadKey_ = '';
};


/** @override */
Controller.prototype.reset = function() {
  goog.base(this, 'reset');

  this.commitText_('', false, 1);
  this.compositionText_ = '';
  this.committedText_ = '';
  this.dataSource_ && this.dataSource_.clear();
};


/** @override */
Controller.prototype.onCompositionCanceled = function() {
  goog.base(this, 'onCompositionCanceled');

  this.clearCandidates_();
  this.compositionText_ = '';
  this.committedText_ = '';
  this.dataSource_ && this.dataSource_.clear();
};


/**
 * Clears the candidates.
 *
 * @private
 */
Controller.prototype.clearCandidates_ = function() {
  this.candidates_ = [];
  chrome.runtime.sendMessage(goog.object.create(
      Name.MSG_TYPE, Type.CANDIDATES_BACK,
      Name.MSG, goog.object.create(
          Name.SOURCE, [],
          Name.CANDIDATES, [],
          Name.MATCHED_LENGTHS, []
      )));
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
  if (!this.context || this.context.contextID == 0) {
    return;
  }

  this.lastCommitType_ = triggerType;
  text = this.maybeNormalizeDeadKey_(text, append);
  var textToCommit;
  if (text && append) {
    textToCommit = this.compositionText_ + text;
  } else if (text && !append) {
    textToCommit = text;
  } else {
    textToCommit = this.compositionText_;
  }

  if (textToCommit) {
    var contextID = this.context.contextID;
    chrome.input.ime.commitText(goog.object.create(
        Name.CONTEXT_ID, contextID,
        Name.TEXT, textToCommit
        ));
  }
  if (text) {
    if (this.dataSource_ && this.supportPrediction_()) {
      this.dataSource_.sendPredictionRequest(
          this.filterPreviousText_(this.surroundingText_ + textToCommit));
    }
    // statstics for text committing.
    var source = this.compositionText_;
    var target = textToCommit.trim();
    var candidates = goog.array.map(this.candidates_, function(candidate) {
      return candidate[Name.CANDIDATE];
    });
    this.statistics_.recordCommit(
        source.length, target.length,
        goog.array.indexOf(candidates, target),
        this.statistics_.getTargetType(source, target), triggerType);
  } else {
    this.clearCandidates_();
  }

  this.compositionText_ = '';
  if (textToCommit != ' ') {
    // If it is a space, don't reset the committed text.
    this.committedText_ = textToCommit;
  }
};


/**
 * If there is a deadkey in previous, normalize the dead key with this text.
 *
 * @param {string} text .
 * @param {boolean} normalizable .
 * @private
 */
Controller.prototype.maybeNormalizeDeadKey_ = function(text, normalizable) {
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
      text = '';
    }
  } else if (normalizable && deadKey && util.supportDeadKey(text)) {
    // Jscompiler doesn't recognize normalize method, use quote first.
    text = (text + deadKey)['normalize']();
  }
  return text;
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

  text = this.maybeNormalizeDeadKey_(text, normalizable);
  if (!text && append) {
    return;
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
    this.dataSource_ && this.dataSource_.sendCompletionRequest(
        text, this.filterPreviousText_(this.surroundingText_),
        opt_spatialData);
  } else {
    this.clearCandidates_();
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
 * @param {!Array.<!Object>} candidates .
 * @private
 */
Controller.prototype.onCandidatesBack_ = function(source, candidates) {
  this.candidates_ = [];
  if (source == this.compositionText_) {
    this.candidates_ = candidates;
  }

  if (!this.enableEmojiCandidate_) {
    this.candidates_ = goog.array.filter(this.candidates_, function(candidate) {
      return !candidate[Name.IS_EMOJI];
    });
  }

  if (this.candidates_.length == 2) {
    // Simply filters out case when there is only two candidates.
    this.candidates_ = [];
  }

  if (source && this.correctionLevel > 0 && this.candidates_.length > 0) {
    var firstCandidate = this.candidates_[0];
    if (firstCandidate[Name.CANDIDATE] != source) {
      firstCandidate[Name.IS_AUTOCORRECT] = true;
    }
  }

  this.isPredict_ = !source;
  chrome.runtime.sendMessage(goog.object.create(
      Name.MSG_TYPE, Type.CANDIDATES_BACK,
      Name.MSG, goog.object.create(
          Name.SOURCE, source,
          Name.CANDIDATES, this.candidates_
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
 * @return {boolean}
 * @private
 */
Controller.prototype.shouldAutoSpace_ = function() {
  return !this.compositionText_ && this.lastCommitType_ == 2;
};


/**
 * Gets the filtered previous word.
 *
 * @param {string} preText The text before composition or cursor.
 * @return {string} The filtered previous word
 * @private
 */
Controller.prototype.filterPreviousText_ = function(preText) {
  return preText.match(
      /[a-z\-\'\u00c0-\u00d6\u00d8-\u00f6\u00f8-\u017f]* *$/i)[0].trim();
};


/**
 * True to do the auto-correct.
 *
 * @param {string} ch .
 * @param {number} triggerType .
 * @private
 */
Controller.prototype.maybeTriggerAutoCorrect_ = function(ch, triggerType) {
  var firstCandidate = this.candidates_.length > 0 ? this.candidates_[0][
      Name.CANDIDATE] : '';
  if (firstCandidate && this.compositionText_ &&
      firstCandidate != this.compositionText_) {
    this.lastCorrection_ = new i18n.input.chrome.xkb.Correction(
        this.compositionText_ + ch, firstCandidate + ch);
    this.commitText_(firstCandidate + ch, false, triggerType);
  } else {
    this.commitText_(ch, true, triggerType);
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
      var offset = this.lastCorrection_.getTargetLength(this.surroundingText_);
      this.lastCorrection_.reverting = true;
      this.deleteSurroudingText_(offset, offset, (function() {
        this.commitText_(this.lastCorrection_.getSource(this.surroundingText_),
            false, 1);
        this.lastCorrection_ = null;
      }).bind(this));
      return;
    }
    this.clearCandidates_();
  }

  if (code == KeyCodes.ENTER) {
    this.maybeTriggerAutoCorrect_('', 1);
    if (this.lastCorrection_) {
      this.lastCorrection_.isTriggerredByEnter = true;
    }
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
    var commit = util.isCommitCharacter(ch);
    var isSpaceKey = keyData[Name.CODE] == KeyCodes.SPACE;
    if (isSpaceKey || commit) {
      this.maybeTriggerAutoCorrect_(ch, isSpaceKey ? 0 : 3);
      return;
    }

    if (!this.dataSource_ || !this.dataSource_.isReady() ||
        this.isPasswdBox_() || this.isUrlBox_()) {
      this.commitText_(ch, true, 1);
    } else {
      if (this.shouldAutoSpace_()) {
        this.commitText_(' ', false, 0);
      }
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
      var committingText = this.shouldAutoSpace_() ?
          ' ' + candidate[Name.CANDIDATE] : candidate[Name.CANDIDATE];
      this.commitText_(committingText, false, 2);
      // TODO: A hack to workaround the IME bug: surrounding text event won't
      // trigger when composition text is equal to committed text. Remove this
      // line when bug is fixed.
      this.surroundingText_ = candidate[Name.CANDIDATE];
      break;
    case Type.SET_LANGUAGE:
      this.processSetLanguage_(message);
      break;
    case Type.DOUBLE_CLICK_ON_SPACE_KEY:
      if (this.enableDoubleSpacePeriod_ &&
          REGEX_DOUBLE_SPACE_PERIOD_CHARACTERS.test(this.surroundingText_)) {
        if (this.compositionText_) {
          console.error('Composition text is not expected when double click' +
              ' on space key.');
        }
        this.deleteSurroudingText_(-1, 1, (function() {
          this.commitText_('. ', false, 4);
          this.lastCorrection_ = new i18n.input.chrome.xkb.Correction(' ',
              '. ');
        }).bind(this));
      } else {
        this.commitText_(' ', true, 0);
      }
      break;
    case Type.CONNECT:
      this.updateSettings({
        'autoSpace': true,
        'autoCapital': true,
        'supportCompact': true,
        'doubleSpacePeriod': this.enableDoubleSpacePeriod_
      });
      break;
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
      } else if (window.xkb && xkb.handleSendKeyEvent) {
        xkb.handleSendKeyEvent(keyData);
      }
      break;
    case Type.OPTION_CHANGE:
      var optionType = message[Name.OPTION_TYPE];
      var prefix = message[Name.OPTION_PREFIX];
      switch (optionType) {
        case OptionType.DOUBLE_SPACE_PERIOD:
        case OptionType.SOUND_ON_KEYPRESS:
          this.updateOptions(prefix);
          break;
      }
      break;
  }

  goog.base(this, 'processMessage', message, sender, sendResponse);
};

});  // goog.scope
