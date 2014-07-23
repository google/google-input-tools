goog.provide('i18n.input.chrome.inputview.CandidatesInfo');



/**
 * The candidates information.
 *
 * @param {string} source .
 * @param {!Array.<!Object>} candidates .
 * @param {Array.<number>=} opt_matchedLengths .
 * @constructor
 */
i18n.input.chrome.inputview.CandidatesInfo = function(source, candidates,
    opt_matchedLengths) {
  /** @type {string} */
  this.source = source;

  /** @type {!Array.<!Object>} */
  this.candidates = candidates;

  /** @type {Array.<number>} */
  this.matchedLengths = opt_matchedLengths || null;
};


/**
 * Gets an empty suggestions instance.
 *
 * @return {!i18n.input.chrome.inputview.CandidatesInfo} .
 */
i18n.input.chrome.inputview.CandidatesInfo.getEmpty = function() {
  return new i18n.input.chrome.inputview.CandidatesInfo('', []);
};

