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
goog.provide('i18n.input.chrome.Background');

goog.require('goog.Disposable');
goog.require('goog.events.EventHandler');
goog.require('goog.events.EventTarget');
goog.require('goog.object');
goog.require('i18n.input.chrome.Env');
goog.require('i18n.input.chrome.EventType');
goog.require('i18n.input.chrome.inputview.PerfTracker');
goog.require('i18n.input.chrome.message.Name');
goog.require('i18n.input.chrome.message.Type');
goog.require('i18n.input.chrome.xkb.Controller');



goog.scope(function() {
var Env = i18n.input.chrome.Env;
var Name = i18n.input.chrome.message.Name;
var Type = i18n.input.chrome.message.Type;
var PerfTracker = i18n.input.chrome.inputview.PerfTracker;



/**
 * The background class implements the script for the background page of chrome
 * os extension for hmm and t13n input tools.
 *
 * @constructor
 * @extends {goog.Disposable}
 */
i18n.input.chrome.Background = function() {
  goog.base(this);

  /**
   * Array of waiting event handlers.
   * @protected {!Array.<function()>}
   */
  this.waitingEventHandlers = [];

  /**
   * The controller mapping.
   *
   * @protected {!Object.<number, !i18n.input.chrome.AbstractController>}
   */
  this.controllers = {};

  /**
   * The event target to communicate to lower layer.
   *
   * @protected {!goog.events.EventTarget}
   */
  this.eventTarget = new goog.events.EventTarget();
  this.registerDisposable(this.eventTarget);

  /** @private {!Function} */
  this.activateFn_ = this.onActivate.bind(this);

  /** @private {!Function} */
  this.deactivateFn_ = this.onDeactivate_.bind(this);

  /** @private {!Function} */
  this.focusFn_ = this.onFocus.bind(this);

  /** @private {!Function} */
  this.blurFn_ = this.onBlur.bind(this);

  /** @private {!Function} */
  this.onInputContextUpdateFn_ = this.wrapAsyncHandler_(
      this.onInputContextUpdate_);

  /** @private {!Function} */
  this.onSurroundingTextFn_ = this.wrapAsyncHandler_(
      this.onSurroundingTextChanged_);

  /** @private {!Function} */
  this.onKeyEventFn_ = this.wrapAsyncHandler_(
      this.onKeyEventAsync);

  /** @private {!Function} */
  this.onCandidateClickFn_ = this.wrapAsyncHandler_(
      this.onCandidateClicked_);

  /** @private {!Function} */
  this.onMenuItemFn_ = this.wrapAsyncHandler_(
      this.onMenuItemActivated_);

  /** @private {!Function} */
  this.onResetFn_ = this.wrapAsyncHandler_(this.onReset);

  /** @private {!PerfTracker} */
  this.perfTracker_ = new PerfTracker(PerfTracker.TickName.
      BACKGROUND_HTML_LOADED);

  /**
   * The active controller.
   *
   * @protected {!i18n.input.chrome.AbstractController}
   */
  this.activeController = new i18n.input.chrome.xkb.Controller();
  this.controllers[ControllerType.XKB] = this.activeController;

  /** @protected {!goog.events.EventHandler} */
  this.eventHandler = new goog.events.EventHandler(this);

  /** @protected {!Env} */
  this.env = Env.getInstance();

  // Sets up a listener which communicate with the
  // option page or inputview
  // window.
  chrome.runtime.onMessage.addListener(this.wrapAsyncHandler_(this.onMessage));
  this.eventHandler.listen(this.eventTarget,
      i18n.input.chrome.EventType.EXECUTE_WAITING_EVENT,
      this.executeWaitingEventHandlers)
      .listen(this.eventTarget,
          i18n.input.chrome.EventType.LISTEN_KEY_EVENT,
          this.handleListenKeyEvent_);

  this.init_();
};
goog.inherits(i18n.input.chrome.Background, goog.Disposable);
var Background = i18n.input.chrome.Background;


/**
 * Flag to indicate whether the background is initialized.
 *
 * @private {boolean}
 */
Background.prototype.initialized_ = false;


/**
 * The controller type
 * @enum {number}
 */
Background.ControllerType = {
  HMM: 0,
  T13N: 1,
  ZHUYIN: 2,
  LATIN: 3,
  JA: 4,
  HWT: 5,
  XKB: 6,
  VKD: 7,
  KOREAN: 8,
  EMOJI: 9,
  VOICE: 10
};
var ControllerType = Background.ControllerType;


/**
 * The max queue length.
 *
 * @private {number}
 */
Background.MAX_QUEUE_LEN_ = 255;


/**
 * The previous active controller, when handwriting panel switch back to
 * previous inputtool, need set previous active controller.
 *
 * @protected {!i18n.input.chrome.AbstractController}
 */
Background.prototype.previousActiveController;


/**
 * Whether the background is listening the key events.
 *
 * @private {boolean}
 */
Background.prototype.listeningKeyEvents_ = false;


/**
 * Listens or unlistens the key events.
 *
 * @param {boolean} listen Whether listen or unlisten.
 * @protected
 */
Background.prototype.listenKeyEvents = function(listen) {
  if (this.listeningKeyEvents_ == listen) return;
  if (listen) {
    chrome.input.ime.onKeyEvent.addListener(this.onKeyEventFn_, ['async']);
  } else {
    chrome.input.ime.onKeyEvent.removeListener(this.onKeyEventFn_);
  }
  this.listeningKeyEvents_ = listen;
};


/**
 * Uninit.
 *
 * @protected
 */
Background.prototype.uninit = function() {
  if (!this.initialized_) {
    return;
  }

  // Removes event handlers.
  chrome.input.ime.onActivate.removeListener(
      this.activateFn_);
  chrome.input.ime.onDeactivated.removeListener(
      this.deactivateFn_);
  chrome.input.ime.onFocus.removeListener(
      this.focusFn_);
  chrome.input.ime.onBlur.removeListener(
      this.blurFn_);
  chrome.input.ime.onKeyEvent.removeListener(
      this.onKeyEventFn_);
  chrome.input.ime.onCandidateClicked.removeListener(
      this.onCandidateClickFn_);
  chrome.input.ime.onMenuItemActivated.removeListener(
      this.onMenuItemFn_);
  chrome.input.ime.onInputContextUpdate.removeListener(
      this.onInputContextUpdateFn_);
  if (chrome.input.ime.onSurroundingTextChanged) {
    chrome.input.ime.onSurroundingTextChanged.removeListener(
        this.onSurroundingTextFn_);
  }

  if (chrome.input.ime.onReset) {
    chrome.input.ime.onReset.removeListener(this.onResetFn_);
  }

  this.activeController.deactivate();

  this.initialized_ = false;
};


/**
 * Initializes the background scripts.
 *
 * @private
 */
Background.prototype.init_ = function() {
  if (this.initialized_) {
    return;
  }

  chrome.input.ime.onActivate.addListener(
      this.activateFn_);
  chrome.input.ime.onDeactivated.addListener(
      this.deactivateFn_);
  chrome.input.ime.onFocus.addListener(
      this.focusFn_);
  chrome.input.ime.onBlur.addListener(
      this.blurFn_);
  chrome.input.ime.onCandidateClicked.addListener(
      this.onCandidateClickFn_);
  chrome.input.ime.onMenuItemActivated.addListener(
      this.onMenuItemFn_);
  chrome.input.ime.onInputContextUpdate.addListener(
      this.onInputContextUpdateFn_);
  if (chrome.input.ime.onSurroundingTextChanged) {
    chrome.input.ime.onSurroundingTextChanged.addListener(
        this.onSurroundingTextFn_);
  }
  if (chrome.input.ime.onReset) {
    chrome.input.ime.onReset.addListener(this.onResetFn_);
  }

  this.initialized_ = true;
  window['setContext'] = this.onFocus.bind(this);
  window['addEventListener']('unload', function() {
    this.dispose();
  }.bind(this));
};


/**
 * Wraps event handler to be able to managed by waitingEventHandlers.
 * @param {!Function} handler Event handler.
 * @return {!Function} Wraped Event handler.
 * @private
 */
Background.prototype.wrapAsyncHandler_ = function(handler) {
  return (/**
           * @this {!Background}
           * @param {!Function} handler
           */
          (function(handler) {
            if (this.waitingEventHandlers.length < Background.MAX_QUEUE_LEN_) {
              this.waitingEventHandlers.push(
                  Function.prototype.apply.bind(handler, this,
                      Array.prototype.slice.call(arguments, 1)));
            }
            this.executeWaitingEventHandlers();
          })).bind(this, handler);
};


/**
 * Gets whether the active controller is ready.
 *
 * @return {boolean} Whether the active controller is ready.
 */
Background.prototype.isControllerReady = function() {
  return this.activeController.isReady();
};


/**
 * Handles listen key event from the controller.
 *
 * @private
 */
Background.prototype.handleListenKeyEvent_ = function() {
  this.listenKeyEvents(this.activeController.isInterestedInKeyEvents());
};


/**
 * Executes the waiting event handlers.
 * This function is used to prevent the event handlers from being executed while
 * the NaCl module is being initialized or another event handler is being
 * executed or waiting for the callback from the NaCl module.
 *
 * @protected
 */
Background.prototype.executeWaitingEventHandlers = function() {
  this.perfTracker_.tick(PerfTracker.TickName.NACL_LOADED);
  this.perfTracker_.stop();
  while (this.isControllerReady() && this.waitingEventHandlers.length > 0) {
    /** @preserveTry */
    try {
      this.waitingEventHandlers.shift()();
    } catch (e) {
      console.log(e.stack);
    }
  }
};


/**
 * Processes incoming message from option page or inputview window.
 *
 * @param {*} message Message from option page or inputview window.
 * @param {!MessageSender} sender Information about the script
 *     context that sent the message.
 * @param {function(*): void} sendResponse Function to call to send a response.
 * @return {boolean|undefined} {@code true} to keep the message channel open in
 *     order to send a response asynchronously.
 */
Background.prototype.onMessage = function(message, sender, sendResponse) {
  if (message[Name.TYPE] == Type.VISIBILITY_CHANGE &&
      message[Name.VISIBILITY]) {
    chrome.input.ime.setCandidateWindowProperties(goog.object.create(
        Name.ENGINE_ID, this.env.engineId,
        Name.PROPERTIES, goog.object.create(
            Name.VISIBLE, false)));
  }
  if (this.activeController) {
    this.activeController.processMessage(message, sender, sendResponse);
  }
  return false;
};


/**
 * Callback method called when IME is activated.
 *
 * @param {string} engineId ID of the engine.
 * @param {string} screenType The current screen type.
 */
Background.prototype.onActivate = function(engineId, screenType) {
  this.env.engineId = engineId;
  chrome.runtime.onSuspend.addListener((function() {
    this.uninit();
  }).bind(this));

  if (this.activeController) {
    // Resets the previous controller.
    this.activeController.onCompositionCanceled();
  }

  this.activeController = this.controllers[ControllerType.XKB];
  if (!this.activeController) {
    this.activeController = new i18n.input.chrome.xkb.Controller();
    this.controllers[ControllerType.XKB] = this.activeController;
  }
  this.activeController.activate(engineId);
  this.activeController.setScreenType(screenType);
  if (this.env.context) {
    this.activeController.register(this.env.context);
  }
  this.listenKeyEvents(this.activeController.isInterestedInKeyEvents());
};


/**
 * Callback method called when IME is deactivated.
 * @param {string} engineID ID of the engine.
 * @private
 */
Background.prototype.onDeactivate_ = function(engineID) {
  this.activeController && this.activeController.deactivate();
  this.listenKeyEvents(false);
};


/**
 * Callback method called when a context acquires a focus.
 * @param {!InputContext} context The context information.
 * @protected
 */
Background.prototype.onFocus = function(context) {
  this.env.context = context;
  this.activeController && this.activeController.register(context);
};


/**
 * Callback method called when a context lost a focus.
 *
 * @param {number} contextID ID of the context.
 * @param {string=} opt_screen The screen type.
 * @protected
 */
Background.prototype.onBlur = function(contextID, opt_screen) {
  this.env.context = null;
  this.activeController && this.activeController.unregister();
};


/**
 * Callback method called when Chrome terminates ongoing text input session.
 * @param {string} engineID ID of the engine.
 * @protected
 */
Background.prototype.onReset = function(engineID) {
  this.activeController && this.activeController.onCompositionCanceled();
};


/**
 * Callback method called when IME catches a new key event.
 * @param {string} engineID ID of the engine.
 * @param {!ChromeKeyboardEvent} keyData key event data.
 * @protected
 */
Background.prototype.onKeyEventAsync = function(engineID, keyData) {
  if (this.activeController) {
    // Sets the correct next request id.
    this.activeController.requestId = Number(keyData.requestId) + 1;
    var ret;
    try {
      ret = this.activeController.handleEvent(keyData);
    } catch (e) {
      // If throw a exception, means didn't handle the key successful.
      chrome.input.ime.keyEventHandled(keyData.requestId, false);
      console.log(e.stack);
    }
    if (goog.isDef(ret)) {
      chrome.input.ime.keyEventHandled(keyData.requestId, ret);
    }
  } else {
    chrome.input.ime.keyEventHandled(keyData.requestId, false);
  }
};


/**
 * Callback method called when candidates on candidate window is clicked.
 * @param {string} engineID ID of the engine.
 * @param {number} candidateID Index of the candidate.
 * @param {string} button Which mouse button was clicked.
 * @private
 */
Background.prototype.onCandidateClicked_ =
    function(engineID, candidateID, button) {
  this.activeController && this.activeController.selectCandidate(candidateID);
};


/**
 * Callback method called when menu item on uber tray is activated.
 * @param {string} engineID ID of the engine.
 * @param {string} name name of the menu item.
 * @private
 */
Background.prototype.onMenuItemActivated_ = function(engineID, name) {
  this.activeController && this.activeController.switchInputToolState(name);
};


/**
 * Callback method called when properties of the context is changed.
 * @param {!InputContext} context context information.
 * @private
 */
Background.prototype.onInputContextUpdate_ = function(context) {
  this.activeController && this.activeController.onInputContextUpdate &&
      this.activeController.onInputContextUpdate(context);
};


/**
 * Callback method called the editable string around caret is changed or when
 * the caret position is moved. The text length is limited to 100 characters for
 * each back and forth direction.
 * @param {string} engineID ID of the engine.
 * @param {!Object.<{text: string, focus: number, anchor: number,
 *     offset: number}>} info The surrounding information.
 * @private
 */
Background.prototype.onSurroundingTextChanged_ = function(engineID, info) {
  this.env.surroundingInfo = info;
  this.env.textBeforeCursor = info.text.slice(0,
      Math.min(info.anchor, info.focus)).replace(/\u00a0/g, ' ');
  this.activeController &&
      this.activeController.onSurroundingTextChanged(engineID, info);
};


/** @override */
Background.prototype.disposeInternal = function() {
  for (var type in this.controllers) {
    this.controllers[Number(type)].dispose();
  }
  goog.dispose(this.eventHandler);
  goog.base(this, 'disposeInternal');
};

});  // goog.scope

