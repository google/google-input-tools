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
goog.require('goog.events.EventType');
goog.require('goog.object');
goog.require('i18n.input.chrome.AbstractController');
goog.require('i18n.input.chrome.Constant');
goog.require('i18n.input.chrome.DataSource');
goog.require('i18n.input.chrome.EngineIdLanguageMap');
goog.require('i18n.input.chrome.Statistics');
goog.require('i18n.input.chrome.inputview.events.KeyCodes');
goog.require('i18n.input.chrome.inputview.util');
goog.require('i18n.input.chrome.message.Name');
goog.require('i18n.input.chrome.message.Type');
goog.require('i18n.input.chrome.options.OptionStorageHandlerFactory');
goog.require('i18n.input.chrome.options.OptionType');
goog.require('i18n.input.chrome.xkb.Correction');
goog.require('i18n.input.chrome.xkb.LatinInputToolCode');


goog.scope(function() {
var AbstractController = i18n.input.chrome.AbstractController;
var Constant = i18n.input.chrome.Constant;
var EventType = i18n.input.chrome.DataSource.EventType;
var KeyCodes = i18n.input.chrome.inputview.events.KeyCodes;
var LatinInputToolCode = i18n.input.chrome.xkb.LatinInputToolCode;
var Name = i18n.input.chrome.message.Name;
var OptionType = i18n.input.chrome.options.OptionType;
var OptionStorageHandlerFactory =
    i18n.input.chrome.options.OptionStorageHandlerFactory;
var Type = i18n.input.chrome.message.Type;
var util = i18n.input.chrome.inputview.util;


/**
 * The regex of characters need to convert two following spaces to period.
 *
 * @type {!RegExp}
 */
var REGEX_DOUBLE_SPACE_PERIOD_CHARACTERS =
    /[a-z0-9\u00E0-\u00F6\u00F8-\u017F'"%\)\]\}\>\+]\s$/i;


/**
 * The regex to split the last word of a sentence.
 *
 * @type {!RegExp}
 */
var REGEX_LETTER = new RegExp(Constant.LATIN_VALID_CHAR, 'i');



/**
 * The background page.
 *
 * @extends {i18n.input.chrome.AbstractController}
 * @constructor
 */
i18n.input.chrome.xkb.Controller = function() {
  i18n.input.chrome.xkb.Controller.base(this, 'constructor');

  if (window.xkb && xkb.DataSource) {
    this.dataSource_ = new xkb.DataSource(5,
        this.onCandidatesBack_.bind(this));
    this.dataSource_.setCorrectionLevel(this.correctionLevel);
  }

  /**
   * The statistics object for recording metrics values.
   *
   * @type {!i18n.input.chrome.Statistics}
   * @private
   */
  this.statistics_ = i18n.input.chrome.Statistics.getInstance();

  /** @private {!Array.<!Object>} */
  this.candidates_ = [];

  if (window.inputview && inputview.getKeyboardConfig) {
    inputview.getKeyboardConfig((function(config) {
      this.isExperimental_ = !!config['experimental'];
    }).bind(this));
  }
};
var Controller = i18n.input.chrome.xkb.Controller;
goog.inherits(Controller, i18n.input.chrome.AbstractController);


/** @private {boolean} */
Controller.prototype.isExperimental_ = false;


/**
 * Whether or not to enable double space to period feature.
 *
 * @private {boolean}
 */
Controller.prototype.doubleSpacePeriod_ = false;


/**
 * Whether to auto-capitalize or not.
 *
 * @private {boolean}
 */
Controller.prototype.autoCapital_ = false;


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
 * Whether the candidates are predicted.
 *
 * @private {boolean}
 */
Controller.prototype.isPredict_ = false;


/**
 * Whether the backspace keydown is handled.
 * This is for handling the backspace keyup correctly.
 *
 * @private {boolean}
 */
Controller.prototype.isBackspaceDownHandled_ = false;


/**
 * Whether the input view is visible.
 *
 * @private {boolean}
 */
Controller.prototype.isVisible_ = false;


/** @override */
Controller.prototype.onSurroundingTextChanged = function(
    engineID, surroundingInfo) {
  var text = surroundingInfo.text.slice(
      0, Math.min(surroundingInfo.anchor, surroundingInfo.focus));
  var clearCallback = true;

  if (this.shouldSetComposition_(surroundingInfo)) {
    var cursorPosition = surroundingInfo.focus;
    if (cursorPosition == surroundingInfo.text.length ||
        !REGEX_LETTER.test(surroundingInfo.text[cursorPosition])) {
      var startPosition = cursorPosition == 0 ? 0 : cursorPosition - 1;
      while (startPosition >= 0 &&
             surroundingInfo.text[startPosition].match(REGEX_LETTER)) {
        startPosition--;
      }
      if (startPosition != 0) {
        // Point to the first character of a word.
        startPosition++;
      }
      if (cursorPosition != startPosition) {
        if (this.isVisible_) {
          this.setToCompositionMode_(startPosition, cursorPosition,
              surroundingInfo);
          return;
        } else {
          // Keyboard may not yet initialized or visible when cursor moved. If a
          // cursor movement triggers the keyboard becomes visible, suggestions
          // should be provided through this callback.
          this.setToCompositionModeCallback_ = this.setToCompositionMode_.bind(
              this, startPosition, cursorPosition, surroundingInfo);
          clearCallback = false;
        }
      }
    }
  }

  if (clearCallback) {
    this.setToCompositionModeCallback_ = null;
  }
  this.surroundingText_ = text;
  if (!this.compositionText_) {
    if (this.lastCorrection_ && !this.lastCorrection_.reverting &&
        this.lastCorrection_.shouldCancel(text)) {
      // Clears the last correct if the cursor changed.
      this.lastCorrection_ = null;
    }

    chrome.runtime.sendMessage(goog.object.create(
        Name.TYPE, Type.SURROUNDING_TEXT_CHANGED,
        Name.TEXT, text));
  }
};


/**
 * Sets the word to composition mode.
 *
 * @param {number} start .
 * @param {number} end .
 * @param {!Object} surroundingInfo
 *
 * @private
 */
Controller.prototype.setToCompositionMode_ =
    function(start, end, surroundingInfo) {
  var word = surroundingInfo.text.slice(start, end);
  this.compositionText_ = word;
  this.surroundingText_ = surroundingInfo.text.slice(0, start);
  this.committedText_ = '';
  chrome.runtime.sendMessage(goog.object.create(
      Name.TYPE, Type.SURROUNDING_TEXT_CHANGED,
      Name.TEXT, this.committedText_));
  this.deleteSurroudingText_(start - end, end - start, (function() {
    chrome.input.ime.setComposition(goog.object.create(
        Name.CONTEXT_ID, this.env.context.contextID,
        Name.TEXT, word,
        Name.CURSOR, word.length));
    this.dataSource_.sendCompletionRequestForWord(
        word, this.filterPreviousText_(this.surroundingText_));
  }).bind(this));
  this.setToCompositionModeCallback_ = null;
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
  if (!this.env.context) {
    return;
  }
  var parameters = {};
  parameters['engineID'] = this.env.engineId;
  parameters['contextID'] = this.env.context.contextID;
  parameters['offset'] = offset;
  parameters['length'] = length;
  chrome.input.ime.deleteSurroundingText(parameters, callback);
};


/** @override */
Controller.prototype.activate = function(inputToolCode) {
  Controller.base(this, 'activate', inputToolCode);
  if (this.dataSource_) {
    this.dataSource_.setLanguage(
        i18n.input.chrome.EngineIdLanguageMap[this.env.engineId][0]);
  }
  this.statistics_.setInputMethodId(inputToolCode);
};


/** @override */
Controller.prototype.updateOptions = function(inputToolCode) {
  var optionStorageHandler =
      OptionStorageHandlerFactory.getInstance().getHandler(inputToolCode);
  if (goog.object.containsKey(LatinInputToolCode, inputToolCode)) {
    // Override default of false for latin, all users with no pre-existing
    // stored values should have default set to true instead. Non-latin XKB
    // input components require the option default as false.
    // This is required both in xkb/controller.js and hmm/init_options.js as
    // the user can access them in either order and the experience in both
    // should be the same.
    if (!optionStorageHandler.has(OptionType.DOUBLE_SPACE_PERIOD)) {
      optionStorageHandler.set(OptionType.DOUBLE_SPACE_PERIOD, true);
    }
    if (!optionStorageHandler.has(OptionType.ENABLE_CAPITALIZATION)) {
      optionStorageHandler.set(OptionType.ENABLE_CAPITALIZATION, true);
    }
  }
  var doubleSpacePeriod = /** @type {boolean} */
      (optionStorageHandler.get(OptionType.DOUBLE_SPACE_PERIOD));
  var soundOnKeypress = /** @type {boolean} */
      (optionStorageHandler.get(OptionType.SOUND_ON_KEYPRESS));
  var correctionLevel = /** @type {number} */ (optionStorageHandler.get(
      OptionType.AUTO_CORRECTION_LEVEL));
  var autoCapital = /** @type {boolean} */ (optionStorageHandler.get(
      OptionType.ENABLE_CAPITALIZATION));
  var enableUserDict = /** @type {boolean} */ (optionStorageHandler.get(
      OptionType.ENABLE_USER_DICT));
  this.correctionLevel = correctionLevel;
  if (this.dataSource_) {
    this.dataSource_.setEnableUserDict(enableUserDict);
    this.dataSource_.setCorrectionLevel(this.correctionLevel);
    this.statistics_.setAutoCorrectLevel(this.correctionLevel);
  }
  this.doubleSpacePeriod_ = doubleSpacePeriod;
  this.autoCapital_ = autoCapital;
  this.updateSettings({
    'autoSpace': true,
    'autoCapital': autoCapital,
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

  this.commitText_('', false, 1, false);
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
      Name.TYPE, Type.CANDIDATES_BACK,
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
 *     4: ByDoubleSpaceToPeriod; 5: ByRevert.
 * @param {boolean} simulateKeypress Whether allowing keypress simulation.
 * @private
 */
Controller.prototype.commitText_ = function(
    text, append, triggerType, simulateKeypress) {
  if (!this.env.context || this.env.context.contextID == 0) {
    return;
  }

  if (!simulateKeypress) {
    this.keyData = null;
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
    this.commitText(textToCommit, !!this.compositionText_);
  }
  if (text) {
    var target = textToCommit.trim();
    if (this.dataSource_) {
      // This must come before the prediction request since the prediction
      // request's callback causes the NaCl module to no longer be ready.
      this.dataSource_.commitText(target);
      if (this.isSuggestionSupported()) {
        this.dataSource_.sendPredictionRequest(
            this.filterPreviousText_(this.surroundingText_ + textToCommit));
      }
    }
    // statstics for text committing.
    var source = this.compositionText_;
    var candidates = goog.array.map(this.candidates_, function(candidate) {
      return candidate[Name.CANDIDATE];
    });
    this.statistics_.recordCommit(
        source, target, goog.array.indexOf(candidates, target), triggerType);
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
 * @return {string} The normalized text.
 * @private
 */
Controller.prototype.maybeNormalizeDeadKey_ = function(text, normalizable) {
  if (!text && normalizable) {
    return '';
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
  if (!this.env.context) {
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
      Name.CONTEXT_ID, this.env.context.contextID,
      Name.TEXT, text,
      Name.CURSOR, text.length
      ));

  if (text) {
    this.dataSource_ && this.dataSource_.sendCompletionRequest(
        text, this.filterPreviousText_(this.surroundingText_),
        opt_spatialData);
  } else {
    this.clearCandidates_();
  }
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

  if (!this.isExperimental_) {
    this.candidates_ = goog.array.filter(this.candidates_, function(candidate) {
      return !candidate[Name.IS_EMOJI];
    });
  }

  if (this.candidates_.length == 2) {
    // Simply filters out case when there are only two candidates.
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
      Name.TYPE, Type.CANDIDATES_BACK,
      Name.MSG, goog.object.create(
          Name.SOURCE, source,
          Name.CANDIDATES, this.candidates_
      )));
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
 * True if the word which has cursor should be set to composition mode.
 *
 * @param {!Object} surroundingInfo .
 * @return {boolean}
 * @private
 */
Controller.prototype.shouldSetComposition_ = function(surroundingInfo) {
  // Sets exisiting word to composition mode if:
  // 1. suggestion is avaiable and enabled;
  // 2. no composition in progress;
  // 3. no text selection;
  // 4. surround text is not empty;
  // 5. cursor is at the end of the existing word.
  // TODO(bshe): Due to crbug.com/446249, condition 5 is temporarily introduced.
  // Ideally, the existing word should be in composition mode when cursor moved
  // in it.
  return !!this.isExperimental_ &&
         !!this.dataSource_ &&
         this.dataSource_.isReady() &&
         this.isSuggestionSupported() &&
         !this.compositionText_ &&
         surroundingInfo.focus == surroundingInfo.anchor &&
         surroundingInfo.text.length > 0;
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
      new RegExp(Constant.LATIN_VALID_CHAR + '*\s*$', 'i'))[0].trim();
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
    this.commitText_(firstCandidate + ch, false, triggerType, false);
  } else {
    this.commitText_(ch, true, triggerType, true);
  }
};


/** @override */
Controller.prototype.handleNonCharacterKeyEvent = function(keyData) {
  var code = keyData[Name.CODE];
  var type = keyData[Name.TYPE];
  if (code == KeyCodes.BACKSPACE) {
    if (type == goog.events.EventType.KEYDOWN) {
      if (this.compositionText_) {
        this.compositionText_ = this.compositionText_.substring(
            0, this.compositionText_.length - 1);
        this.setComposition_(this.compositionText_, false, false);
        this.isBackspaceDownHandled_ = true;
        return;
      }

      if (this.lastCorrection_) {
        var offset = this.lastCorrection_.getTargetLength(
            this.surroundingText_);
        this.lastCorrection_.reverting = true;
        this.deleteSurroudingText_(offset, offset, (function() {
          this.commitText_(
              this.lastCorrection_.getSource(this.surroundingText_),
              false, 5, false);
          this.lastCorrection_ = null;
        }).bind(this));
        this.isBackspaceDownHandled_ = true;
        return;
      }
      this.isBackspaceDownHandled_ = false;
      this.clearCandidates_();
    } else if (type == goog.events.EventType.KEYUP &&
        this.isBackspaceDownHandled_) {
      return;
    }
  }

  if (code == KeyCodes.ENTER) {
    this.maybeTriggerAutoCorrect_('', 1);
    if (this.lastCorrection_) {
      this.lastCorrection_.isTriggerredByEnter = true;
    }
  }

  this.lastCommitType_ = -1;
  goog.base(this, 'handleNonCharacterKeyEvent', keyData);
};


/**
 * Overrides handleCharacterKeyEvent instead of handleEvent because handleEvent
 * is called for physical key events which xkb controller should NOT handle.
 *
 * @override
 */
Controller.prototype.handleCharacterKeyEvent = function(keyData) {
  if (keyData[Name.TYPE] == goog.events.EventType.KEYDOWN) {
    var ch = keyData[Name.KEY];
    var commit = util.isCommitCharacter(ch);
    var isSpaceKey = keyData[Name.CODE] == KeyCodes.SPACE;
    if (isSpaceKey || commit) {
      this.maybeTriggerAutoCorrect_(ch, isSpaceKey ? 0 : 3);
      return;
    }

    if (!this.dataSource_ || !this.dataSource_.isReady() ||
        !this.isSuggestionSupported()) {
      this.commitText_(ch, true, 1, true);
    } else {
      if (this.shouldAutoSpace_()) {
        // Calls this.commitText instead of this.commitText_ to avoid side
        // effects like prediction.
        // Also set |this.keyData| as null to suppress the keypress simulation.
        this.keyData = null;
        this.commitText(' ', false);
      }
      this.setComposition_(ch, true, true, keyData[Name.SPATIAL_DATA] || {});
    }
  }
};


/** @override */
Controller.prototype.processMessage = function(message, sender, sendResponse) {
  var type = message[Name.TYPE];
  switch (type) {
    case Type.SELECT_CANDIDATE:
      var candidate = message[Name.CANDIDATE];
      var committingText = this.shouldAutoSpace_() ?
          ' ' + candidate[Name.CANDIDATE] : candidate[Name.CANDIDATE];
      this.commitText_(committingText, false, 2, false);
      // TODO: A hack to workaround the IME bug: surrounding text event won't
      // trigger when composition text is equal to committed text. Remove this
      // line when bug is fixed.
      this.surroundingText_ = candidate[Name.CANDIDATE];
      break;
    case Type.DOUBLE_CLICK_ON_SPACE_KEY:
      if (this.doubleSpacePeriod_ &&
          REGEX_DOUBLE_SPACE_PERIOD_CHARACTERS.test(this.surroundingText_)) {
        if (this.compositionText_) {
          console.error('Composition text is not expected when double click' +
              ' on space key.');
        }
        this.deleteSurroudingText_(-1, 1, (function() {
          this.commitText_('. ', false, 4, false);
          this.lastCorrection_ = new i18n.input.chrome.xkb.Correction(' ',
              '. ');
        }).bind(this));
      } else {
        this.commitText_(' ', true, 0, true);
      }
      break;
    case Type.CONNECT:
      this.updateOptions(this.env.engineId);
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
    case Type.VISIBILITY_CHANGE:
      this.isVisible_ = message[Name.VISIBILITY];
      if (this.isVisible_ && this.setToCompositionModeCallback_) {
        // Set word to composition mode only when input view become visible. The
        // suggestions are displayed in input view.
        this.setToCompositionModeCallback_();
      }
      break;
    case Type.OPTION_CHANGE:
      var optionType = message[Name.OPTION_TYPE];
      var prefix = message[Name.OPTION_PREFIX];
      // We only need to immediately update controller when the currently active
      // IME's options are changed.
      if (this.env.engineId == prefix) {
        this.updateOptions(prefix);
      }
      break;
  }

  goog.base(this, 'processMessage', message, sender, sendResponse);
};

});  // goog.scope


