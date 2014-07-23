goog.provide('i18n.input.chrome.inputview.elements.content.Candidate');

goog.require('goog.dom.classlist');
goog.require('i18n.input.chrome.inputview.Css');
goog.require('i18n.input.chrome.inputview.elements.Element');
goog.require('i18n.input.chrome.inputview.elements.ElementType');
goog.require('i18n.input.chrome.message.Name');

goog.scope(function() {
var ElementType = i18n.input.chrome.inputview.elements.ElementType;
var Css = i18n.input.chrome.inputview.Css;
var TagName = goog.dom.TagName;
var Name = i18n.input.chrome.message.Name;



/**
 * The candidate component.
 *
 * @param {string} id .
 * @param {!Object} candidate .
 * @param {i18n.input.chrome.inputview.elements.content.Candidate.Type}
 *     candidateType .
 * @param {number} height .
 * @param {boolean} isDefault .
 * @param {number=} opt_width .
 * @param {goog.events.EventTarget=} opt_eventTarget .
 * @constructor
 * @extends {i18n.input.chrome.inputview.elements.Element}
 */
i18n.input.chrome.inputview.elements.content.Candidate = function(id,
    candidate, candidateType, height, isDefault, opt_width, opt_eventTarget) {
  goog.base(this, id, ElementType.CANDIDATE, opt_eventTarget);

  /** @type {!Object} */
  this.candidate = candidate;

  /** @type {i18n.input.chrome.inputview.elements.content.Candidate.Type} */
  this.candidateType = candidateType;

  /** @type {number} */
  this.width = opt_width || 0;

  /** @type {number} */
  this.height = height;

  /** @type {boolean} */
  this.isDefault = isDefault;
};
var Candidate = i18n.input.chrome.inputview.elements.content.Candidate;
goog.inherits(Candidate, i18n.input.chrome.inputview.elements.Element);


/**
 * The type of this candidate.
 *
 * @enum {number}
 */
Candidate.Type = {
  CANDIDATE: 0,
  NUMBER: 1
};


/** @override */
Candidate.prototype.createDom = function() {
  goog.base(this, 'createDom');

  var dom = this.getDomHelper();
  var elem = this.getElement();
  goog.dom.classlist.add(elem, Css.CANDIDATE);
  dom.setTextContent(elem, this.candidate[Name.CANDIDATE]);
  elem.style.height = this.height + 'px';
  if (this.width > 0) {
    elem.style.width = this.width + 'px';
  }
  if (this.isDefault) {
    goog.dom.classlist.add(elem, Css.CANDIDATE_DEFAULT);
  }
};


/** @override */
Candidate.prototype.setHighlighted = function(highlight) {
  if (highlight) {
    goog.dom.classlist.add(this.getElement(), Css.CANDIDATE_HIGHLIGHT);
  } else {
    goog.dom.classlist.remove(this.getElement(), Css.CANDIDATE_HIGHLIGHT);
  }
};


});  // goog.scope

