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
goog.require('i18n.input.chrome.Env');
goog.require('i18n.input.chrome.FeatureName');
goog.require('i18n.input.chrome.Statistics');
goog.require('i18n.input.chrome.TriggerType');
goog.require('i18n.input.chrome.events.KeyCodes');
goog.require('i18n.input.chrome.inputview.util');
goog.require('i18n.input.chrome.message.Name');
goog.require('i18n.input.chrome.message.Type');
goog.require('i18n.input.chrome.options.OptionStorageHandlerFactory');
goog.require('i18n.input.chrome.options.OptionType');
goog.require('i18n.input.chrome.xkb.Correction');
goog.require('i18n.input.lang.InputTool');

goog.scope(function() {
var Constant = i18n.input.chrome.Constant;
var Env = i18n.input.chrome.Env;
var FeatureName = i18n.input.chrome.FeatureName;
var KeyCodes = i18n.input.chrome.events.KeyCodes;
var Name = i18n.input.chrome.message.Name;
var OptionType = i18n.input.chrome.options.OptionType;
var OptionStorageHandlerFactory =
    i18n.input.chrome.options.OptionStorageHandlerFactory;
var TriggerType = i18n.input.chrome.TriggerType;
var Type = i18n.input.chrome.message.Type;
var util = i18n.input.chrome.inputview.util;
var InputTool = i18n.input.lang.InputTool;


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
 * Max length of surrounding text events.
 *
 * @const {number}
 */
var MAX_SURROUNDING_TEXT_LENGTH = 100;


/**
 * The number of gesture results to return to the UI.
 *
 * @const {number}
 */
var GESTURE_RESULTS_TO_RETURN = 4;



/**
 * The background page.
 *
 * @extends {i18n.input.chrome.AbstractController}
 * @constructor
 */
i18n.input.chrome.xkb.Controller = function() {
  i18n.input.chrome.xkb.Controller.base(this, 'constructor');

  if (window.xkb && xkb.Model) {
    this.model_ = new xkb.Model(5,
        this.onCandidatesBack_.bind(this),
        this.onGesturesBack_.bind(this));
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
 * The model.
 *
 * @private {xkb.Model}
 */
Controller.prototype.model_ = null;


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
 * Whether or not the last composition set was from a gesture.
 *
 * @private {boolean}
 */
Controller.prototype.lastCompositionWasGesture_ = false;


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


/**
 * Whether gesture editing is in progress.
 *
 * @private {boolean}
 */
Controller.prototype.gestureEditingInProgress_ = false;


/**
 * Whether the NACL is enabled.
 *
 * @private {boolean}
 */
Controller.prototype.isNaclEnabled_ = false;


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
        Name.TEXT_BEFORE_CURSOR, this.env.textBeforeCursor,
        Name.ANCHOR, surroundingInfo.anchor,
        Name.FOCUS, surroundingInfo.focus,
        Name.OFFSET, surroundingInfo.offset));
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
  this.updateCompositionSurroundingText();

  this.deleteSurroundingText_(start - cursor, end - start, (function() {
    chrome.input.ime.setComposition(goog.object.create(
        Name.CONTEXT_ID, this.env.context.contextID,
        Name.TEXT, word,
        Name.CURSOR, this.compositionTextCursorPosition_));
    this.model_.sendCompletionRequestForWord(word,
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
  this.isNaclEnabled_ = Env.isXkbAndNaclEnabled(inputToolCode);
  if (this.isNaclEnabled_ && this.model_) {
    this.model_.setLanguage(InputTool.get(inputToolCode).languageCode);
  }
  // In parent activate method will call update option to use
  // this.model_ language files.
  Controller.base(this, 'activate', inputToolCode);
  this.statistics_.setInputMethodId(inputToolCode);
  this.statistics_.setPhysicalKeyboard(false);
};


/** @override */
Controller.prototype.updateOptions = function(inputToolCode) {
  var optionStorageHandler =
      OptionStorageHandlerFactory.getInstance().getHandler(inputToolCode);
  // Override default of false for xkb, all users with no pre-existing
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
  var doubleSpacePeriod = /** @type {boolean} */
      (optionStorageHandler.get(OptionType.DOUBLE_SPACE_PERIOD));
  var soundOnKeypress = /** @type {boolean} */
      (optionStorageHandler.get(OptionType.SOUND_ON_KEYPRESS));
  var correctionLevel = /** @type {number} */ (optionStorageHandler.get(
      OptionType.AUTO_CORRECTION_LEVEL));
  var autoCapital = /** @type {boolean} */ (optionStorageHandler.get(
      OptionType.ENABLE_CAPITALIZATION));
  var enableGestureEditing = /** @type {boolean} */ (optionStorageHandler.get(
      OptionType.ENABLE_GESTURE_EDITING));
  var enableGestureTyping = /** @type {boolean} */ (optionStorageHandler.get(
      OptionType.ENABLE_GESTURE_TYPING));
  this.correctionLevel = correctionLevel;
  if (this.model_) {
    this.model_.setEnableUserDict(true);
    this.model_.setCorrectionLevel(this.correctionLevel);
    this.statistics_.setAutoCorrectLevel(this.correctionLevel);
  }
  this.doubleSpacePeriod_ = doubleSpacePeriod;
  this.updateSettings({
    'autoSpace': true,
    'autoCapital': correctionLevel > 0 && autoCapital,
    'doubleSpacePeriod': doubleSpacePeriod,
    'soundOnKeypress': soundOnKeypress,
    'gestureEditing': enableGestureEditing,
    'gestureTyping': enableGestureTyping
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
  this.model_ && this.model_.clear();
};


/** @override */
Controller.prototype.onCompositionCanceled = function() {
  goog.base(this, 'onCompositionCanceled');
  this.clearCandidates_();
  this.compositionText_ = '';
  this.compositionTextCursorPosition_ = 0;
  this.committedText_ = '';
  this.model_ && this.model_.clear();
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
  // Override the trigger type with GESTURE. This needs to be done because
  // gesture commit events only result in setting the gesture text as the
  // current composition, rather than actually committing it.
  if (this.lastCompositionWasGesture_) {
    this.lastCommitType_ = TriggerType.GESTURE;
    this.lastCompositionWasGesture_ = false;
  } else {
    this.lastCommitType_ = triggerType;
  }
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
    if (this.model_) {
      // If source is the same with commit text, don't increase frequency.
      if (text != this.compositionText_ &&
          triggerType == TriggerType.CANDIDATE) {
        // Only increase frequency if the user selected this candidate manually.
        this.model_.changeWordFrequency(target, 1);
      }
      if (this.isSuggestionSupported()) {
        this.model_.sendPredictionRequest(
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
 * @param {boolean=} opt_isGesture .
 * @private
 */
Controller.prototype.setComposition_ = function(text, append, normalizable,
    opt_spatialData, opt_isGesture) {
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

  if (opt_isGesture) {
    this.lastCompositionWasGesture_ = true;
  } else {
    this.lastCompositionWasGesture_ = false;
    if (this.compositionText_) {
      if (this.compositionTextCursorPosition_ == this.compositionText_.length) {
        this.model_ && this.model_.sendCompletionRequest(
            this.compositionText_,
            this.filterPreviousText_(this.env.textBeforeCursor),
            opt_spatialData);
      } else {
        // sendCompletionRequest assumes spatia data is for the last character
        // in compositionText_. In cases that we want to add/delete characters
        // beside the last one in compositionText_, we use
        // sendCompletionRequestForWord. Spatial data is not use here.
        this.model_ && this.model_.sendCompletionRequestForWord(
            this.compositionText_,
            this.filterPreviousText_(this.env.textBeforeCursor));
      }
    } else {
      this.clearCandidates_();
    }
  }
  this.updateCompositionSurroundingText();
};


/**
 * Callback when candidates are fetched back.
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
 * Callback when gesture response is fetched back.
 *
 * @param {!Object} response .
 * @private
 */
Controller.prototype.onGesturesBack_ = function(response) {
  var results = response.results;
  if (results.length > GESTURE_RESULTS_TO_RETURN) {
    results = results.slice(0, GESTURE_RESULTS_TO_RETURN);
  }
  chrome.runtime.sendMessage(goog.object.create(
      Name.TYPE, Type.GESTURES_BACK,
      Name.MSG, goog.object.create(
          Name.GESTURE_RESULTS, response
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
      (this.lastCommitType_ == TriggerType.CANDIDATE ||
          this.lastCommitType_ == TriggerType.GESTURE);
};


/**
 * Returns true if a space should be automatically inserted before the next
 * gestured text.
 *
 * @return {boolean}
 * @private
 */
Controller.prototype.shouldAutoSpaceForGesture_ = function() {
  if (!this.env.textBeforeCursor) {
    return false;
  }
  var charBeforeCursor =
      this.env.textBeforeCursor.charAt(this.env.textBeforeCursor.length - 1);
  // If the text before the cursor was not whitespace, a space should be added
  // before a gesture text is inserted.
  return !(charBeforeCursor == ' ' || charBeforeCursor == '\n');
};


/**
 * True if the word which has cursor should be set to composition mode.
 *
 * @return {boolean}
 * @private
 */
Controller.prototype.shouldSetComposition_ = function() {
  // Sets exisiting word to composition mode if:
  // 1. Suggestion is available and enabled.
  // 2. No composition in progress.
  // 3. No text selection.
  // 4. Surround text is not empty.
  // 5. Virtual keyboard is visible.
  // 6. skipNextSetComposition_ is false.
  // 7. Not typing on the physical keyboard.
  // 8. Not gesture editing.
  return this.env.featureTracker.isEnabled(FeatureName.EXPERIMENTAL) &&
         !!this.model_ &&
         this.isSuggestionSupported() &&
         !this.compositionText_ &&
         this.env.surroundingInfo.focus == this.env.surroundingInfo.anchor &&
         this.env.surroundingInfo.text.length > 0 &&
         this.isVisible_ &&
         !this.skipNextSetComposition_ &&
         !this.usingPhysicalKeyboard_ &&
         !this.gestureEditingInProgress_;
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
      firstCandidate != this.compositionText_ &&
      !this.lastCompositionWasGesture_) {
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
    this.model_.sendCompletionRequestForWord(this.compositionText_,
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
        if (this.lastCompositionWasGesture_) {
          // Backspace clears the gesture-typed composition.
          this.setComposition_('', false, false);
          this.statistics_.recordEnum(
              i18n.input.chrome.Statistics.GESTURE_TYPING_METRIC_NAME,
              i18n.input.chrome.Statistics.GestureTypingEvent.DELETED,
              i18n.input.chrome.Statistics.GestureTypingEvent.MAX);
        } else if (this.compositionTextCursorPosition_ == 0) {
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
              this.model_.sendCompletionRequestForWord(word,
                  this.filterPreviousText_(this.env.textBeforeCursor));
            }).bind(this));
            this.updateCompositionSurroundingText();
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
          this.model_.sendCompletionRequestForWord(this.compositionText_,
              this.filterPreviousText_(this.env.textBeforeCursor));
          this.updateCompositionSurroundingText();
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

    if (!this.isNaclEnabled_ || !this.model_ || !this.model_.isReady() ||
        !this.isSuggestionSupported()) {
      this.commitText_(ch, true, TriggerType.RESET, true);
    } else {
      if (this.lastCompositionWasGesture_) {
        // Commit any composed text from a gesture. This call will force the
        // auto space below.
        this.commitText_('', false, TriggerType.GESTURE, false);
      }
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
    case Type.CONFIRM_GESTURE_RESULT:
      var forceAutoSpace = message[Name.FORCE_AUTO_SPACE];
      if (this.shouldAutoSpaceForGesture_() || forceAutoSpace) {
        this.commitText(' ', false);
      }
      var committingText = message[Name.TEXT];
      this.setComposition_(committingText, false, false, undefined, true);
      break;
    case Type.CONNECT:
      this.updateOptions(this.env.engineId);
      break;
    case Type.SEND_GESTURE_EVENT:
      // Commit the previous composition.
      if (this.compositionText_) {
        // TODO: This commitText_ call does not seem to trigger
        // onSurroundingTextChanged, and so we are always one commit-text late
        // when trying to figure out autospacing in the CONFIRM_GESTURE_RESULT
        // handling code. However, commiting actual text does seem to trigger
        // onSurroundingTextChanged.
        this.commitText_('', true, TriggerType.RESET, false);
        // Force onSurroundingText to update.
        this.setComposition_(' ', false, false);
        this.setComposition_('', false, false);
      }
      if (this.model_) {
        var gestureData = /** @type {!Array.<!Object>} */
            (message[Name.GESTURE_DATA]);
        this.model_.decodeGesture(gestureData, this.env.textBeforeCursor);
      }
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
    case Type.SEND_KEYBOARD_LAYOUT:
      if (this.model_) {
        var keyboardLayout = /** @type {?Object} */
            (message[Name.KEYBOARD_LAYOUT]);
        this.model_.sendKeyboardLayout(keyboardLayout);
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
    case Type.SET_GESTURE_EDITING:
      this.gestureEditingInProgress_ = message[Name.IN_PROGRESS];
      var isSwipe = message[Name.IS_SWIPE];
      // Commit any existing commit text.
      if (this.compositionText_ && this.gestureEditingInProgress_) {
        this.commitText_('', false, TriggerType.COMPOSITION_DISABLED, false);
        if (isSwipe) {
          // If the first action is a swipe, need to send a surrounding text
          // changed event since an IMF bug prevents it from being sent.
          this.updateCompositionSurroundingText();
        }
      }
      break;
  }
  goog.base(this, 'processMessage', message, sender, sendResponse);
};


/**
 * Updates surrounding text as a result of change in composition text.
 *
 * This is a workaround to the fact that composition changes do not cause IMF
 * to send the native event.
 */
Controller.prototype.updateCompositionSurroundingText = function() {
  var text = this.env.textBeforeCursor + this.compositionText_;
  var commitLength = this.compositionText_.length;

  chrome.runtime.sendMessage(goog.object.create(
      Name.TYPE, Type.SURROUNDING_TEXT_CHANGED,
      Name.TEXT_BEFORE_CURSOR, text,
      Name.ANCHOR, this.env.surroundingInfo.anchor + commitLength,
      Name.FOCUS, this.env.surroundingInfo.focus + commitLength,
      Name.OFFSET, this.env.surroundingInfo.offset));
};


/** @override */
Controller.prototype.commitText = function(text, onStage) {
  // Commit text will result onSurroundingTextChanged event. It is not necessary
  // to try to set a just committed word to composition mode in this case, so
  // set skipNextSetComposition_ to true.
  this.skipNextSetComposition_ = true;
  goog.base(this, 'commitText', text, onStage);
};


/** @override */
Controller.prototype.isSuggestionSupported = function() {
  var context = this.env.context;
  if (!context || !goog.array.contains(['text', 'search'], context.type)) {
    return false;
  }
  return !goog.isDef(context.autoCorrect) || !!context.autoCorrect;
};
});  // goog.scope
