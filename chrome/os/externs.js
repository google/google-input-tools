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


/**
 * HTML5 localStorage extern.
 */
var localStorage;


/**
 * Gets item.
 *
 * @param {string} key The key in local storage.
 */
localStorage.getItem = function(key) {};


/**
 * Sets item.
 *
 * @param {string} key The key in local storage.
 * @param {string} value The value in local storage.
 */
localStorage.setItem = function(key, value) {};


/**
 * Removes item.
 *
 * @param {string} key The key in local storage.
 */
localStorage.removeItem = function(key) {};


/**
 * Posts a message to a PPAPI plugin.
 * @param {string} message The message.
 */
Element.prototype.postMessage = function(message) {};


/**
 * chrome.input.ime.onSurroundingTextChanged is introduced in Chrome 27 and
 * it is not defined in google3/javascript/externs/chrome_extensions.js yet.
 * TODO(horo) Move it to chrome_extensions.js when Chrome 27 become stable.
 * @type {!ChromeEvent}
 */
chrome.input.ime.onSurroundingTextChanged;


/**
 * chrome.input.ime.onReset is introduced in Chrome 29 and it is not defined in
 * google3/javascript/externs/chrome_extensions.js yet.
 * TODO(horo) Move it to chrome_extensions.js when Chrome 29 become stable.
 * @type {!ChromeEvent}
 */
chrome.input.ime.onReset;


/**
 * @param {!Object} parameters
 * @param {function(boolean): void=} opt_callback Callback function.
 */
chrome.input.ime.sendKeyEvents = function(parameters, opt_callback) {};


/** Hides the keyboard */
chrome.input.ime.hideInputView = function() {};


/**
 * namespace
 * @const
 */
var mozc = {};

/**
 * The following types are subsets of Protobuf defined in commands.proto.
 */



/** @constructor */
mozc.UserDictionaryEntry = function() {};


/** @type {string|undefined} */
mozc.UserDictionaryEntry.prototype.key;


/** @type {string|undefined} */
mozc.UserDictionaryEntry.prototype.value;


/** @type {string|undefined} */
mozc.UserDictionaryEntry.prototype.comment;


/** @type {string|undefined} */
mozc.UserDictionaryEntry.prototype.pos;



/** @constructor */
mozc.UserDictionary = function() {};


/** @type {string|undefined} */
mozc.UserDictionary.prototype.id;


/** @type {boolean|undefined} */
mozc.UserDictionary.prototype.enabled;


/** @type {string|undefined} */
mozc.UserDictionary.prototype.name;


/** @type {!Array<!mozc.UserDictionaryEntry>} */
mozc.UserDictionary.prototype.entries;



/** @constructor */
mozc.UserDictionaryStorage = function() {};


/** @type {number|undefined} */
mozc.UserDictionaryStorage.prototype.version;


/** @type {!Array<!mozc.UserDictionary>} */
mozc.UserDictionaryStorage.prototype.dictionaries;



/** @constructor */
mozc.UserDictionaryCommand = function() {};


/** @type {string} */
mozc.UserDictionaryCommand.prototype.type;


/** @type {string|undefined} */
mozc.UserDictionaryCommand.prototype.session_id;


/** @type {string|undefined} */
mozc.UserDictionaryCommand.prototype.dictionary_id;


/** @type {string|undefined} */
mozc.UserDictionaryCommand.prototype.dictionary_name;


/** @type {!Array<number>} */
mozc.UserDictionaryCommand.prototype.entry_index;


/** @type {!mozc.UserDictionaryEntry|undefined} */
mozc.UserDictionaryCommand.prototype.entry;


/** @type {string|undefined} */
mozc.UserDictionaryCommand.prototype.data;


/** @type {boolean|undefined} */
mozc.UserDictionaryCommand.prototype.ensure_non_empty_storage;



/** @constructor */
mozc.UserDictionaryCommandStatus = function() {};


/** @type {string} */
mozc.UserDictionaryCommandStatus.prototype.status;


/** @type {string|undefined} */
mozc.UserDictionaryCommandStatus.prototype.session_id;


/** @type {!mozc.UserDictionaryStorage|undefined} */
mozc.UserDictionaryCommandStatus.prototype.storage;


/** @type {!Array<!mozc.UserDictionaryEntry>|undefined} */
mozc.UserDictionaryCommandStatus.prototype.entries;


/** @type {string|undefined} */
mozc.UserDictionaryCommandStatus.prototype.dictionary_id;


/** @type {number|undefined} */
mozc.UserDictionaryCommandStatus.prototype.entry_size;



/** @constructor */
mozc.KeyEvent = function() {};


/** @type {number|undefined} */
mozc.KeyEvent.prototype.key_code;


/** @type {string|undefined} */
mozc.KeyEvent.prototype.special_key;


/** @type {!Array<string>} */
mozc.KeyEvent.prototype.modifier_keys;


/** @type {string|undefined} */
mozc.KeyEvent.prototype.key_string;



/** @constructor */
mozc.SessionCommand = function() {};


/** @type {string} */
mozc.SessionCommand.prototype.type;


/** @type {number|undefined} */
mozc.SessionCommand.prototype.id;


/** @type {string|undefined} */
mozc.SessionCommand.prototype.composition_mode;


/** @type {string|undefined} */
mozc.SessionCommand.prototype.text;



/** @constructor */
mozc.Context = function() {};


/** @type {string|undefined} */
mozc.Context.prototype.preceding_text;


/** @type {string|undefined} */
mozc.Context.prototype.following_text;


/** @type {boolean|undefined} */
mozc.Context.prototype.suppress_suggestion;


/** @type {string|undefined} */
mozc.Context.prototype.input_field_type;



/** @constructor */
mozc.Capability = function() {};


/** @type {string|undefined} */
mozc.Capability.prototype.text_deletion;



/** @constructor */
mozc.ApplicationInfo = function() {};


/** @type {number|undefined} */
mozc.ApplicationInfo.prototype.timezone_offset;



/** @constructor */
mozc.AuthorizationInfo = function() {};


/** @type {string|undefined} */
mozc.AuthorizationInfo.prototype.access_token;



/** @constructor */
mozc.Input = function() {};


/** @type {string} */
mozc.Input.prototype.type;


/** @type {string|undefined} */
mozc.Input.prototype.id;


/** @type {!mozc.KeyEvent|undefined} */
mozc.Input.prototype.key;


/** @type {!mozc.SessionCommand|undefined} */
mozc.Input.prototype.command;


/** @type {!Object<string,*>|undefined} */
mozc.Input.prototype.config;


/** @type {!mozc.Context|undefined} */
mozc.Input.prototype.context;


/** @type {!mozc.Capability|undefined} */
mozc.Input.prototype.capability;


/** @type {!mozc.ApplicationInfo|undefined} */
mozc.Input.prototype.application_info;


/** @type {!mozc.AuthorizationInfo|undefined} */
mozc.Input.prototype.auth_code;


/** @type {!mozc.UserDictionaryCommand|undefined} */
mozc.Input.prototype.user_dictionary_command;



/** @constructor */
mozc.Result = function() {};


/** @type {string} */
mozc.Result.prototype.type;


/** @type {string} */
mozc.Result.prototype.value;


/** @type {string|undefined} */
mozc.Result.prototype.key;


/** @type {number|undefined} */
mozc.Result.prototype.cursor_offset;



/** @constructor */
mozc.Segment = function() {};


/** @type {string} */
mozc.Segment.prototype.annotation;


/** @type {string} */
mozc.Segment.prototype.value;


/** @type {number} */
mozc.Segment.prototype.value_length;


/** @type {string|undefined} */
mozc.Segment.prototype.key;



/** @constructor */
mozc.Preedit = function() {};


/** @type {number} */
mozc.Preedit.prototype.cursor;


/** @type {!Array<!mozc.Segment>} */
mozc.Preedit.prototype.segment;


/** @type {number|undefined} */
mozc.Preedit.prototype.highlighted_position;



/** @constructor */
mozc.Status = function() {};


/** @type {boolean|undefined} */
mozc.Status.prototype.activated;


/** @type {string|undefined} */
mozc.Status.prototype.mode;



/** @constructor */
mozc.DeletionRange = function() {};


/** @type {number|undefined} */
mozc.DeletionRange.prototype.offset;


/** @type {number|undefined} */
mozc.DeletionRange.prototype.length;



/** @constructor */
mozc.Callback = function() {};


/** @type {!mozc.SessionCommand|undefined} */
mozc.Callback.prototype.session_command;


/** @type {number|undefined} */
mozc.Callback.prototype.delay_millisec;



/** @constructor */
mozc.Annotation = function() {};


/** @type {string|undefined} */
mozc.Annotation.prototype.prefix;


/** @type {string|undefined} */
mozc.Annotation.prototype.suffix;


/** @type {string|undefined} */
mozc.Annotation.prototype.description;


/** @type {string|undefined} */
mozc.Annotation.prototype.shortcut;



/** @constructor */
mozc.CandidateWord = function() {};


/** @type {number} */
mozc.CandidateWord.prototype.id;


/** @type {number} */
mozc.CandidateWord.prototype.index;


/** @type {string} */
mozc.CandidateWord.prototype.key;


/** @type {string} */
mozc.CandidateWord.prototype.value;



/** @constructor */
mozc.CandidateList = function() {};


/** @type {!Array.<!mozc.CandidateWord>} */
mozc.CandidateList.prototype.candidates;



/** @constructor */
mozc.Candidate = function() {};


/** @type {number} */
mozc.Candidate.prototype.index;


/** @type {string} */
mozc.Candidate.prototype.value;


/** @type {number|undefined} */
mozc.Candidate.prototype.id;


/** @type {!mozc.Annotation|undefined} */
mozc.Candidate.prototype.annotation;


/** @type {number|undefined} */
mozc.Candidate.prototype.information_id;



/** @constructor */
mozc.Information = function() {};


/** @type {number|undefined} */
mozc.Information.prototype.id;


/** @type {string|undefined} */
mozc.Information.prototype.title;


/** @type {string|undefined} */
mozc.Information.prototype.description;


/** @type {!Array<number>} */
mozc.Information.prototype.candidate_id;



/** @constructor */
mozc.InformationList = function() {};


/** @type {number|undefined} */
mozc.InformationList.prototype.focused_index;


/** @type {!Array<!mozc.Information>} */
mozc.InformationList.prototype.information;


/** @type {string|undefined} */
mozc.InformationList.prototype.category;


/** @type {string|undefined} */
mozc.InformationList.prototype.display_type;


/** @type {number|undefined} */
mozc.InformationList.prototype.delay;



/** @constructor */
mozc.Footer = function() {};


/** @type {string|undefined} */
mozc.Footer.prototype.label;


/** @type {boolean|undefined} */
mozc.Footer.prototype.index_visible;


/** @type {boolean|undefined} */
mozc.Footer.prototype.logo_visible;


/** @type {string|undefined} */
mozc.Footer.prototype.sub_label;



/** @constructor */
mozc.Candidates = function() {};


/** @type {number|undefined} */
mozc.Candidates.prototype.focused_index;


/** @type {number} */
mozc.Candidates.prototype.size;


/** @type {!Array<!mozc.Candidate>} */
mozc.Candidates.prototype.candidate;


/** @type {number} */
mozc.Candidates.prototype.position;


/** @type {!Array<!mozc.Candidate>} */
mozc.Candidates.prototype.subcandidates;


/** @type {!mozc.InformationList|undefined} */
mozc.Candidates.prototype.usages;


/** @type {string|undefined} */
mozc.Candidates.prototype.category;


/** @type {string|undefined} */
mozc.Candidates.prototype.display_type;


/** @type {!mozc.Footer|undefined} */
mozc.Candidates.prototype.footer;


/** @type {string|undefined} */
mozc.Candidates.prototype.direction;



/** @constructor */
mozc.Output = function() {};


/** @type {string|undefined} */
mozc.Output.prototype.id;


/** @type {string|undefined} */
mozc.Output.prototype.mode;


/** @type {boolean|undefined} */
mozc.Output.prototype.consumed;


/** @type {!mozc.Result|undefined} */
mozc.Output.prototype.result;


/** @type {!mozc.Preedit|undefined} */
mozc.Output.prototype.preedit;


/** @type {!mozc.Candidates|undefined} */
mozc.Output.prototype.candidates;


/** @type {!mozc.CandidateList|undefined} */
mozc.Output.prototype.all_candidate_words;


/** @type {!mozc.KeyEvent|undefined} */
mozc.Output.prototype.key;


/** @type {string|undefined} */
mozc.Output.prototype.url;


/** @type {!Object<string,*>|undefined} */
mozc.Output.prototype.config;


/** @type {string|undefined} */
mozc.Output.prototype.preedit_method;


/** @type {string|undefined} */
mozc.Output.prototype.error_code;


/** @type {!mozc.Status|undefined} */
mozc.Output.prototype.status;


/** @type {!mozc.DeletionRange|undefined} */
mozc.Output.prototype.deletion_range;


/** @type {!mozc.Callback|undefined} */
mozc.Output.prototype.callback;


/** @type {!mozc.UserDictionaryCommandStatus|undefined} */
mozc.Output.prototype.user_dictionary_command_status;


/** @constructor */
mozc.Command = function() {};


/** @type {!mozc.Input} */
mozc.Command.prototype.input;


/** @type {!mozc.Output} */
mozc.Command.prototype.output;


/**
 * The following types are used in the communication between NaCl module and
 * JavaScript module.
 */



/** @constructor */
mozc.PosType = function() {};


/** @type {string} */
mozc.PosType.prototype.type;


/** @type {string} */
mozc.PosType.prototype.name;



/** @constructor */
mozc.Event = function() {};


/** @type {string|undefined} */
mozc.Event.prototype.type;


/** @type {string|undefined} */
mozc.Event.prototype.name;


/** @type {string|undefined} */
mozc.Event.prototype.data;


/** @type {string|undefined} */
mozc.Event.prototype.big_dictionary_version;


/** @type {number|undefined} */
mozc.Event.prototype.big_dictionary_state;


/** @type {!Object<string,*>|undefined} */
mozc.Event.prototype.config;


/** @type {boolean|undefined} */
mozc.Event.prototype.result;


/** @type {string|undefined} */
mozc.Event.prototype.error;


/** @type {!Array<!mozc.PosType>|undefined} */
mozc.Event.prototype.posList;



/** @constructor */
mozc.Message = function() {};


/** @type {!mozc.Command|undefined} */
mozc.Message.prototype.cmd;


/** @type {!mozc.Event|undefined} */
mozc.Message.prototype.event;


/** @type {!Object|undefined} */
mozc.Message.prototype.args;


/** @type {!number|undefined} */
mozc.Message.prototype.id;


/** @const */
chrome.inputMethodPrivate = {};


/**
 * @param {function(!Array<!Object>): void} callback .
 */
chrome.inputMethodPrivate.getInputMethods = function(callback) {};


/**
 * @param {function(string)} callback .
 */
chrome.inputMethodPrivate.getCurrentInputMethod = function(callback) {};


/**
 * @param {function(!Object)} callback .
 */
chrome.inputMethodPrivate.getInputMethodConfig = function(callback) {};


/**
 * @param {!string} inputMethodId .
 * @param {function()=} opt_callback .
 */
chrome.inputMethodPrivate.setCurrentInputMethod =
    function(inputMethodId, opt_callback) {};


/**
 * @param {function(!Array<string>)} callback .
 */
chrome.inputMethodPrivate.fetchAllDictionaryWords = function(callback) {};


/**
 * @param {string} word
 * @param {function()} callback .
 */
chrome.inputMethodPrivate.addWordToDictionary = function(word, callback) {};


/** @const */
chrome.inputMethodPrivate.onCompositionBoundsChanged = {};


/**
 * @param {function(!BoundSize, !Array<!BoundSize>): (boolean|undefined)}
 *     callback .
 */
chrome.inputMethodPrivate.onCompositionBoundsChanged.addListener =
    function(callback) {};


/**
 * @param {function(!BoundSize, !Array<!BoundSize>): (boolean|undefined)}
 *     callback .
 */
chrome.inputMethodPrivate.onCompositionBoundsChanged.removeListener =
    function(callback) {};


/** @const */
chrome.inputMethodPrivate.onDictionaryLoaded = {};


/**
 * @param {function()} callback .
 */
chrome.inputMethodPrivate.onDictionaryLoaded.addListener =
    function(callback) {};


/**
 * @param {function()} callback .
 */
chrome.inputMethodPrivate.onDictionaryLoaded.removeListener =
    function(callback) {};


/** @const */
chrome.inputMethodPrivate.onDictionaryChanged = {};


/**
 * @param {function(!Array<string>, !Array<string>)} callback .
 */
chrome.inputMethodPrivate.onDictionaryChanged.addListener =
    function(callback) {};


/**
 * @param {function(!Array<string>, !Array<string>)} callback .
 */
chrome.inputMethodPrivate.onDictionaryChanged.removeListener =
    function(callback) {};


/** @const */
chrome.virtualKeyboardPrivate = {};


/**
 * @param {function(!Object)} callback .
 */
chrome.virtualKeyboardPrivate.getKeyboardConfig = function(callback) {};


/** Opens the language settings page in ChromeOS. */
chrome.virtualKeyboardPrivate.openSettings = function() {};


/**
 * @param {!Object} parameters
 * @param {function(): void=} opt_callback Callback function.
 */
chrome.virtualKeyboardPrivate.sendKeyEvent =
    function(parameters, opt_callback) {};


/** @type {chrome.app.window.Bounds} */
chrome.app.window.AppWindow.prototype.outerBounds;


/**
 * Sets the position.
 *
 * @param {number} x The x position.
 * @param {number} y The y position.
 */
chrome.app.window.Bounds.prototype.setPosition = function(x, y) {};


/**
 * Sets the size.
 *
 * @param {number} w The width.
 * @param {number} h The height.
 */
chrome.app.window.Bounds.prototype.setSize = function(w, h) {};



/**
 * Callback parameter which bring by composition bounds changed event.
 *
 * @constructor
 */
var BoundSize = function() {};


/** @type {number} */
BoundSize.prototype.x;


/** @type {number} */
BoundSize.prototype.y;


/** @type {number} */
BoundSize.prototype.w;


/** @type {number} */
BoundSize.prototype.h;


/** @type {boolean} */
InputContext.prototype.autoCorrect;


/** @type {boolean} */
InputContext.prototype.autoComplete;


/** @type {boolean} */
InputContext.prototype.spellCheck;
