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

goog.require('goog.Timer');
goog.require('goog.array');
goog.require('goog.events.EventType');
goog.require('goog.object');
goog.require('i18n.input.chrome.AbstractController');
goog.require('i18n.input.chrome.Constant');
goog.require('i18n.input.chrome.EngineIdLanguageMap');
goog.require('i18n.input.chrome.Statistics');
goog.require('i18n.input.chrome.TriggerType');
goog.require('i18n.input.chrome.inputview.FeatureName');
goog.require('i18n.input.chrome.inputview.events.KeyCodes');
goog.require('i18n.input.chrome.inputview.util');
goog.require('i18n.input.chrome.message.Name');
goog.require('i18n.input.chrome.message.Type');
goog.require('i18n.input.chrome.options.OptionStorageHandlerFactory');
goog.require('i18n.input.chrome.options.OptionType');
goog.require('i18n.input.chrome.xkb.Correction');
goog.require('i18n.input.chrome.xkb.LatinInputToolCode');

goog.scope(function() {
var Constant = i18n.input.chrome.Constant;
var FeatureName = i18n.input.chrome.inputview.FeatureName;
var KeyCodes = i18n.input.chrome.inputview.events.KeyCodes;
var LatinInputToolCode = i18n.input.chrome.xkb.LatinInputToolCode;
var Name = i18n.input.chrome.message.Name;
var OptionType = i18n.input.chrome.options.OptionType;
var OptionStorageHandlerFactory =
    i18n.input.chrome.options.OptionStorageHandlerFactory;
var TriggerType = i18n.input.chrome.TriggerType;
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
 * The regex to split the portion of word before cursor if any.
 *
 * @type {!RegExp}
 */
var LETTERS_BEFORE_CURSOR = new RegExp(Constant.LATIN_VALID_CHAR + '+$', 'i');


/**
 * The regex to split the portion of word after cursor if any.
 *
 * @type {!RegExp}
 */
var LETTERS_AFTER_CURSOR =
    new RegExp('^' + Constant.LATIN_VALID_CHAR + '+', 'i');



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
};
var Controller = i18n.input.chrome.xkb.Controller;
goog.inherits(Controller, i18n.input.chrome.AbstractController);


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
 * The cursor position in composition text.
 *
 * @private {number}
 */
Controller.prototype.compositionTextCursorPosition_ = 0;


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
 * The type of the last commit.
 *
 * @private {!TriggerType}
 */
Controller.prototype.lastCommitType_ = TriggerType.UNKNOWN;


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
 * Whether the left/right arrow keydown is handled.
 * This is for handling the left/right arrow keyup correctly.
 *
 * @private {boolean}
 */
Controller.prototype.isArrowKeyDownHandled_ = false;


/**
 * Whether the input view is visible.
 *
 * @private {boolean}
 */
Controller.prototype.isVisible_ = false;


/**
 * Whether to skip next attempt to set the word which has cursor to composition
 * mode.
 *
 * @private {boolean}
 */
Controller.prototype.skipNextSetComposition_ = false;


/**
 * Whether physical keyboard is used for typing.
 *
 * @private {boolean}
 */
Controller.prototype.usingPhysicalKeyboard_ = false;


/**
 * When surrounding text change, not going to trigger complementation at once.
 * Will delay 500ms to do it.
 *
 * @private {number}
 */
Controller.prototype.delayTimer_ = 0;


/** @override */
Controller.prototype.onSurroundingTextChanged = function(
    engineID, surroundingInfo) {
  this.maybeSetExistingWordToCompositonMode_();

  if (!this.compositionText_) {
    if (this.lastCorrection_ && !this.lastCorrection_.reverting &&
        this.lastCorrection_.shouldCancel(this.env.textBeforeCursor)) {
      // Clears the last correct if the cursor changed.
      this.lastCorrection_ = null;
    }

    chrome.runtime.sendMessage(goog.object.create(
        Name.TYPE, Type.SURROUNDING_TEXT_CHANGED,
        Name.TEXT, this.env.textBeforeCursor,
        Name.ANCHOR, surroundingInfo.anchor,
        Name.FOCUS, surroundingInfo.focus));
  }
};


/**
 * Sets the word which has cursor to composition mode if needed.
 *
 * @private
 */
Controller.prototype.maybeSetExistingWordToCompositonMode_ = function() {
  goog.Timer.clear(this.delayTimer_);
  if (this.shouldSetComposition_()) {
    var cursorPosition = this.env.surroundingInfo.focus;
    var beforeCursorMatched =
        LETTERS_BEFORE_CURSOR.exec(this.env.textBeforeCursor);
    var startPosition = cursorPosition;
    if (beforeCursorMatched) {
      var word = beforeCursorMatched[0];
      startPosition = this.env.textBeforeCursor.length - word.length;
    }

    var textAfterCursor = this.env.surroundingInfo.text.slice(cursorPosition);
    var afterCursorMatched = LETTERS_AFTER_CURSOR.exec(textAfterCursor);
    var endPosition = cursorPosition;
    if (afterCursorMatched) {
      var word = afterCursorMatched[0];
      endPosition += word.length;
    }

    if (startPosition != endPosition) {
      var word = this.env.surroundingInfo.text.slice(
          startPosition, endPosition);
      var cb = this.setToCompositionMode_.bind(this, startPosition,
          endPosition, cursorPosition, word);
      this.delayTimer_ = goog.Timer.callOnce(cb, 500);
    }
  }
  this.skipNextSetComposition_ = false;
};


/**
 * Sets the word to composition mode.
 *
 * @param {number} start .
 * @param {number} end .
 * @param {number} cursor .
 * @param {string} word .
 *
 * @private
 */
Controller.prototype.setToCompositionMode_ =
    function(start, end, cursor, word) {
  this.compositionText_ = word;
  this.compositionTextCursorPosition_ = cursor - start;
  this.env.textBeforeCursor = this.env.textBeforeCursor.slice(0, start);
  this.committedText_ = '';
  chrome.runtime.sendMessage(goog.object.create(
      Name.TYPE, Type.SURROUNDING_TEXT_CHANGED,
      Name.TEXT, this.committedText_));

  this.deleteSurroundingText_(start - cursor, end - start, (function() {
    chrome.input.ime.setComposition(goog.object.create(
        Name.CONTEXT_ID, this.env.context.contextID,
        Name.TEXT, word,
        Name.CURSOR, this.compositionTextCursorPosition_));
    this.dataSource_.sendCompletionRequestForWord(word,
        this.filterPreviousText_(this.env.textBeforeCursor));
  }).bind(this));
};


/**
 * Deletes the surrounding text.
 *
 * @param {number} offset .
 * @param {number} length .
 * @param {!Function} callback .
 * @private
 */
Controller.prototype.deleteSurroundingText_ = function(offset, length,
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
  this.statistics_.setPhysicalKeyboard(false);
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
  this.correctionLevel = correctionLevel;
  if (this.dataSource_) {
    this.dataSource_.setEnableUserDict(true);
    this.dataSource_.setCorrectionLevel(this.correctionLevel);
    this.statistics_.setAutoCorrectLevel(this.correctionLevel);
  }
  this.doubleSpacePeriod_ = doubleSpacePeriod;
  this.updateSettings({
    'autoSpace': true,
    'autoCapital': autoCapital,
    'doubleSpacePeriod': doubleSpacePeriod,
    'soundOnKeypress': soundOnKeypress
  });
};


/** @override */
Controller.prototype.endSession = function() {
  goog.base(this, 'endSession');
  this.statistics_.recordSessionEnd();
};


/** @override */
Controller.prototype.unregister = function() {
  goog.base(this, 'unregister');
  this.compositionText_ = '';
  this.compositionTextCursorPosition_ = 0;
  this.committedText_ = '';
  this.lastCommitType_ = TriggerType.UNKNOWN;
  this.deadKey_ = '';
  this.endSession();
};


/** @override */
Controller.prototype.reset = function() {
  goog.base(this, 'reset');

  this.commitText_('', false, TriggerType.RESET, false);
  this.compositionText_ = '';
  this.compositionTextCursorPosition_ = 0;
  this.committedText_ = '';
  this.dataSource_ && this.dataSource_.clear();
};


/** @override */
Controller.prototype.onCompositionCanceled = function() {
  goog.base(this, 'onCompositionCanceled');

  this.clearCandidates_();
  this.compositionText_ = '';
  this.compositionTextCursorPosition_ = 0;
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
 * @param {!TriggerType} triggerType What triggered the commit.
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
      if (triggerType == TriggerType.CANDIDATE) {
        // Only increase frequency if the user selected this candidate manually.
        this.dataSource_.changeWordFrequency(target, 1);
      }
      if (this.isSuggestionSupported()) {
        this.dataSource_.sendPredictionRequest(
            this.filterPreviousText_(this.env.textBeforeCursor + textToCommit));
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
  this.compositionTextCursorPosition_ = 0;
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
    var cursorPosition = this.compositionTextCursorPosition_;
    this.compositionText_ = this.compositionText_.slice(0, cursorPosition) +
        text + this.compositionText_.slice(cursorPosition);
    this.compositionTextCursorPosition_ = cursorPosition + text.length;
  } else {
    this.compositionText_ = text;
    this.compositionTextCursorPosition_ = text.length;
  }
  chrome.input.ime.setComposition(goog.object.create(
      Name.CONTEXT_ID, this.env.context.contextID,
      Name.TEXT, this.compositionText_,
      Name.CURSOR, this.compositionTextCursorPosition_
      ));

  if (this.compositionText_) {
    if (this.compositionTextCursorPosition_ == this.compositionText_.length) {
      this.dataSource_ && this.dataSource_.sendCompletionRequest(
          this.compositionText_,
          this.filterPreviousText_(this.env.textBeforeCursor),
          opt_spatialData);
    } else {
      // sendCompletionRequest assumes spatia data is for the last character in
      // compositionText_. In cases that we want to add/delete characters beside
      // the last one in compositionText_, we use sendCompletionRequestForWord.
      // Spatial data is not use here.
      this.dataSource_ && this.dataSource_.sendCompletionRequestForWord(
          this.compositionText_,
          this.filterPreviousText_(this.env.textBeforeCursor));
    }
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

  if (!this.env.featureTracker.isEnabled(FeatureName.EXPERIMENTAL)) {
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
  return !this.compositionText_ &&
      this.lastCommitType_ == TriggerType.CANDIDATE;
};


/**
 * True if the word which has cursor should be set to composition mode.
 *
 * @return {boolean}
 * @private
 */
Controller.prototype.shouldSetComposition_ = function() {
  // Sets exisiting word to composition mode if:
  // 1. suggestion is available and enabled;
  // 2. no composition in progress;
  // 3. no text selection;
  // 4. surround text is not empty;
  // 5. virtual keyboard is visible;
  // 6. skipNextSetComposition_ is false;
  // 7. not typing by physical keyboard.
  return this.env.featureTracker.isEnabled(FeatureName.EXPERIMENTAL) &&
         !!this.dataSource_ &&
         this.isSuggestionSupported() &&
         !this.compositionText_ &&
         this.env.surroundingInfo.focus == this.env.surroundingInfo.anchor &&
         this.env.surroundingInfo.text.length > 0 &&
         this.isVisible_ &&
         !this.skipNextSetComposition_ &&
         !this.usingPhysicalKeyboard_;
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
 * @param {!TriggerType} triggerType .
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


/**
 * Moves cursor position in composition text. Note chrome.input APIs do not
 * support move cursor in composition text directly, but support set cursor
 * position when set composition text. To move cursor, we need to set
 * composition on the same text but with a different cursor position to move it
 * to the correct place. If the cursor moves out of the composition word, this
 * function will not consume the arrow key event and return false.
 *
 * @param {boolean} isRight True if move the cursor to right.
 * @return {boolean} True if move cursor successfully.
 * @private
 */
Controller.prototype.moveCursorInComposition_ = function(isRight) {
  if (this.compositionText_) {
    if (isRight) {
      if (this.compositionTextCursorPosition_ == this.compositionText_.length) {
        return false;
      }
      this.compositionTextCursorPosition_++;
    } else {
      if (this.compositionTextCursorPosition_ == 0) {
        return false;
      }
      this.compositionTextCursorPosition_--;
    }
    chrome.input.ime.setComposition(goog.object.create(
        Name.CONTEXT_ID, this.env.context.contextID,
        Name.TEXT, this.compositionText_,
        Name.CURSOR, this.compositionTextCursorPosition_));
    this.dataSource_.sendCompletionRequestForWord(this.compositionText_,
        this.filterPreviousText_(this.env.textBeforeCursor));
    return true;
  }
  return false;
};


/** @override */
Controller.prototype.handleEvent = function(e) {
  goog.base(this, 'handleEvent', e);
  this.usingPhysicalKeyboard_ = true;
  // For physical key events, just confirm the composition and release the key.
  if (this.compositionText_) {
    this.commitText_('', true, TriggerType.RESET, false);
  }
  return false;
};


/** @override */
Controller.prototype.handleNonCharacterKeyEvent = function(keyData) {
  var code = keyData[Name.CODE];
  var type = keyData[Name.TYPE];
  if (code == KeyCodes.BACKSPACE) {
    if (type == goog.events.EventType.KEYDOWN) {
      this.statistics_.recordBackspace();
      if (this.compositionText_) {
        if (this.compositionTextCursorPosition_ == 0) {
          // removes previous character and combines two word if needed.
          var compositionText = this.compositionText_;
          var textBeforeCursor = this.env.textBeforeCursor;
          if (textBeforeCursor) {
            // Text before cursor after remove one character
            textBeforeCursor = textBeforeCursor.slice(0, -1);
            // clear previous composition.
            this.setComposition_('', false, false);

            var word = compositionText;
            this.env.textBeforeCursor = textBeforeCursor;
            var offset = 1;
            this.compositionTextCursorPosition_ = 0;
            var beforeCursorMatched =
                LETTERS_BEFORE_CURSOR.exec(textBeforeCursor);
            if (beforeCursorMatched) {
              word = beforeCursorMatched[0] + word;
              this.env.textBeforeCursor = textBeforeCursor.slice(0,
                  -beforeCursorMatched[0].length);
              offset += beforeCursorMatched[0].length;
              this.compositionTextCursorPosition_ =
                  beforeCursorMatched[0].length;
            }
            this.compositionText_ = word;
            this.deleteSurroundingText_(-offset, offset, (function() {
              chrome.input.ime.setComposition(goog.object.create(
                  Name.CONTEXT_ID, this.env.context.contextID,
                  Name.TEXT, word,
                  Name.CURSOR, this.compositionTextCursorPosition_));
              this.dataSource_.sendCompletionRequestForWord(word,
                  this.filterPreviousText_(this.env.textBeforeCursor));
            }).bind(this));
          }
        } else {
          this.compositionText_ =
              this.compositionText_.slice(0,
                  this.compositionTextCursorPosition_ - 1) +
              this.compositionText_.slice(this.compositionTextCursorPosition_);
          this.compositionTextCursorPosition_--;
          chrome.input.ime.setComposition(goog.object.create(
              Name.CONTEXT_ID, this.env.context.contextID,
              Name.TEXT, this.compositionText_,
              Name.CURSOR, this.compositionTextCursorPosition_));
          this.dataSource_.sendCompletionRequestForWord(this.compositionText_,
              this.filterPreviousText_(this.env.textBeforeCursor));
        }
        this.isBackspaceDownHandled_ = true;
        return;
      }

      if (this.lastCorrection_) {
        var offset = this.lastCorrection_.getTargetLength(
            this.env.textBeforeCursor);
        this.lastCorrection_.reverting = true;
        this.deleteSurroundingText_(-offset, offset, (function() {
          this.commitText_(
              this.lastCorrection_.getSource(this.env.textBeforeCursor),
              false, TriggerType.REVERT, false);
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
    this.maybeTriggerAutoCorrect_('', TriggerType.RESET);
    if (this.lastCorrection_) {
      this.lastCorrection_.isTriggerredByEnter = true;
    }
  }

  // Hide in experimental flag. Will enable it once get official review.
  if (this.isExperimental_ &&
      (code == KeyCodes.ARROW_LEFT || code == KeyCodes.ARROW_RIGHT)) {
    if (type == goog.events.EventType.KEYDOWN && this.compositionText_) {
      this.isArrowKeyDownHandled_ =
          this.moveCursorInComposition_(code == KeyCodes.ARROW_RIGHT);
      if (this.isArrowKeyDownHandled_) {
        return;
      }
      // If arrow event is not consumed by moveCursorInComposition_, it means
      // the cursor is about to move across the composition word boundary. We
      // need to commit the composition text and then move cursor accordingly.
      // If the cursor is at the beginning of a word (position 0) and user
      // pressed left arrow key, it triggers ChromeOS IMF to commit the
      // composition text and move the cursor to the end of the committed word.
      // To move the cursor to the expected place,  we send a ctrl+left arrow
      // key event to move the cursor back to the beginning of a word before
      // send the plain left arrow key event.
      if (this.compositionTextCursorPosition_ == 0 &&
          code == KeyCodes.ARROW_LEFT) {
        var keyDataClone = goog.object.clone(keyData);
        keyDataClone.ctrlKey = true;
        this.sendKeyEvent(/** @type {!ChromeKeyboardEvent} **/(keyDataClone));
      }
    } else if (type == goog.events.EventType.KEYUP &&
        this.isArrowKeyDownHandled_) {
      return;
    }
  }

  this.isArrowKeyDownHandled_ = false;
  this.lastCommitType_ = TriggerType.UNKNOWN;
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
    this.statistics_.recordCharacterKey();
    // Simulates the key event if no input field is focused.
    // Please refer to crbug.com/423880.
    if (!this.env.context) {
      this.sendKeyEvent(keyData);
      return;
    }
    var ch = keyData[Name.KEY];
    var commit = util.isCommitCharacter(ch);
    var isSpaceKey = keyData[Name.CODE] == KeyCodes.SPACE;
    if (isSpaceKey) {
      this.maybeTriggerAutoCorrect_(ch, TriggerType.SPACE);
      return;
    } else if (commit) {
      this.maybeTriggerAutoCorrect_(ch, TriggerType.SYMBOL_OR_NUMBER);
      return;
    }

    if (!this.dataSource_ || !this.dataSource_.isReady() ||
        !this.isSuggestionSupported()) {
      this.commitText_(ch, true, TriggerType.RESET, true);
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
      this.commitText_(committingText, false, TriggerType.CANDIDATE, false);
      // TODO: A hack to workaround the IME bug: surrounding text event won't
      // trigger when composition text is equal to committed text. Remove this
      // line when bug is fixed.
      this.env.textBeforeCursor = candidate[Name.CANDIDATE];
      break;
    case Type.DOUBLE_CLICK_ON_SPACE_KEY:
      if (this.doubleSpacePeriod_ && REGEX_DOUBLE_SPACE_PERIOD_CHARACTERS.test(
          this.env.textBeforeCursor)) {
        if (this.compositionText_) {
          console.error('Composition text is not expected when double click' +
              ' on space key.');
        }
        this.deleteSurroundingText_(-1, 1, (function() {
          this.commitText_('. ', false, TriggerType.DOUBLE_SPACE_TO_PERIOD,
              false);
          this.lastCorrection_ = new i18n.input.chrome.xkb.Correction(' ',
              '. ');
        }).bind(this));
      } else {
        this.commitText_(' ', true, TriggerType.SPACE, true);
      }
      break;
    case Type.CONNECT:
      this.updateOptions(this.env.engineId);
      break;
    case Type.SEND_KEY_EVENT:
      this.usingPhysicalKeyboard_ = false;
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
      this.usingPhysicalKeyboard_ = !this.isVisible_;
      this.maybeSetExistingWordToCompositonMode_();
      if (!this.isVisible_) {
        this.statistics_.recordSessionEnd();
      }
      break;
    case Type.OPTION_CHANGE:
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


/** @override */
Controller.prototype.commitText = function(text, onStage) {
  // Commit text will result onSurroundingTextChanged event. It is not necessary
  // to try to set a just committed word to composition mode in this case, so
  // set skipNextSetComposition_ to true.
  this.skipNextSetComposition_ = true;
  goog.base(this, 'commitText', text, onStage);
};

});  // goog.scope


