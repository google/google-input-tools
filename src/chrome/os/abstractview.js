goog.provide('i18n.input.chrome.AbstractView');

goog.require('goog.object');
goog.require('i18n.input.chrome.message.Name');
goog.require('i18n.input.chrome.message.Type');

goog.scope(function() {
var Name = i18n.input.chrome.message.Name;
var Type = i18n.input.chrome.message.Type;



/**
 * The abstract view.
 *
 * @constructor
 */
i18n.input.chrome.AbstractView = function() {};
var AbstractView = i18n.input.chrome.AbstractView;


/**
 * True if this IME is running in standalone mode which means no touchscreen
 * keyboard is attached.
 *
 * @type {boolean}
 */
AbstractView.prototype.standalone = true;


/**
 * The context.
 *
 * @protected {Object}
 */
AbstractView.prototype.context = null;


/**
 * Sets the property of the candidate window.
 *
 * @param {string} engineID .
 * @param {boolean} visible .
 * @param {boolean=} opt_cursorVisible .
 * @param {boolean=} opt_vertical .
 * @param {number=} opt_pageSize .
 */
AbstractView.prototype.setCandidateWindowProperties = function(engineID,
    visible, opt_cursorVisible, opt_vertical, opt_pageSize) {
  if (this.standalone) {
    var properties = goog.object.create(Name.VISIBLE, visible);
    if (goog.isDef(opt_cursorVisible)) {
      properties[Name.CURSOR_VISIBLE] = !!opt_cursorVisible;
    }
    if (goog.isDef(opt_vertical)) {
      properties[Name.VERTICAL] = !!opt_vertical;
    }
    if (goog.isDef(opt_pageSize)) {
      properties[Name.PAGE_SIZE] = opt_pageSize;
    }
    chrome.input.ime.setCandidateWindowProperties(goog.object.create(
        Name.ENGINE_ID, engineID, Name.PROPERTIES, properties));
  } else if (!visible) {
    chrome.runtime.sendMessage(goog.object.create(
        Name.MSG_TYPE, Type.CANDIDATES_BACK,
        Name.MSG, goog.object.create(Name.CANDIDATES, [])));
  }
};


/**
 * Sets the candidates.
 *
 * @param {!Array.<!Object>} candidates .
 */
AbstractView.prototype.setCandidates = function(candidates) {
  if (this.standalone) {
    chrome.input.ime.setCandidates(goog.object.create(
        Name.CONTEXT_ID, this.context.contextID,
        Name.CANDIDATES, candidates));
  } else {
    chrome.runtime.sendMessage(goog.object.create(
        Name.MSG_TYPE, Type.CANDIDATES_BACK,
        Name.MSG, goog.object.create(Name.CANDIDATES, candidates)));
  }
};


/**
 * Sets the cursor position.
 *
 * @param {number} cursorPosition .
 */
AbstractView.prototype.setCursorPosition = function(cursorPosition) {
  if (this.standalone) {
    chrome.input.ime.setCursorPosition(goog.object.create(
        Name.CONTEXT_ID, this.context.contextID,
        Name.CANDIDATE_ID, cursorPosition));
  }
};


/**
 * Sets the context.
 *
 * @param {Object} context The input context.
 */
AbstractView.prototype.setContext = function(context) {
  this.context = context;
};

});  // goog.scope
