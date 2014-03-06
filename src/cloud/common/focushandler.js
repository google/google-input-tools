// Copyright 2014 The Cloud Input Tools Authors. All Rights Reserved.
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
 * @fileoverview Provides common operation of dom for input tools.
 */


goog.provide('i18n.input.common.FocusHandler');

goog.require('goog.Timer');
goog.require('goog.array');
goog.require('goog.dom');
goog.require('goog.dom.TagName');
goog.require('goog.events.BrowserEvent');
goog.require('goog.events.EventHandler');
goog.require('goog.events.EventTarget');
goog.require('goog.object');
goog.require('goog.uri.utils');
goog.require('goog.userAgent');
goog.require('i18n.input.api.DocumentReplacer');
goog.require('i18n.input.common.GlobalSettings');
goog.require('i18n.input.common.dom');



/**
 * Tracks focus switch between elements of page, include iframe. If focus
 * causes current active element changing then fire an events. Suggests
 * pass current active element document.
 *
 * @param {!Document|!Array.<!Document>} doc The document or array of documents
 *     to listen on.
 * @constructor
 * @extends {goog.events.EventTarget}
 */
i18n.input.common.FocusHandler = function(doc) {
  goog.base(this);

  /**
   * Mark the visited documents flag.
   *
   * @type {!Object.<number, boolean>}
   * @private
   */
  this.visitedDocuments_ = {};

  /**
   * The main document.
   *
   * @type {!Document}
   * @private
   */
  this.mainDocument_ = goog.isArrayLike(doc) ? doc[0] : doc;

  /**
   * The current active element.
   *
   * @type {?Element}
   * @private
   */
  this.currentActiveElement_ = null;

  /**
   * The event handler.
   *
   * @type {goog.events.EventHandler}
   * @private
   */
  this.eventHandler_ = new goog.events.EventHandler(this);

  this.addFocusHandler_(doc);
};
goog.inherits(i18n.input.common.FocusHandler, goog.events.EventTarget);


/**
 * Enum type for the events fired by the focus handler.
 *
 * @enum {string}
 */
i18n.input.common.FocusHandler.EventType = {
  ACTIVE_ELEMENT_CHANGE: 'aec'
};


/**
 * Initializes the focus handler.
 *
 * @param {!Document|!Array.<!Document>} doc The document or array of documents
 *     to listen on.
 * @private
 */
i18n.input.common.FocusHandler.prototype.addFocusHandler_ = function(doc) {
  var docs = /** @type {!Array.<!Document>} */
      (goog.isArrayLike(doc) ? doc : [doc]);

  // In IE we use focusin/focusout and in other browsers we use a capturing
  // listener for focus/blur
  var typeIn = goog.userAgent.IE ? 'focusin' : 'focus';
  var typeOut = goog.userAgent.IE ? 'focusout' : 'blur';

  goog.array.forEach(docs, function(value) {
    var uid = goog.getUid(value);
    if (!this.visitedDocuments_[uid]) {
      this.visitedDocuments_[uid] = true;
      var target = goog.userAgent.WEBKIT ?
          goog.dom.getWindow(value) : value;
      this.eventHandler_.listen(target, typeIn, this.onFocusIn_,
          i18n.input.common.GlobalSettings.canListenInCaptureForIE8);
      this.eventHandler_.listen(target, typeOut, this.onFocusOut_,
          i18n.input.common.GlobalSettings.canListenInCaptureForIE8);
    }
  }, this);
};


/**
 * Handles the focus in event.
 *
 * @param {!goog.events.BrowserEvent} e The event.
 * @private
 */
i18n.input.common.FocusHandler.prototype.onFocusIn_ = function(e) {
  var target = e.target;
  if (target && target != i18n.input.api.DocumentReplacer.getWindow()) {
    if (target.frameElement) {
      target = target.frameElement;
    }
    this.fireEvent_(e, /** @type {!Element} */ (target));
  }
};


/**
 * Handles the focus out event.
 *
 * @param {!goog.events.BrowserEvent} e The event.
 * @private
 */
i18n.input.common.FocusHandler.prototype.onFocusOut_ = function(e) {
  // Browser setting document active element has some latency,
  // so set up time to solve it.

  // In IE, the Event object will be invalid after the event handling has
  // finished. Take a copy of the Event object and create a new BrowserEvent.
  var rawEventCopy = /** @type {Event} */
      (goog.object.clone(e.getBrowserEvent()));
  var wrappedFocusOut = goog.bind(this.onFocusOutInner_,
      this, this.mainDocument_, new goog.events.BrowserEvent(rawEventCopy));
  goog.Timer.callOnce(wrappedFocusOut, 0);
};


/**
 * Handles the focus out event.
 *
 * @param {!Document} currentDocument The document.
 * @param {!goog.events.BrowserEvent} e The event.
 * @private
 */
i18n.input.common.FocusHandler.prototype.onFocusOutInner_ =
    function(currentDocument, e) {
  if (!this.visitedDocuments_) {
    // Avoids this method is called after the focus handler is disposed.
    return;
  }

  // TODO(shuchen): remove the below check for docs apps and investigate the
  // root cause of b/7250478.
  var appName = i18n.input.common.GlobalSettings.ApplicationName;
  if (appName != 'kix' && appName != 'punch' && appName != 'trix' ||
      !goog.userAgent.IE) {
    var domHelper = goog.dom.getDomHelper(currentDocument);
    // If switches from one iframe to another iframe, the activeElement of the
    // parent document won't change, so iterate all the iframes.
    goog.array.forEach(i18n.input.common.dom.getSameDomainDocuments(
        domHelper.getDocument()), function(frameDoc) {
          /** @preserveTry */
          try {
            // In IE and FF3.5, some iframe is not allowed to access.
            // It throws exception when access the iframe property.
            this.addFocusHandler_(frameDoc);
          } catch (ex) {}
        }, this);
  }

  // Gets the current active element.
  var activeElement = goog.dom.getActiveElement(currentDocument);
  if (activeElement) {
    var parentActiveElement = activeElement;

    // The current active element is iframe. Add focus event on its inner
    // document.
    var tagName = activeElement.tagName;
    while (tagName && (tagName == goog.dom.TagName.IFRAME ||
            tagName == goog.dom.TagName.FRAME)) {
      if (activeElement.src &&
          !goog.uri.utils.haveSameDomain(activeElement.src,
              window.location.href)) {
        break;
      }
      var innerDocument = goog.dom.getFrameContentDocument(activeElement);
      if (!innerDocument) {
        break;
      }
      this.addFocusHandler_(innerDocument);

      var innerDocumentActiveElement = goog.dom.getActiveElement(innerDocument);
      if (!innerDocumentActiveElement) {
        break;
      }

      parentActiveElement = activeElement;
      activeElement = innerDocumentActiveElement;
      tagName = activeElement.tagName;
      if (tagName && tagName.toUpperCase() == goog.dom.TagName.BODY) {
        // Resets active element to parent iframe.
        activeElement = parentActiveElement;
        break;
      }
    }
    this.fireEvent_(e, activeElement);
  }
};


/**
 * Fires active element change event if current active element changes.
 *
 * @param {!goog.events.BrowserEvent} e The browser event.
 * @param {Element} activeElement The active element.
 * @private
 */
i18n.input.common.FocusHandler.prototype.fireEvent_ =
    function(e, activeElement) {
  if (activeElement != this.currentActiveElement_) {
    // Resets the current active element.
    this.currentActiveElement_ = activeElement;
    // Fires the active element change event.
    var be = e.getBrowserEvent();
    var event = new goog.events.BrowserEvent(be);
    event.target = activeElement;
    event.type = i18n.input.common.FocusHandler.EventType.ACTIVE_ELEMENT_CHANGE;

    this.dispatchEvent(event);
  }
};


/** @override */
i18n.input.common.FocusHandler.prototype.disposeInternal = function() {
  goog.base(this, 'disposeInternal');
  // In Docs, some iframes' domains may be changed during runtime.
  // To avoid different domain exception, add try to catch it.
  /** @preserveTry */
  try {
    goog.dispose(this.eventHandler_);
  } catch (e) {}
  delete this.visitedDocuments_;
  delete this.currentActiveElement_;
};
