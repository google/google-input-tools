// Copyright 2012 Google Inc. All Rights Reserved.

/**
 * @fileoverview Handwriting event type.
 * @author wuyingbing@google.com (Yingbing Wu)
 */


goog.provide('i18n.input.hwt.CandidateSelectEvent');
goog.provide('i18n.input.hwt.CommitEvent');
goog.provide('i18n.input.hwt.EventType');

goog.require('goog.events');
goog.require('goog.events.Event');


/**
 * Handwriting event type to expose to external.
 *
 * @enum {string}
 */
i18n.input.hwt.EventType = {
  BACKSPACE: goog.events.getUniqueId('b'),
  CANDIDATE_SELECT: goog.events.getUniqueId('cs'),
  COMMIT: goog.events.getUniqueId('c'),
  COMMIT_START: goog.events.getUniqueId('hcs'),
  COMMIT_END: goog.events.getUniqueId('hce'),
  RECOGNITION_READY: goog.events.getUniqueId('rr'),
  ENTER: goog.events.getUniqueId('e'),
  HANDWRITING_CLOSED: goog.events.getUniqueId('hc'),
  MOUSEUP: goog.events.getUniqueId('m'),
  SPACE: goog.events.getUniqueId('s')
};



/**
 * Candidate select event.
 *
 * @param {string} candidate The candidate.
 * @constructor
 * @extends {goog.events.Event}
 */
i18n.input.hwt.CandidateSelectEvent = function(candidate) {
  goog.base(this, i18n.input.hwt.EventType.CANDIDATE_SELECT);

  /**
   * The candidate.
   *
   * @type {string}
   */
  this.candidate = candidate;
};
goog.inherits(i18n.input.hwt.CandidateSelectEvent, goog.events.Event);



/**
 * Commit event.
 *
 * @param {string} text The text to commit.
 * @param {number=} opt_back The number of characters to be deleted.
 * @constructor
 * @extends {goog.events.Event}
 */
i18n.input.hwt.CommitEvent = function(text, opt_back) {
  goog.base(this, i18n.input.hwt.EventType.COMMIT);

  /**
   * The text.
   *
   * @type {string}
   */
  this.text = text;

  /**
   * The number of characters to be deleted.
   *
   * @type {number}
   */
  this.back = opt_back || 0;
};
goog.inherits(i18n.input.hwt.CommitEvent, goog.events.Event);

