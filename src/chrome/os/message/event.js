goog.provide('i18n.input.chrome.message.Event');

goog.require('goog.events.Event');



/**
 * The message event.
 *
 * @param {i18n.input.chrome.message.Type} type .
 * @param {Object} msg .
 * @param {Function=} opt_sendResponse .
 * @constructor
 * @extends {goog.events.Event}
 */
i18n.input.chrome.message.Event = function(type, msg, opt_sendResponse) {
  goog.base(this, type);

  /** @type {Object} */
  this.msg = msg;

  /** @type {Function} */
  this.sendResponse = opt_sendResponse || null;
};
goog.inherits(i18n.input.chrome.message.Event,
    goog.events.Event);

