goog.provide('i18n.input.chrome.xkb.Correction');


/**
 * The correction item.
 *
 * @param {string} source .
 * @param {string} target .
 * @constructor
 */
i18n.input.chrome.xkb.Correction = function(source, target) {
  /** @type {string} */
  this.source = source;

  /** @type {string} */
  this.target = target;
};

