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
// Copyright 2013 The ChromeOS IME Authors. All Rights Reserved.
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

/**
 * @fileoverview Controller for os decoder chrome os extension.
 */

goog.provide('goog.ime.chrome.os.Controller');

goog.require('goog.Disposable');
goog.require('goog.events.EventHandler');
goog.require('goog.ime.chrome.os.ConfigFactory');
goog.require('goog.ime.chrome.os.EventType');
goog.require('goog.ime.chrome.os.Key');
goog.require('goog.ime.chrome.os.KeyboardLayouts');
goog.require('goog.ime.chrome.os.Model');
goog.require('goog.ime.chrome.os.Modifier');
goog.require('goog.ime.chrome.os.StateID');
goog.require('goog.ime.chrome.os.Status');
goog.require('goog.ime.chrome.os.View');



/**
 * The controller for os chrome os extension.
 *
 * @constructor
 * @extends {goog.Disposable}
 */
goog.ime.chrome.os.Controller = function() {
  /**
   * The model.
   *
   * @type {!goog.ime.chrome.os.Model}
   * @protected
   */
  this.model = new goog.ime.chrome.os.Model();

  /**
   * The view.
   *
   * @type {!goog.ime.chrome.os.View}
   * @protected
   */
  this.view = new goog.ime.chrome.os.View(this.model);

  /**
   * The config factory
   *
   * @type {goog.ime.chrome.os.ConfigFactory}
   * @protected
   */
  this.configFactory = goog.ime.chrome.os.ConfigFactory.getInstance();

  /**
   * The event handler.
   *
   * @type {!goog.events.EventHandler}
   * @private
   */
  this.eventHandler_ = new goog.events.EventHandler(this);
  this.eventHandler_.
      listen(this.model, goog.ime.chrome.os.EventType.COMMIT,
          this.handleCommitEvent).
      listen(this.model, goog.ime.chrome.os.EventType.OPENING,
          this.handleOpeningEvent).
      listen(this.model, goog.ime.chrome.os.EventType.CLOSING,
          this.handleClosingEvent).
      listen(this.model, goog.ime.chrome.os.EventType.MODELUPDATED,
          this.handleModelUpdatedEvent);
};
goog.inherits(goog.ime.chrome.os.Controller, goog.Disposable);


/**
 * The current context.
 *
 * @type {Object}
 * @private
 */
goog.ime.chrome.os.Controller.prototype.context_ = null;


/**
 * The key action table.
 *
 * @type {Array.<!Array>}
 * @private
 */
goog.ime.chrome.os.Controller.prototype.keyActionTable_ = null;


/**
 * The shortcut table.
 *
 * @type {Array.<!Array>}
 * @private
 */
goog.ime.chrome.os.Controller.prototype.shortcutTable_ = null;


/**
 * True if the last key down is shift (with not modifiers).
 *
 * @type {boolean}
 * @private
 */
goog.ime.chrome.os.Controller.prototype.lastKeyDownIsShift_ = false;


/**
 * Activates an input tool.
 *
 * @param {!string} inputToolCode The input tool.
 */
goog.ime.chrome.os.Controller.prototype.activate = function(inputToolCode) {
  this.configFactory.setInputTool(inputToolCode);
  this.model.setInputTool(inputToolCode);
  this.view.updateInputTool();
  this.keyActionTable_ = this.getKeyActionTable();
  this.shortcutTable_ = this.getShortcutTable();
};


/**
 * Deactivates the input tool.
 */
goog.ime.chrome.os.Controller.prototype.deactivate = function() {
  this.configFactory.setInputTool('');
  this.model.reset();
  this.view.updateInputTool();
  this.keyActionTable_ = null;
  this.shortcutTable_ = null;
};


/**
 * Register a context.
 *
 * @param {Object} context The context.
 */
goog.ime.chrome.os.Controller.prototype.register = function(context) {
  this.context_ = context;
  this.view.setContext(context);
  this.model.clear();
};


/**
 * Unregister the context.
 */
goog.ime.chrome.os.Controller.prototype.unregister = function() {
  this.context_ = null;
  this.view.setContext(null);
  this.model.clear();
};


/**
 * Resets the context.
 */
goog.ime.chrome.os.Controller.prototype.reset = function() {
  this.model.abort();
};


/**
 * Sets the fuzzy expansions for a given input tool.
 *
 * @param {string} inputToolCode The input tool code.
 * @param {boolean} enableFuzzy True if fuzzy pinyin is enabled.
 * @param {Object.<string, boolean>} fuzzyExpansions The fuzzy expansion map.
 */
goog.ime.chrome.os.Controller.prototype.setFuzzyExpansions = function(
    inputToolCode, enableFuzzy, fuzzyExpansions) {
  var enabledFuzzyExpansions = [];
  if (enableFuzzy) {
    for (var fuzzyPair in fuzzyExpansions) {
      if (fuzzyExpansions[fuzzyPair]) {
        enabledFuzzyExpansions.push(fuzzyPair);
      }
    }
  }
  this.model.setFuzzyExpansions(inputToolCode, enabledFuzzyExpansions);
};


/**
 * Enables/Disables user dictionary for a given input tool.
 *
 * @param {string} inputToolCode The input tool code.
 * @param {boolean} enableUserDict True if user dictionary is enabled.
 */
goog.ime.chrome.os.Controller.prototype.setUserDictEnabled = function(
    inputToolCode, enableUserDict) {
  this.model.enableUserDict(inputToolCode, enableUserDict);
};


/**
 * Sets the pageup/pagedown characters.
 *
 * @param {string} inputToolCode The input tool code.
 * @param {string} pageupChars The characters for page up.
 * @param {string} pagedownChars The characters for page down.
 */
goog.ime.chrome.os.Controller.prototype.setPageMoveChars = function(
    inputToolCode, pageupChars, pagedownChars) {
  var config = this.configFactory.getConfig(inputToolCode);
  if (config) {
    if (pageupChars) {
      config.pageupCharReg = new RegExp('[' + pageupChars + ']');
    } else {
      config.pageupCharReg = /xyz/g;
    }

    if (pagedownChars) {
      config.pagedownCharReg = new RegExp('[' + pagedownChars + ']');
    } else {
      config.pagedownCharReg = /xyz/g;
    }
    this.keyActionTable_ = this.getKeyActionTable();
  }
};


/**
 * Sets the inital language, sbc and puncutation modes.
 *
 * @param {string} inputToolCode The input tool code.
 * @param {boolean} initLang True, if the initial language is Chinese.
 * @param {boolean} initSBC True, if the initial character width is wide.
 * @param {boolean} initPunc True, if the initial punctuation is Chinese.
 */
goog.ime.chrome.os.Controller.prototype.setInputToolStates = function(
    inputToolCode, initLang, initSBC, initPunc) {
  var config = this.configFactory.getConfig(inputToolCode);
  var stateID = goog.ime.chrome.os.StateID;
  if (config) {
    config.states[stateID.LANG].value = initLang;
    config.states[stateID.SBC].value = initSBC;
    config.states[stateID.PUNC].value = initPunc;
  }
};


/**
 * Sets the keyboard layout, select keys and page size.
 *
 * @param {string} inputToolCode The input tool code.
 * @param {string} layout The keyboard layout id.
 * @param {string} selectKeys The select keys.
 * @param {number} pageSize The page size.
 */
goog.ime.chrome.os.Controller.prototype.setPageSettings = function(
    inputToolCode, layout, selectKeys, pageSize) {
  var config = this.configFactory.getConfig(inputToolCode);
  var stateID = goog.ime.chrome.os.StateID;
  var keyboardLayouts = goog.ime.chrome.os.KeyboardLayouts;
  if (config) {
    config.layout = layout;
    config.selectKeys = selectKeys;
    config.pageSize = pageSize;
    this.keyActionTable_ = this.getKeyActionTable();
  }
};


/**
 * Switch the input tool state.
 *
 * @param {string} stateId The state ID.
 */
goog.ime.chrome.os.Controller.prototype.switchInputToolState = function(
    stateId) {
  var config = this.configFactory.getCurrentConfig();
  config.states[stateId].value = !config.states[stateId].value;
  var stateID = goog.ime.chrome.os.StateID;
  if (stateId == stateID.LANG) {
    config.states[stateID.PUNC].value = config.states[stateID.LANG].value;
  }
  this.model.clear();
  this.view.updateItems();
  this.view.hide();
};


/**
 * Handles key event.
 *
 * @param {!ChromeKeyboardEvent} e the key event.
 * @return {boolean} True if the event is handled successfully.
 */
goog.ime.chrome.os.Controller.prototype.handleEvent = function(e) {
  var inputTool = this.configFactory.getInputTool();
  if (!this.context_ || !inputTool || !this.keyActionTable_ ||
      !this.model.ready) {
    return false;
  }


  if (this.shortcutTable_ &&
      this.handleKeyInActionTable_(e, this.shortcutTable_)) {
    return true;
  }

  if (this.preProcess(e)) {
    return true;
  }

  var config = this.configFactory.getCurrentConfig();
  var langStateID = goog.ime.chrome.os.StateID.LANG;
  if (config.states[langStateID] && !config.states[langStateID].value) {
    return false;
  }

  if (this.handleKeyInActionTable_(e, this.keyActionTable_)) {
    return true;
  }

  if (e.type == goog.ime.chrome.os.EventType.KEYDOWN &&
      e.key != goog.ime.chrome.os.Key.INVALID &&
      e.key != goog.ime.chrome.os.Modifier.SHIFT &&
      e.key != goog.ime.chrome.os.Modifier.CTRL &&
      e.key != goog.ime.chrome.os.Modifier.ALT &&
      this.model.status != goog.ime.chrome.os.Status.INIT) {
    this.model.selectCandidate(-1, '');
  }
  return false;
};


/**
 * Pre-processes the key press event.
 *
 * @param {!ChromeKeyboardEvent} e the key event.
 * @return {boolean} Whether the key event is processed. If returns false,
 *     controller should continue processing the event.
 */
goog.ime.chrome.os.Controller.prototype.preProcess = function(e) {
  var Key = goog.ime.chrome.os.Key;
  if (e.type != goog.ime.chrome.os.EventType.KEYDOWN ||
      this.model.status != goog.ime.chrome.os.Status.INIT ||
      e.ctrlKey || e.altKey) {
    return false;
  }

  var config = this.configFactory.getCurrentConfig();
  var trans = config.preTransform(e.key);
  if (trans) {
    if (this.context_) {
      chrome.input.ime.commitText({
        'contextID': this.context_.contextID,
        'text': trans});
    }
    return true;
  }
  return false;
};


/**
 * Processes the char key.
 *
 * @param {Event} e The key event.
 * @return {boolean} Whether the key event is processed.
 */
goog.ime.chrome.os.Controller.prototype.processCharKey = function(e) {
  var text = this.configFactory.getCurrentConfig().transform(
      this.model.source, e.key);
  if (!text) {
    return this.model.status != goog.ime.chrome.os.Status.INIT;
  }
  this.model.updateSource(text);
  return true;
};


/**
 * Handles keyevent in a key action table.
 *
 * @param {!ChromeKeyboardEvent} e The key event.
 * @param {!Array.<!Array>} table The key action table.
 * @return {boolean} True if the key is handled.
 * @private
 */
goog.ime.chrome.os.Controller.prototype.handleKeyInActionTable_ = function(
    e, table) {
  var Key = goog.ime.chrome.os.Key;
  var Modifier = goog.ime.chrome.os.Modifier;
  var key = this.getKey_(e);

  for (var i = 0, item; item = table[i]; i++) {
    // Each item of the key action table is an array with this format:
    // [EventType, Modifier, KeyCode/KeyChar, ModelStatus, MoreConditionFunc,
    //    ActionFunc, ActionFuncScopeObj, args].
    if (e.type != item[0]) {
      continue;
    }
    var modifier = item[1];
    if (modifier == Modifier.SHIFT && !e.shiftKey ||
        modifier == Modifier.CTRL && !e.ctrlKey ||
        modifier == Modifier.ALT && !e.altKey) {
      continue;
    }
    if (modifier == 0) {
      if (e.ctrlKey || e.altKey || e.shiftKey) {
        continue;
      }
    }
    if (item[2] && key != item[2] &&
        (key.length != 1 || !key.match(item[2]))) {
      continue;
    }
    if (item[3] && this.model.status != item[3]) {
      continue;
    }
    if (!item[4] || item[4]()) {
      if (item[5].apply(
          item[6], item[7] != undefined ? item.slice(7) : [e]) != false) {
        return true;
      }
    }
  }
  return false;
};


/**
 * Processes the number key.
 *
 * @param {Object} e The key event.
 * @return {boolean} Whether the key event is processed.
 */
goog.ime.chrome.os.Controller.prototype.processNumberKey = function(e) {
  var selectKeys = this.configFactory.getCurrentConfig().selectKeys;
  var pageOffset = selectKeys.indexOf(e.key);
  if (pageOffset < 0) {
    return true;
  }
  var pageSize = this.configFactory.getCurrentConfig().pageSize;
  if (pageOffset >= 0 && pageOffset < pageSize) {
    var index = this.model.getPageIndex() * pageSize + pageOffset;
    this.model.selectCandidate(index);
  }
  return true;
};


/**
 * Processes the punch key.
 *
 * @param {Event} e The key event.
 * @return {boolean} Whether the key event is processed.
 */
goog.ime.chrome.os.Controller.prototype.processPuncKey = function(e) {
  var config = this.configFactory.getCurrentConfig();
  var punc = config.postTransform(e.key);
  this.model.selectCandidate(undefined, punc);
  return true;
};


/**
 * Processes the revert key.
 *
 * @param {!goog.events.BrowserEvent} e The key event.
 * @return {boolean} Whether the key event is processed.
 */
goog.ime.chrome.os.Controller.prototype.processRevertKey = function(e) {
  this.model.revert();
  return true;
};


/**
 * Processes the commit key.
 *
 * @param {Event} e The key event.
 * @return {boolean} Whether the key event is processed.
 */
goog.ime.chrome.os.Controller.prototype.processCommitKey = function(e) {
  if (e.key == goog.ime.chrome.os.Key.ENTER &&
      this.model.status == goog.ime.chrome.os.Status.SELECT) {
    this.model.selectCandidate(-1, '');
  } else {
    this.model.selectCandidate(undefined);
  }
  return true;
};


/**
 * Process the key to enter model select status.
 *
 * @param {Event} e The key event.
 * @return {boolean} Whether the key event is processed.
 */
goog.ime.chrome.os.Controller.prototype.processSelectKey = function(e) {
  if (this.model.status == goog.ime.chrome.os.Status.FETCHED) {
    this.model.enterSelect();
  }
  return true;
};


/**
 * Processes the abort key.
 *
 * @param {Event} e The key event.
 * @return {boolean} Whether the key event is processed.
 */
goog.ime.chrome.os.Controller.prototype.processAbortKey = function(e) {
  this.model.abort();
  return true;
};


/**
 * Handles the model opening event.
 *
 * @protected
 */
goog.ime.chrome.os.Controller.prototype.handleOpeningEvent = function() {
  this.view.show();
};


/**
 * Handles the model closing event.
 *
 * @protected
 */
goog.ime.chrome.os.Controller.prototype.handleClosingEvent = function() {
  this.view.hide();
};


/**
 * Handles the model updated event.
 *
 * @protected
 */
goog.ime.chrome.os.Controller.prototype.handleModelUpdatedEvent =
    function() {
  this.view.refresh();
};


/**
 * Handles the commit event.
 *
 * @param {!Event} e The commit event.
 * @protected
 */
goog.ime.chrome.os.Controller.prototype.handleCommitEvent = function(e) {
  if (this.context_) {
    var config = this.configFactory.getCurrentConfig();
    chrome.input.ime.commitText({
      'contextID': this.context_.contextID,
      'text': this.model.segments.join('')});
  }
};


/**
 * Gets the key action table
 *
 * @return {!Array.<!Array>} The key action table.
 * @protected
 */
goog.ime.chrome.os.Controller.prototype.getKeyActionTable = function() {
  var Type = goog.ime.chrome.os.EventType;
  var Key = goog.ime.chrome.os.Key;
  var Modifier = goog.ime.chrome.os.Modifier;
  var Status = goog.ime.chrome.os.Status;
  var config = this.configFactory.getCurrentConfig();
  var self = this;

  var onStageCondition = function() {
    return self.model.status != goog.ime.chrome.os.Status.INIT;
  };

  var onStageNotSelectableCondition = function() {
    return self.model.status != goog.ime.chrome.os.Status.INIT &&
        self.model.status != goog.ime.chrome.os.Status.SELECT;
  };

  var hasCandidatesCondition = function() {
    return self.model.status == goog.ime.chrome.os.Status.FETCHED ||
        self.model.status == goog.ime.chrome.os.Status.SELECT;
  };

  var selectReg = new RegExp('[' + config.selectKeys + ']');

  // [EventType, Modifier, KeyCode/KeyChar, ModelStatus, MoreConditionFunc,
  //  ActionFunc, ActionFuncScopeObj, args]
  return [
    [Type.KEYDOWN, 0, Key.PAGE_UP, null, onStageCondition,
     this.model.movePage, this.model, 1],
    [Type.KEYDOWN, 0, config.pageupCharReg, Status.SELECT, null,
     this.model.movePage, this.model, 1],
    [Type.KEYDOWN, 0, Key.PAGE_DOWN, null, onStageCondition,
     this.model.movePage, this.model, -1],
    [Type.KEYDOWN, 0, config.pagedownCharReg, Status.SELECT, null,
     this.model.movePage, this.model, -1],
    [Type.KEYDOWN, 0, selectReg, Status.SELECT, null,
     this.processNumberKey, this],
    [Type.KEYDOWN, 0, Key.SPACE, null, onStageNotSelectableCondition,
     this.processSelectKey, this],
    [Type.KEYDOWN, 0, Key.SPACE, null, onStageCondition,
     this.processCommitKey, this],
    [Type.KEYDOWN, 0, Key.DOWN, null, onStageNotSelectableCondition,
     this.processSelectKey, this],
    [Type.KEYDOWN, 0, Key.ENTER, null, onStageCondition,
     this.processCommitKey, this],
    [Type.KEYDOWN, 0, Key.BACKSPACE, null, hasCandidatesCondition,
     this.processRevertKey, this],
    [Type.KEYDOWN, 0, Key.UP, null, onStageCondition,
     this.model.moveHighlight, this.model, -1],
    [Type.KEYDOWN, 0, Key.DOWN, null, onStageCondition,
     this.model.moveHighlight, this.model, 1],
    [Type.KEYDOWN, 0, Key.LEFT, null, onStageCondition,
     this.model.moveCursorLeft, this.model],
    [Type.KEYDOWN, 0, Key.RIGHT, null, onStageCondition,
     this.model.moveCursorRight, this.model],
    [Type.KEYDOWN, 0, Key.ESC, null, onStageCondition,
     this.processAbortKey, this],
    [Type.KEYDOWN, 0, config.editorCharReg, null, null,
     this.processCharKey, this],
    [Type.KEYDOWN, Modifier.SHIFT, config.editorCharReg, null, null,
     this.processCharKey, this],
    [Type.KEYDOWN, 0, config.punctuationReg, null, onStageCondition,
     this.processPuncKey, this]
  ];
};


/**
 * Gets the shortcut table
 *
 * @return {!Array.<!Array>} The shortcut table.
 * @protected
 */
goog.ime.chrome.os.Controller.prototype.getShortcutTable = function() {
  var shortcutTable = [];
  var Type = goog.ime.chrome.os.EventType;
  var Key = goog.ime.chrome.os.Key;
  var Modifier = goog.ime.chrome.os.Modifier;
  var config = this.configFactory.getCurrentConfig();

  var self = this;
  for (var stateID in config.states) {
    var stateValue = config.states[stateID];
    if (stateValue.shortcut.length == 1 &&
        stateValue.shortcut[0] == Modifier.SHIFT) {
      // To handle SHIFT shortcut.
      shortcutTable.unshift(
          [Type.KEYDOWN, null, null, null, null,
           this.updateLastKeyIsShift_, this]);
      shortcutTable.push(
          [Type.KEYUP, Modifier.SHIFT, Modifier.SHIFT, null,
           function() {
             return self.lastKeyDownIsShift_;
           }, this.switchInputToolState, this, stateID]);
    } else if (stateValue.shortcut.length >= 1) {
      shortcutTable.push(
          [Type.KEYDOWN, stateValue.shortcut[1], stateValue.shortcut[0],
           null, null, this.switchInputToolState, this, stateID]);
    }
  }
  return shortcutTable;
};


/**
 * Updates whether the last keydown is SHIFT.
 *
 * @param {!ChromeKeyboardEvent} e the key event.
 * @return {boolean} It should always return false.
 * @private
 */
goog.ime.chrome.os.Controller.prototype.updateLastKeyIsShift_ = function(e) {
  if (this.getKey_(e) == goog.ime.chrome.os.Modifier.SHIFT &&
      !e.altKey && !e.ctrlKey) {
    this.lastKeyDownIsShift_ = true;
  } else {
    this.lastKeyDownIsShift_ = false;
  }
  return false;
};


/**
 * Gets the key from an key event.
 *
 * @param {!ChromeKeyboardEvent} e the key event.
 * @return {string} The key string.
 * @private
 */
goog.ime.chrome.os.Controller.prototype.getKey_ = function(e) {
  var key = e.key;
  if (key == goog.ime.chrome.os.Key.INVALID) {
    var modifiers = goog.ime.chrome.os.Modifier;
    for (var index in modifiers) {
      var modifier = modifiers[index];
      if (e['code'].indexOf(modifier) == 0) {
        key = modifier;
      }
    }
  }
  return key;
};
