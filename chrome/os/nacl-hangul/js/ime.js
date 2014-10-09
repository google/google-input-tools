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
// Copyright 2013 Google Inc. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * @fileoverview Hangul IME for ChromeOS with IME Extension API.
 */

'use strict';

/**
 * Hangul IME with IME extension API based on Native Client and libhangul.
 *
 * @constructor
 */
function HangulIme() {
  /**
   * Context information which is provided by Chrome.
   *
   * @type {?Object}
   * @private
   */
  this.context_ = null;

  /**
   * Engine id which is specified on manifest.js.
   *
   * @type {string}
   * @private
   */
  this.engineID_ = '';

  /**
   * Show vertical candidates window rather than horizontal.
   *
   * @type {boolean}
   * @private
   */
  this.candidateWindowVertical_ = true;

  /**
   * Number of candidates per page on candidates window.
   *
   * @type {number}
   * @private
   */
  this.candidateWindowPageSize_ = 10;

  /**
   * Raw input text.
   *
   * @type {string}
   * @private
   */
  this.inputText_ = '';

  /**
   * Commit text. This is a volatile property, and will be cleared by
   * {@code HangulIme.commit_}.
   *
   * @type {?string}
   * @private
   */
  this.commitText_ = null;

  /**
   * Editing hangul text.
   *
   * @type {string}
   * @private
   */
  this.hangul_ = '';

  /**
   * Matched lengths of every hangul character.
   *
   * @type {Array.<number>}
   * @private
   */
  this.hangulLengths_ = [];

  /**
   * Segment information.
   *
   * @type {HangulIme.Segment}
   * @private
   */
  this.segment_ = null;

  /**
   * Cursor position.
   *
   * @type {number}
   * @private
   */
  this.cursor_ = 0;

  /**
   * The state of the IME.
   *
   * @type {HangulIme.State}
   * @private
   */
  this.state_ = HangulIme.State.RESET;

  /**
   * Flag of conversion to Hanja automatically when typing.
   *
   * @type {boolean}
   * @private
   */
  this.autoHanjaConversion_ = false;

  /**
   * Native Client Module
   *
   * @type {Object}
   * @private
   */
  this.naclModule_ = null;

  /**
   * Ignore key table not to handle some keys.
   *
   * @type {Object}
   * @private
   */
  this.ignoreKeySet_ = {};

  var ignoreKeys = [{
    'key': 'ChromeOSSwitchWindow',
    'ctrlKey': true
  }, {
    'key': 'ChromeOSSwitchWindow',
    'ctrlKey': true,
    'shiftKey': true
  }];
  for (var i = 0; i < ignoreKeys.length; i++) {
    this.ignoreKeySet_[this.stringifyKeyAndModifiers_(ignoreKeys[i])] = true;
  }

  /**
   * Menu items of this IME.
   *
   * @type {Array}
   * @private
   */
  this.menuItems_ = [{
    'id': HangulIme.Menu.HANGUL_MODE,
    'label': 'Hangul Mode',
    'style': 'radio',
    'checked': true
  }, {
    'id': HangulIme.Menu.HANJA_MODE,
    'label': 'Hanja Mode',
    'style': 'radio',
    'checked': false
  }];

  /**
   * Current keyboard layout
   *
   * @type {Object}
   * @private
   */
  this.currentKeyboard_ = HangulIme.keyboardLayouts[0];

  this.clear_();
}


/**
 * Enum of IME state.
 *
 * @enum {number}
 */
HangulIme.State = {
  /**
   * IME doesn't have any input text.
   */
  RESET: 0,
  /**
   * IME has a input text.
   */
  HANGUL: 1,
  /**
   * IME has a input text, and hanja is being selected.
   */
  HANJA: 2,
  /**
   * English letter is directly committed.
   */
  ENGLISH: 3
};


/**
 * Enum of Key Event type.
 *
 * @enum {string}
 */
HangulIme.KeyEventType = {
  KEYUP: 'keyup',
  KEYDOWN: 'keydown'
};


/**
 * Enum of menu items.
 *
 * @enum {string}
 */
HangulIme.Menu = {
  HANGUL_MODE: 'menu-hangul-mode',
  HANJA_MODE: 'menu-hanja-mode'
};


/**
 * Hanja Candidate.
 *
 * @param {string} text Text of the candidate.
 * @param {string} annotation Annotation of the candidate.
 * @param {number} matchedLength Matched length of the candidate.
 * @constructor
 */
HangulIme.Candidate = function(text, annotation, matchedLength) {
  /**
   * Text.
   *
   * @type {string}
   */
  this.text = text;

  /**
   * Annotation.
   *
   * @type {string}
   */
  this.annotation = annotation;

  /**
   * Matched length.
   *
   * @type {number}
   */
  this.matchedLength = matchedLength;
};


/**
 * Segment information of a HANGUL text.
 *
 * @constructor
 */
HangulIme.Segment = function() {
  /**
   * Candidates list.
   *
   * @type {Array.<Object>}
   */
  this.candidates = [];

  /**
   * Focused candidate index.
   *
   * @type {number}
   */
  this.focusedIndex = 0;
};


/**
 * Clears properties of IME.
 *
 * @private
 */
HangulIme.prototype.clear_ = function() {
  this.inputText_ = '';
  this.commitText_ = null;
  this.hangul_ = '';
  this.hangulLengths_ = [];
  this.resetSegment_();
  this.cursor_ = 0;
  if (this.state_ !== HangulIme.State.ENGLISH) {
    this.state_ = HangulIme.State.RESET;
  }
};


/**
 * Stringifies key event data.
 *
 * @param {Object} keyData key event data.
 * @return {string} stringified key event data.
 * @private
 */
HangulIme.prototype.stringifyKeyAndModifiers_ = function(keyData) {
  var keys = [
    keyData['key']
  ];
  if (keyData['altKey']) {
    keys.push('alt');
  }
  if (keyData['ctrlKey']) {
    keys.push('ctrl');
  }
  if (keyData['shiftKey']) {
    keys.push('shift');
  }
  return keys.join(' ');
};


/**
 * Resets segment.
 *
 * @private
 */
HangulIme.prototype.resetSegment_ = function() {
  this.segment_ = new HangulIme.Segment();
};

/**
 * Requests candidates if input text.
 *
 * @param {string=} opt_text input text to generate candidates.
 *                          If not given, this.inputText_ will be used.
 * @private
 */
HangulIme.prototype.requestCandidates_ = function(opt_text) {
  var text = opt_text || this.inputText_;
  if (!text) {
    return;
  }
  var request = JSON.stringify({
    'text': text,
    'num': 0
  });

  // this.updateCandidates will be called after NaCl responded
  this.naclModule_.postMessage(request);
};


/**
 * Updates candidates
 *
 * @param {Array} data of candidates.
 *        data[0] is the original text converted.
 *        data[1] is the list of candidates.
 *        data[2] is matched length of original text.
 *        data[3] is an object containing other properties, which are:
 *                matched_length, same as data[2].
 *                annotations, annotation of every candidate.
 */
HangulIme.prototype.updateCandidates = function(data) {
  var segment = this.segment_;
  segment.focusedIndex = 0;

  var originalText = data[0];
  var candidatesList = data[1];
  var matched = data[2];
  var otherProps = data[3];
  var annotations = otherProps['annotation'];

  // Determine if results are Hanja by seeing if original text is Hangul
  var isHanja = false;
  var asciiRegex = /^[\000-\255]*$/;
  if (!asciiRegex.test(originalText)) {
    isHanja = true;
  }

  segment.candidates = [];
  if (candidatesList && candidatesList.length > 0) {
    if (!isHanja) {
      this.hangul_ = candidatesList.join('');
      this.hangulLengths_ = matched;
      // Converts Hangul to Hanja automatically if autoHanjaConversion_ is true
      if (this.autoHanjaConversion_) {
        this.requestCandidates_(this.hangul_);
        return;
      }
    } else {
      // Generate Hanja candidates and transfer to HANJA state
      for (var i = 0; i < candidatesList.length; i++) {
        var candidate = new HangulIme.Candidate(candidatesList[i],
            annotations[i], matched[i]);
        segment.candidates.push(candidate);
      }
      this.state_ = HangulIme.State.HANJA;
    }
  }
  // Update preedit text and candidate window
  this.update_();
};


/**
 * Gets preedit text.
 *
 * @return {string} preedit text.
 * @private
 */
HangulIme.prototype.getPreeditText_ = function() {
  return this.hangul_;
};


/**
 * Updates preedit text.
 *
 * @private
 */
HangulIme.prototype.updatePreedit_ = function() {
  if (this.state_ === HangulIme.State.RESET) {
    chrome.input.ime.setComposition({
      'contextID': this.context_.contextID,
      'text': '',
      'cursor': 0
    });
    return;
  }

  var preeditText = this.getPreeditText_();
  var segmentData = {
    'start': 0,
    'end': preeditText.length,
    'style': 'underline'
  };

  var composition = {
    'contextID': this.context_.contextID,
    'text': preeditText,
    'segments': [
      segmentData
    ],
    'cursor': segmentData.end
  };

  if (this.state_ === HangulIme.State.HANGUL) {
    composition['selectionStart'] = segmentData.start;
    composition['selectionEnd'] = segmentData.end;
  }

  chrome.input.ime.setComposition(composition);
};


/**
 * Updates candidates window.
 *
 * @private
 */
HangulIme.prototype.updateCandidatesWindow_ = function() {
  if (this.state_ === HangulIme.State.RESET) {
    chrome.input.ime.setCandidateWindowProperties({
      'engineID': this.engineID_,
      'properties': {
        'visible': false,
        'auxiliaryTextVisible': false
      }
    });
  } else {
    var segment = this.segment_;
    var visible = false;
    var auxiliaryText = '';
    // Show candidate window in Hanja mode
    if (this.state_ === HangulIme.State.HANJA &&
        segment.candidates.length > 0) {
      visible = true;
      // Trim candidates for faster display
      var numPages = Math.ceil(
        segment.candidates.length / this.candidateWindowPageSize_);
      var page = Math.floor(
        segment.focusedIndex / this.candidateWindowPageSize_);
      var displayCandidates = segment.candidates.slice(
        page * this.candidateWindowPageSize_,
        (page + 1) * this.candidateWindowPageSize_);
      // Set id and label for candidates to display
      for (var i = 0; i < displayCandidates.length; i++) {
        var num = i + 1;
        if (num == 10) {
          num = 0;
        }
        displayCandidates[i] = {
          'candidate': displayCandidates[i].text,
          'annotation': displayCandidates[i].annotation,
          'id': i,
          'label': num.toString()
        };
      }
      auxiliaryText = (page + 1) + '/' + numPages + ' Google Hangul IME';
      chrome.input.ime.setCandidates({
        'contextID': this.context_.contextID,
        'candidates': displayCandidates
      });
      var offsetInPage = segment.focusedIndex % this.candidateWindowPageSize_;
      chrome.input.ime.setCursorPosition({
        'contextID': this.context_.contextID,
        'candidateID': offsetInPage
      });
    }
    chrome.input.ime.setCandidateWindowProperties({
      'engineID': this.engineID_,
      'properties': {
        'visible': visible,
        'cursorVisible': visible,
        'vertical': this.candidateWindowVertical_,
        'pageSize': this.candidateWindowPageSize_,
        'auxiliaryTextVisible': visible,
        'auxiliaryText': auxiliaryText
      }
    });
  }
};


/**
 * Commits commit text if {@code commitText_} isn't null. This function clears
 * {@code commitText_} since it is a volatile property.
 *
 * @private
 */
HangulIme.prototype.commit_ = function() {
  if (this.commitText_ == null) {
    return;
  }

  chrome.input.ime.commitText({
    'contextID': this.context_.contextID,
    'text': this.commitText_
  });

  this.commitText_ = null;
};


/**
 * Updates output using IME Extension API.
 *
 * @private
 */
HangulIme.prototype.update_ = function() {
  this.updatePreedit_();
  this.updateCandidatesWindow_();
  this.commit_();
};


/**
 * Prepares commit text using preedit text and clears a context.
 *
 * @private
 */
HangulIme.prototype.prepareCommitText_ = function() {
  if (this.state_ === HangulIme.State.RESET) {
    return;
  }
  var commitText = '';
  if (this.state_ === HangulIme.State.HANJA) {
    // Commit the pointing candidate
    var segment = this.segment_;
    if (segment.candidates.length > segment.focusedIndex) {
      commitText = segment.candidates[segment.focusedIndex].text;
    }
  } else {
    commitText = this.getPreeditText_();
  }
  this.clear_();
  this.commitText_ = commitText;
};


/**
 * Prepares commit text using preedit text and clears a context.
 *
 * @private
 */
HangulIme.prototype.commitCandidate_ = function() {
  if (this.state_ === HangulIme.State.HANJA) {
    // Find the pointing candidate
    var segment = this.segment_;
    if (segment.candidates.length > segment.focusedIndex) {
      var candidate = segment.candidates[segment.focusedIndex];
      if (candidate.matchedLength === this.hangul_.length) {
        this.clear_();
      } else {
        this.hangul_ = this.hangul_.slice(candidate.matchedLength);
        var inputOffset = 0;
        for (var i = 0; i < candidate.matchedLength; i++) {
          inputOffset += this.hangulLengths_[i];
        }
        this.inputText_ = this.inputText_.slice(inputOffset);
        this.moveCursor_(this.inputText_.length);
        this.requestCandidates_(this.hangul_);
      }
      this.commitText_ = candidate.text;
    }

    if (!this.commitText_) {
      // If there is no candidate, the commit text is set to the source.
      var commitText = this.getPreeditText_();
      this.clear_();
      this.commitText_ = commitText;
    }
  }
  this.update_();
};


/**
 * Inserts characters into the cursor position.
 *
 * @param {string} value text we want to insert into.
 * @private
 */
HangulIme.prototype.insertChar_ = function(value) {
  var text = this.inputText_;
  this.inputText_ =
    text.slice(0, this.cursor_) + value + text.slice(this.cursor_);
  this.moveCursor_(this.cursor_ + value.length);
};


/**
 * Removes a character.
 *
 * @param {number} index index of the character you want to remove.
 * @private
 */
HangulIme.prototype.removeChar_ = function(index) {
  if (index < 0 || this.inputText_.length <= index) {
    return;
  }

  this.inputText_ =
    this.inputText_.substr(0, index) + this.inputText_.substr(index + 1);

  if (this.inputText_.length === 0) {
    this.clear_();
    this.update_();
    return;
  }

  if (index < this.cursor_) {
    this.moveCursor_(this.cursor_ - 1);
  }
};


/**
 * Moves a cursor position.
 *
 * @param {number} index Cursor position.
 * @private
 */
HangulIme.prototype.moveCursor_ = function(index) {
  if (index < 0 || this.inputText_.length < index) {
    return;
  }
  this.cursor_ = index;
};


/**
 * Selects a candidate.
 *
 * @param {number} candidateID index of the candidate.
 * @private
 */
HangulIme.prototype.focusCandidate_ = function(candidateID) {
  if (this.state_ === HangulIme.State.RESET) {
    return;
  }

  var segment = this.segment_;
  if (candidateID < 0 || segment.candidates.length <= candidateID) {
    return;
  }

  segment.focusedIndex = candidateID;
};


/**
 * Determines whether keyData is a valid character of current keyboard
 *
 * @param {Object} keyData
 * @return {boolean}
 * @private
 */
HangulIme.prototype.isValidChar_ = function(keyData) {
  if (this.currentKeyboard_.validKeys.indexOf(keyData['key']) === -1) {
    return false;
  }
  return true;
};


/**
 * Handles an alphabet key.
 *
 * @param {Object} keyData key event data.
 * @return {boolean} true if key event is consumed.
 * @private
 */
HangulIme.prototype.handleKey_ = function(keyData) {
  if (this.handleSpecialKey_(keyData)) {
    return true;
  }
  // Does not handle key with modifiers
  if (keyData['altKey'] || keyData['ctrlKey']) {
    this.prepareCommitText_();
    this.update_();
    return false;
  }
  // Does not handle single Shift key
  if (keyData['key'] === 'Shift') {
    return false;
  }
  // Directly commit in English mode
  if (this.state_ === HangulIme.State.ENGLISH) {
    return false;
  }
  // Check char map
  if (this.currentKeyboard_.charMap[keyData['key']] != undefined) {
    // Commit current preedit text
    this.prepareCommitText_();
    this.update_();
    // Commit mapped char
    this.commitText_ = this.currentKeyboard_.charMap[keyData['key']];
    this.commit_();
    return true;
  }
  // Handles special character '\' and '|' in hanja mode.
  if (this.state_ == HangulIme.State.HANJA && (
      keyData['key'] == '|' || keyData['key'] == '\\')) {
    this.prepareCommitText_();
    this.update_();
    // Commit mapped char
    this.commitText_ = keyData['key'] == '|' ? '\u20a9' : '\uffe6';
    this.commit_();
    return true;
  }
  // Commit current preedit text and let system handle the key if not valid
  if (!this.isValidChar_(keyData)) {
    this.prepareCommitText_();
    this.update_();
    return false;
  }
  // Change state
  switch (this.state_) {
    case HangulIme.State.RESET:
      this.state_ = HangulIme.State.HANGUL;
      this.resetSegment_();
      break;
    case HangulIme.State.HANGUL:
      break;
    case HangulIme.State.HANJA:
      // If Hanja mode is transfered by HanjaKey, and a valid char key is
      // pressed, then do nothing and the char key is consumed
      if (!this.autoHanjaConversion_){
        return true;
      }
      // Or else Hanja mode is transfered automatically by HanjaMode, append
      // the char
      break;
    default:
      break;
  }
  // Insert a char to position of cursor
  this.insertChar_(keyData['key']);
  this.requestCandidates_();
  return true;
};


/**
 * Handles an arrow key (Left, Right, Up, Down).
 *
 * @param {Object} keyData key event data.
 * @return {boolean} true if key event is consumed.
 * @private
 */
HangulIme.prototype.handleArrowKey_ = function(keyData) {
  var segment = this.segment_;
  var index = segment.focusedIndex;
  var numPages = Math.ceil(
    segment.candidates.length / this.candidateWindowPageSize_);
  var currentPage = Math.floor(index / this.candidateWindowPageSize_);
  if (keyData['key'] === 'Up') {
    // Previous candidate
    index--;
    if (index === -1) {
      index = segment.candidates.length - 1;
    }
  } else if (keyData['key'] === 'Down') {
    // Next candidate
    index++;
    if (index === segment.candidates.length) {
      index = 0;
    }
  } else if (keyData['key'] === 'Left') {
    // Previous page
    currentPage--;
    if (currentPage === -1) {
      currentPage = numPages - 1;
    }
    index = currentPage * this.candidateWindowPageSize_;
  } else if (keyData['key'] === 'Right') {
    // Next page
    currentPage++;
    if (currentPage === numPages) {
      currentPage = 0;
    }
    index = currentPage * this.candidateWindowPageSize_;
  }
  this.focusCandidate_(index);
  this.updatePreedit_();
  this.updateCandidatesWindow_();
  return true;
};


/**
 * Handles a non-alphabet key.
 *
 * @param {Object} keyData key event data.
 * @return {boolean} true if key event is consumed.
 * @private
 */
HangulIme.prototype.handleSpecialKey_ = function(keyData) {
  if (keyData['altKey'] || keyData['ctrlKey'] || keyData['shiftKey']) {
    if (keyData['code'] != 'AltRight' && keyData['code'] != 'ControlRight') {
      return false;
    }
  }

  // When pressing Esc, discard preedit text and reset
  if (keyData['code'] === 'Escape') {
    if (this.state_ === HangulIme.State.HANGUL) {
      this.clear_();
      this.update_();
      return true;
    }
    if (this.state_ === HangulIme.State.HANJA) {
      this.state_ = HangulIme.State.HANGUL;
      this.update_();
      return true;
    }
  }

  // Switch to or from English mode when pressing Right Alt for Hangul key
  if (keyData['code'] === 'AltRight' || keyData['code'] === 'HangulMode') {
    if (this.state_ === HangulIme.State.ENGLISH) {
      this.clear_();
      this.state_ = HangulIme.State.RESET;
    } else {
      this.prepareCommitText_();
      this.update_();
      this.clear_();
      this.state_ = HangulIme.State.ENGLISH;
    }
    return true;
  }

  if (keyData['code'] === 'ControlRight' || keyData['code'] === 'Hanja') {
    if (this.state_ === HangulIme.State.HANGUL) {
      var hangul = this.getPreeditText_();
      this.requestCandidates_(hangul);
      return true;
    } else if (this.state_ === HangulIme.State.RESET ||
          this.state_ === HangulIme.State.ENGLISH) {
      this.state_ = HangulIme.State.RESET;
      if (this.autoHanjaConversion_) {
        this.onMenuItemActivated(this.engineID_, HangulIme.Menu.HANGUL_MODE);
      } else {
        this.onMenuItemActivated(this.engineID_, HangulIme.Menu.HANJA_MODE);
      }
      return true;
    } else if (this.state_ === HangulIme.State.HANJA) {
      // Go back to Hangul mode and hide the candidate window
      this.state_ = HangulIme.State.HANGUL;
      this.updateCandidatesWindow_();
      return true;
    }
  }

  if (this.state_ === HangulIme.State.HANGUL) {
    if (keyData['key'] === 'Backspace') {
      if (this.cursor_ !== 0) {
        this.removeChar_(this.cursor_ - 1);
        this.requestCandidates_();
      }
      return true;
    } else if (keyData['key'] === 'Delete') {
      if (this.cursor_ !== this.inputText_.length) {
        this.removeChar_(this.cursor_);
        this.requestCandidates_();
      }
      return true;
    } else if (keyData['key'] === 'Enter') {
      this.prepareCommitText_();
      this.update_();
      return false;
    }
  }

  if (this.state_ === HangulIme.State.HANJA) {
    var segment = this.segment_;
    if (keyData['key'] === 'Up' || keyData['key'] === 'Down' ||
        keyData['key'] === 'Left' || keyData['key'] === 'Right') {
      return this.handleArrowKey_(keyData);
    } else if (/[0-9]/.test(keyData['key'])) {
      // Select candidate by number key
      var offset = (keyData['key'] - '0') - 1;
      if (offset == -1) {
        offset = 9;
      }
      var pageIndex = Math.floor(
        segment.focusedIndex / this.candidateWindowPageSize_) *
        this.candidateWindowPageSize_;
      var index = pageIndex + offset;
      if (index < segment.candidates.length) {
        this.focusCandidate_(index);
        this.commitCandidate_();
      }
      return true;
    } else if (keyData['key'] === 'Backspace' || keyData['key'] === 'Delete') {
      if (this.autoHanjaConversion_) {
        // Remove last char
        if (this.cursor_ !== 0) {
          this.removeChar_(this.cursor_ - 1);
        }
      } else {
        // Do nothing and consume the backspace or delete key
        return true;
      }
      this.resetSegment_();
      this.requestCandidates_();
      return true;
    } else if (keyData['key'] === ' ' || keyData['key'] === 'Enter') {
      this.commitCandidate_();
      return true;
    }
  }
  return false;
};


/**
 * Callback method. It is called when IME is activated.
 *
 * @param {string} engineID engine ID.
 */
HangulIme.prototype.onActivate = function(engineID) {
  this.engineID_ = engineID;
  this.clear_();
  this.setUpMenu_();
};


/**
 * Callback method. It is called when IME is deactivated.
 *
 * @param {string} engineID engine ID.
 */
HangulIme.prototype.onDeactivated = function(engineID) {
  this.clear_();
  this.engineID_ = '';
};


/**
 * Callback method. It is called when a context acquires a focus.
 *
 * @param {Object} context context information.
 */
HangulIme.prototype.onFocus = function(context) {
  this.context_ = context;
  this.clear_();
  this.updateCandidatesWindow_();
};


/**
 * Callback method. It is called when a context lost a focus.
 *
 * @param {number} contextID ID of the context.
 */
HangulIme.prototype.onBlur = function(contextID) {
  this.clear_();
  this.updateCandidatesWindow_();
  this.context_ = null;
};


/**
 * Callback method. It is called when context is reset.
 *
 * @param {number} contextID ID of the context.
 */
HangulIme.prototype.onReset = function(contextID) {
  this.clear_();
  this.updateCandidatesWindow_();
};


/**
 * Callback method. It is called when properties of the context is changed.
 *
 * @param {Object} context context information.
 */
HangulIme.prototype.onInputContextUpdate = function(context) {
  this.context_ = context;
  this.update_();
};


/**
 * Callback method. It is called when IME catches a new key event.
 *
 * @param {string} engineID ID of the engine.
 * @param {Object} keyData key event data.
 * @return {boolean} true if the key event is consumed.
 */
HangulIme.prototype.onKeyEvent = function(engineID, keyData) {
  if (keyData.type !== HangulIme.KeyEventType.KEYDOWN) {
    return false;
  }

  if (this.ignoreKeySet_[this.stringifyKeyAndModifiers_(keyData)]) {
    return false;
  }

  return this.handleKey_(keyData);
};


/**
 * Callback method. It is called when candidates on candidate window is clicked.
 *
 * @param {number} engineID ID of the engine.
 * @param {number} candidateID Index of the candidate.
 * @param {string} button Which mouse button was clicked.
 */
HangulIme.prototype.onCandidateClicked = function(engineID,
    candidateID, button) {
  if (button === 'left') {
    // Focus and commit
    var pageIndex = Math.floor(
      this.segment_.focusedIndex / this.candidateWindowPageSize_) *
      this.candidateWindowPageSize_;
    this.focusCandidate_(pageIndex + candidateID);
    this.commitCandidate_();
  }
};


/**
 * Sets up a menu on a uber tray.
 *
 * @private
 */
HangulIme.prototype.setUpMenu_ = function() {
  chrome.input.ime.setMenuItems({
    'engineID': this.engineID_,
    'items': this.menuItems_
  });
};


/**
 * Callback method. It is called when menu item on uber tray is activated.
 *
 * @param {string} engineID ID of the engine.
 * @param {string} id ID of the menu item.
 */
HangulIme.prototype.onMenuItemActivated = function(engineID, id) {
  if (engineID !== this.engineID_) {
    return;
  }
  if (id == HangulIme.Menu.HANGUL_MODE) {
    this.autoHanjaConversion_ = false;
    this.getMenuItem_(HangulIme.Menu.HANGUL_MODE)['checked'] = true;
    this.getMenuItem_(HangulIme.Menu.HANJA_MODE)['checked'] = false;
  } else if (id == HangulIme.Menu.HANJA_MODE) {
    this.autoHanjaConversion_ = true;
    this.getMenuItem_(HangulIme.Menu.HANGUL_MODE)['checked'] = false;
    this.getMenuItem_(HangulIme.Menu.HANJA_MODE)['checked'] = true;
  }
  chrome.input.ime.updateMenuItems({
    'engineID': this.engineID_,
    'items': this.menuItems_
  });
};


/**
 * Gets the menu item by ID.
 *
 * @param {string} id ID of the menu item.
 * @return {Object} Menu item.
 * @private
 */
HangulIme.prototype.getMenuItem_ = function(id) {
  for (var i = 0; i < this.menuItems_.length; i++) {
    if (this.menuItems_[i].id === id) {
      return this.menuItems_[i];
    }
  }
  return null;
};

/**
 * Switches to another keyboard layout.
 *
 * @param {string} engineID ID of input method.
 */
HangulIme.prototype.switchKeyboard = function(engineID) {
  var keyboard = null;
  for (var i = 0; i < HangulIme.keyboardLayouts.length; i++) {
    if (HangulIme.keyboardLayouts[i].id == engineID) {
      keyboard = HangulIme.keyboardLayouts[i];
    }
  }
  if (keyboard != null && keyboard != this.currentKeyboard_) {
    this.currentKeyboard_ = keyboard;
    // Send message to NaCl to switch keyboard layout
    var request = JSON.stringify({
      'keyboard': keyboard.name
    });
    this.naclModule_.postMessage(request);
  }
};

/**
 * Sets Native Client module.
 *
 * @param {Object} naclModule
 */
HangulIme.prototype.setNaclModule = function(naclModule) {
  this.naclModule_ = naclModule;
  var handleNaclMessage = HangulIme.prototype.handleNaclMessage.bind(this);
  naclModule.addEventListener('message', handleNaclMessage, true);
};


/**
 * Handles messages post from Native Client module.
 *
 * @param {string} message
 */
HangulIme.prototype.handleNaclMessage = function(message) {
  var response = JSON.parse(message['data']);
  if (!response || response[0] !== 'SUCCESS') {
    console.error('Error from NaCl:');
    console.error(response);
    return;
  }
  var payload = response[1];
  var data = payload[0];
  this.updateCandidates(data);
};


/**
 * Initializes IME.
 */
document.addEventListener('readystatechange', function() {
  var imeInstance = window.imeInstance = new HangulIme;
  if (document.readyState === 'complete') {
    console.log('Initializing Hangul IME...');
    var requestId = 0;
    chrome.input.ime.onActivate.addListener(function(engineID) {
      imeInstance.switchKeyboard(engineID);
      imeInstance.onActivate(engineID);
    });
    chrome.input.ime.onDeactivated.addListener(function(engineID) {
      imeInstance.switchKeyboard(engineID);
      imeInstance.onDeactivated(engineID);
    });
    chrome.input.ime.onFocus.addListener(function(context) {
      imeInstance.onFocus(context);
    });
    chrome.input.ime.onBlur.addListener(function(contextID) {
      imeInstance.onBlur(contextID);
    });
    chrome.input.ime.onInputContextUpdate.addListener(function(context) {
      imeInstance.onInputContextUpdate(context);
    });
    chrome.input.ime.onKeyEvent.addListener(function(engineID, keyData) {
      imeInstance.switchKeyboard(engineID);
      requestId = Number(keyData.requestId) + 1;
      return imeInstance.onKeyEvent(engineID, keyData);
    });
    chrome.input.ime.onCandidateClicked.addListener(
        function(engineID, candidateID, button) {
      imeInstance.switchKeyboard(engineID);
      imeInstance.onCandidateClicked(engineID, candidateID, button);
    });
    chrome.input.ime.onMenuItemActivated.addListener(function(engineID, name) {
      imeInstance.switchKeyboard(engineID);
      imeInstance.onMenuItemActivated(engineID, name);
    });
    var onReset = chrome.input.ime.onReset;
    // This API is only available on Chrome OS 29, add this for compatibility
    if (onReset) {
      onReset.addListener(function(engineID) {
        imeInstance.switchKeyboard(engineID);
        imeInstance.onReset(engineID);
      });
    }
    var enChars = 'qwertyuiopasdfghjklzxcvbnm';
    var hangulChars =
        '\u3142\u3148\u3137\u3131\u3145\u315B\u3155\u3151\u3150\u3154' +
        '\u3141\u3134\u3147\u3139\u314E\u3157\u3153\u314F\u3163' +
        '\u314B\u314C\u314A\u314D\u3160\u315C\u3161';
    var hangulShiftChars =
        '\u3143\u3149\u3138\u3132\u3146\u315B\u3155\u3151\u3152\u3156' +
        '\u3141\u3134\u3147\u3139\u314E\u3157\u3153\u314F\u3163' +
        '\u314B\u314C\u314A\u314D\u3160\u315C\u3161';
    var hangulMap = {};
    for (var i = 0; i < hangulChars.length; ++i) {
      hangulMap[hangulChars[i]] = enChars[i];
      hangulMap[hangulShiftChars[i]] = enChars[i].toUpperCase();
    }
    var handleSoftKey = function(keyData) {
      var kData = {};
      var properties = ['type', 'requestId', 'extensionId', 'key', 'code',
      'keyCode', 'altKey', 'ctrlKey', 'shiftKey', 'capsLock'];
      properties.forEach(function(property) {
        if (keyData[property] !== undefined) {
          kData[property] = keyData[property];
        }
      });
      kData['requestId'] = String(requestId++);
      if (hangulMap[kData['key']]) {
        kData['key'] = hangulMap[kData['key']];
      }
      chrome.input.ime.sendKeyEvents({
          'contextID': 0,
          'keyData': [kData]
      });
    };
    chrome.runtime.onMessage.addListener(function(message) {
      if (message['type'] == 'send_key_event') {
        var keyData = message['keyData'];
        if (keyData.length) {
          keyData.forEach(function(kd) { handleSoftKey(kd); });
        } else {
          handleSoftKey(keyData);
        }
      }
    });
    var naclModule = document.getElementById('nacl_module');
    imeInstance.setNaclModule(naclModule);
    if (chrome.inputMethodPrivate && chrome.inputMethodPrivate.startIme) {
      chrome.inputMethodPrivate.startIme();
    }
  }
}, false);
