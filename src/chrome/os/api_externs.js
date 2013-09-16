// Copyright 2013 The ChromeOS VK Authors. All Rights Reserved.
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

var chrome = {};


/**
 * @const
 * @see http://code.google.com/chrome/extensions/input.ime.html
 */
chrome.input = {};


/** @const */
chrome.input.ime = {};


/** @const */
chrome.inputMethodPrivate = {};


/**
 * @see http://code.google.com/chrome/extensions/events.html
 * @constructor
 */
function ChromeEvent() {}


/** @param {Function} callback */
ChromeEvent.prototype.addListener = function(callback) {};


/** @param {Function} callback */
ChromeEvent.prototype.removeListener = function(callback) {};


/** @param {Function} callback */
ChromeEvent.prototype.hasListener = function(callback) {};


/** @param {Function} callback */
ChromeEvent.prototype.hasListeners = function(callback) {};



/**
 * @see http://code.google.com/chrome/extensions/input.ime.html#type-KeyboardEvent
 * @constructor
 */
function ChromeKeyboardEvent() {}


/** @type {string} */
ChromeKeyboardEvent.prototype.type;


/** @type {string} */
ChromeKeyboardEvent.prototype.requestId;


/** @type {string} */
ChromeKeyboardEvent.prototype.key;


/** @type {boolean} */
ChromeKeyboardEvent.prototype.altKey;


/** @type {boolean} */
ChromeKeyboardEvent.prototype.ctrlKey;


/** @type {boolean} */
ChromeKeyboardEvent.prototype.shiftKey;



/**
 * The OnKeyEvent event takes an extra argument.
 * @constructor
 */
function ChromeInputImeOnKeyEventEvent() {}


/**
 * @param {function(string, !ChromeKeyboardEvent): (boolean|undefined)} callback
 *     callback.
 * @param {Array.<string>=} opt_extraInfoSpec Array of extra information.
 */
ChromeInputImeOnKeyEventEvent.prototype.addListener =
    function(callback, opt_extraInfoSpec) {};


/**
 * @param {function(string, !ChromeKeyboardEvent): (boolean|undefined)} callback
 *     callback.
 */
ChromeInputImeOnKeyEventEvent.prototype.removeListener = function(callback) {};


/**
 * @param {function(string, !ChromeKeyboardEvent): (boolean|undefined)} callback
 *     callback.
 */
ChromeInputImeOnKeyEventEvent.prototype.hasListener = function(callback) {};


/**
 * @param {function(string, !ChromeKeyboardEvent): (boolean|undefined)} callback
 *     callback.
 */
ChromeInputImeOnKeyEventEvent.prototype.hasListeners = function(callback) {};



/**
 * @see http://code.google.com/chrome/extensions/input.ime.html#type-InputContext
 * @constructor
 */
function InputContext() {}


/** @type {number} */
InputContext.prototype.contextID;


/** @type {string} */
InputContext.prototype.type;


/**
 * @param {!Object.<string,number>} parameters An object with a
 *     'contextID' (number) key.
 * @param {function(boolean): void} callback Callback function.
 */
chrome.input.ime.clearComposition = function(parameters, callback) {};


/**
 * @param {!Object.<string,(string|number)>} parameters An object with
 *     'contextID' (number) and 'text' (string) keys.
 * @param {function(boolean): void=} opt_callback Callback function.
 */
chrome.input.ime.commitText = function(parameters, opt_callback) {};


/**
 * @param {!Object.<string,(number|Object.<string,(string|number|boolean)>)>}
 *     parameters An object with 'engineID' (string) and 'properties'
 *     (Object) keys.
 * @param {function(boolean): void=} opt_callback Callback function.
 */
chrome.input.ime.setCandidateWindowProperties =
    function(parameters, opt_callback) {};


/**
 * @param {!Object.<string,(number|Object.<string,(string|number)>)>}
 *     parameters An object with 'contextID' (number) and 'candidates'
 *     (array of object) keys.
 * @param {function(boolean): void=} opt_callback Callback function.
 */
chrome.input.ime.setCandidates = function(parameters, opt_callback) {};


/**
 * @param {!Object.<string,(string|number|Object.<string,(string|number)>)>}
 *     parameters An object with 'contextID' (number), 'text' (string),
 *     'selectionStart (number), 'selectionEnd' (number), 'cursor' (number),
 *     and 'segments' (array of object) keys.
 * @param {function(boolean): void=} opt_callback Callback function.
 */
chrome.input.ime.setComposition = function(parameters, opt_callback) {};


/**
 * @param {!Object.<string,number>} parameters An object with
 *     'contextID' (number) and 'candidateID' (number) keys.
 * @param {function(boolean): void=} opt_callback Callback function.
 */
chrome.input.ime.setCursorPosition = function(parameters, opt_callback) {};


/**
 * @param {!Object.<string,(string|Array.<Object.<string,(string|boolean)>>)>}
 *     parameters An object with 'engineID' (string) and 'items'
 *     (array of object) keys.
 * @param {function(): void=} opt_callback Callback function.
 */
chrome.input.ime.setMenuItems = function(parameters, opt_callback) {};


/**
 * @param {!Object.<string,(string|Array.<Object.<string,(string|boolean)>>)>}
 *     parameters An object with  'engineID' (string) and 'items'
 *     (array of object) keys.
 * @param {function(): void=} opt_callback Callback function.
 */
chrome.input.ime.updateMenuItems = function(parameters, opt_callback) {};


/**
 * @param {!Object.<string,(string|number)>} parameters An object with
 *     'contextID' (number) and 'text' (string) keys.
 * @param {function(boolean): void=} opt_callback Callback function.
 */
chrome.input.ime.deleteSurroundingText = function(parameters, opt_callback) {};


/**
 * @param {string} requestId Request id of the event that was handled. This
 *     should come from keyEvent.requestId.
 * @param {boolean} response True if the keystroke was handled, false if not.
 */
chrome.input.ime.keyEventHandled = function(requestId, response) {};


/** @type {!ChromeEvent} */
chrome.input.ime.onActivate;


/** @type {!ChromeEvent} */
chrome.input.ime.onBlur;


/** @type {!ChromeEvent} */
chrome.input.ime.onCandidateClicked;


/** @type {!ChromeEvent} */
chrome.input.ime.onDeactivated;


/** @type {!ChromeEvent} */
chrome.input.ime.onFocus;


/** @type {!ChromeEvent} */
chrome.input.ime.onInputContextUpdate;


/** @type {!ChromeInputImeOnKeyEventEvent} */
chrome.input.ime.onKeyEvent;


/** @type {!ChromeEvent} */
chrome.input.ime.onMenuItemActivated;


/** @type {!ChromeEvent} */
chrome.input.ime.onSurroundingTextChanged;


/** @type {!ChromeEvent} */
chrome.input.ime.onReset;


chrome.inputMethodPrivate.startIme = function() {};


/**
 * @see http://code.google.com/chrome/extensions/runtime.html
 * @const
 */
chrome.runtime = {};


/** @type {!Object.<string,string>|undefined} */
chrome.runtime.lastError;


/** @type {string} */
chrome.runtime.id;


/**
 * @param {function(!Window=): void} callback Callback function.
 */
chrome.runtime.getBackgroundPage = function(callback) {};



/**
 * Manifest information returned from chrome.runtime.getManifest. See
 * http://developer.chrome.com/extensions/manifest.html. Note that there are
 * several other fields not included here. They should be added to these externs
 * as needed.
 * @constructor
 */
chrome.runtime.Manifest = function() {};


/** @type {string} */
chrome.runtime.Manifest.prototype.name;


/** @type {string} */
chrome.runtime.Manifest.prototype.version;


/** @type {number|undefined} */
chrome.runtime.Manifest.prototype.manifest_version;


/** @type {string|undefined} */
chrome.runtime.Manifest.prototype.description;


/** @type {!chrome.runtime.Manifest.Oauth2|undefined} */
chrome.runtime.Manifest.prototype.oauth2;



/**
 * Oauth2 info in the manifest.
 * See http://developer.chrome.com/apps/app_identity.html#update_manifest.
 * @constructor
 */
chrome.runtime.Manifest.Oauth2 = function() {};


/** @type {string} */
chrome.runtime.Manifest.Oauth2.prototype.client_id;


/**
 * http://developer.chrome.com/extensions/runtime.html#method-getManifest
 * @return {!chrome.runtime.Manifest} The full manifest file of the app or
 *     extension.
 */
chrome.runtime.getManifest = function() {};


/**
 * @param {string} path A path to a resource within an extension expressed
 *     relative to it's install directory.
 * @return {string} The fully-qualified URL to the resource.
 */
chrome.runtime.getURL = function(path) {};


/**
 * Reloads the app or extension.
 */
chrome.runtime.reload = function() {};


/**
 * @param {function(string, !Object=): void} callback
 */
chrome.runtime.requestUpdateCheck = function(callback) {};


/**
 * @param {string|!Object.<string>=} opt_extensionIdOrConnectInfo Either the
 *     extensionId to connect to, in which case connectInfo params can be
 *     passed in the next optional argument, or the connectInfo params.
 * @param {!Object.<string>=} opt_connectInfo The connectInfo object,
 *     if arg1 was the extensionId to connect to.
 * @return {!Port} New port.
 */
chrome.runtime.connect = function(
    opt_extensionIdOrConnectInfo, opt_connectInfo) {};


/**
 * @param {string|*} extensionIdOrMessage Either the extensionId to send the
 *     message to, in which case the message is passed as the next arg, or the
 *     message itself.
 * @param {(*|function(*): void)=} opt_messageOrCallback The message, if arg1
 *     was the extensionId, or the callback, if arg1 was the message, or
 *     optional.
 * @param {function(*): void=} opt_callback The callback function which
 *     takes a JSON response object sent by the handler of the request.
 */
chrome.runtime.sendMessage = function(
    extensionIdOrMessage, opt_messageOrCallback, opt_callback) {};


/** @type {!chrome.runtime.PortEvent} */
chrome.runtime.onConnect;


/** @type {!chrome.runtime.PortEvent} */
chrome.runtime.onConnectExternal;


/** @type {!chrome.runtime.ObjectEvent} */
chrome.runtime.onInstalled;


/** @type {!chrome.runtime.MessageSenderEvent} */
chrome.runtime.onMessage;


/** @type {!chrome.runtime.MessageSenderEvent} */
chrome.runtime.onMessageExternal;


/** @type {!ChromeEvent} */
chrome.runtime.onStartup;


/** @type {!ChromeEvent} */
chrome.runtime.onSuspend;


/** @type {!ChromeEvent} */
chrome.runtime.onSuspendCanceled;


/** @type {!chrome.runtime.ObjectEvent} */
chrome.runtime.onUpdateAvailable;



/**
 * Event whose listeners take an Object parameter.
 * @constructor
 */
chrome.runtime.ObjectEvent = function() {};


/**
 * @param {function(!Object): void} callback Callback.
 */
chrome.runtime.ObjectEvent.prototype.addListener = function(callback) {};


/**
 * @param {function(!Object): void} callback Callback.
 */
chrome.runtime.ObjectEvent.prototype.removeListener = function(callback) {};


/**
 * @param {function(!Object): void} callback Callback.
 * @return {boolean}
 */
chrome.runtime.ObjectEvent.prototype.hasListener = function(callback) {};


/**
 * @return {boolean}
 */
chrome.runtime.ObjectEvent.prototype.hasListeners = function() {};



/**
 * Event whose listeners take a Port parameter.
 * @constructor
 */
chrome.runtime.PortEvent = function() {};


/**
 * @param {function(!Port): void} callback Callback.
 */
chrome.runtime.PortEvent.prototype.addListener = function(callback) {};


/**
 * @param {function(!Port): void} callback Callback.
 */
chrome.runtime.PortEvent.prototype.removeListener = function(callback) {};


/**
 * @param {function(!Port): void} callback Callback.
 * @return {boolean}
 */
chrome.runtime.PortEvent.prototype.hasListener = function(callback) {};


/**
 * @return {boolean}
 */
chrome.runtime.PortEvent.prototype.hasListeners = function() {};



/**
 * Event whose listeners take a MessageSender and additional parameters.
 * @see https://developer.chrome.com/dev/apps/runtime.html#event-onMessage
 * @constructor
 */
chrome.runtime.MessageSenderEvent = function() {};


/**
 * @param {function(*, !MessageSender, function(*): void): (boolean|undefined)}
 *     callback Callback.
 */
chrome.runtime.MessageSenderEvent.prototype.addListener = function(callback) {};


/**
 * @param {function(*, !MessageSender, function(*): void): (boolean|undefined)}
 *     callback Callback.
 */
chrome.runtime.MessageSenderEvent.prototype.removeListener = function(callback)
    {};


/**
 * @param {function(*, !MessageSender, function(*): void): (boolean|undefined)}
 *     callback Callback.
 * @return {boolean}
 */
chrome.runtime.MessageSenderEvent.prototype.hasListener = function(callback) {};


/**
 * @return {boolean}
 */
chrome.runtime.MessageSenderEvent.prototype.hasListeners = function() {};



/**
 * @see http://code.google.com/chrome/extensions/extension.html#type-Port
 * @constructor
 */
function Port() {}


/** @type {string} */
Port.prototype.name;


/** @type {ChromeEvent} */
Port.prototype.onDisconnect;


/** @type {ChromeEvent} */
Port.prototype.onMessage;


/** @type {MessageSender} */
Port.prototype.sender;


/**
 * @param {Object.<string>} obj Message object.
 */
Port.prototype.postMessage = function(obj) {};


/**
 * Note: as of 2012-04-12, this function is no longer documented on
 * the public web pages, but there are still existing usages.
 */
Port.prototype.disconnect = function() {};



/**
 * @constructor
 */
function MessageSender() {}


/** @type {Tab} */
MessageSender.prototype.tab;


/** @type {string} */
MessageSender.prototype.id;


/**
 * @const
 * @see http://code.google.com/chrome/extensions/windows.html
 */
chrome.windows = {};


/**
 * @param {Object=} opt_createData May have many keys to specify parameters.
 *     Or the callback.
 * @param {function(ChromeWindow): void=} opt_callback Callback.
 */
chrome.windows.create = function(opt_createData, opt_callback) {};


/**
 * @param {number} id Window id.
 * @param {Object=} opt_getInfo May have 'populate' key. Or the callback.
 * @param {function(!ChromeWindow): void=} opt_callback Callback when
 *     opt_getInfo is an object.
 */
chrome.windows.get = function(id, opt_getInfo, opt_callback) {};


/**
 * @param {Object=} opt_getInfo May have 'populate' key. Or the callback.
 * @param {function(!Array.<!ChromeWindow>): void=} opt_callback Callback.
 */
chrome.windows.getAll = function(opt_getInfo, opt_callback) {};


/**
 * @param {Object=} opt_getInfo May have 'populate' key. Or the callback.
 * @param {function(ChromeWindow): void=} opt_callback Callback.
 */
chrome.windows.getCurrent = function(opt_getInfo, opt_callback) { };


/**
 * @param {Object=} opt_getInfo May have 'populate' key. Or the callback.
 * @param {function(ChromeWindow): void=} opt_callback Callback.
 */
chrome.windows.getLastFocused = function(opt_getInfo, opt_callback) { };


/**
 * @param {number} tabId Tab Id.
 * @param {function(): void=} opt_callback Callback.
 */
chrome.windows.remove = function(tabId, opt_callback) {};


/**
 * @param {number} tabId Tab Id.
 * @param {Object} updateProperties An object which may have many keys for
 *     various options.
 * @param {function(): void=} opt_callback Callback.
 */
chrome.windows.update = function(tabId, updateProperties, opt_callback) {};


/** @type {ChromeEvent} */
chrome.windows.onCreated;


/** @type {ChromeEvent} */
chrome.windows.onFocusChanged;


/** @type {ChromeEvent} */
chrome.windows.onRemoved;


/**
 * @see http://code.google.com/chrome/extensions/windows.html#property-WINDOW_ID_NONE
 * @type {number}
 */
chrome.windows.WINDOW_ID_NONE;


/**
 * @see http://code.google.com/chrome/extensions/windows.html#property-WINDOW_ID_CURRENT
 * @type {number}
 */
chrome.windows.WINDOW_ID_CURRENT;



/**
 * @see http://code.google.com/chrome/extensions/windows.html
 * @constructor
 */
function ChromeWindow() {}


/** @type {number} */
ChromeWindow.prototype.id;


/** @type {boolean} */
ChromeWindow.prototype.focused;


/** @type {number} */
ChromeWindow.prototype.top;


/** @type {number} */
ChromeWindow.prototype.left;


/** @type {number} */
ChromeWindow.prototype.width;


/** @type {number} */
ChromeWindow.prototype.height;


/** @type {Array.<Tab>} */
ChromeWindow.prototype.tabs;


/** @type {boolean} */
ChromeWindow.prototype.incognito;


/** @type {string} */
ChromeWindow.prototype.type;


/** @type {string} */
ChromeWindow.prototype.state;


/** @type {boolean} */
ChromeWindow.prototype.alwaysOnTop;


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


/** @type {Console} */
var console;
