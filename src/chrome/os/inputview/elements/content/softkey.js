goog.provide('i18n.input.chrome.inputview.elements.content.SoftKey');

goog.require('goog.dom.classlist');
goog.require('goog.math.Coordinate');
goog.require('goog.style');
goog.require('i18n.input.chrome.inputview.Css');
goog.require('i18n.input.chrome.inputview.elements.Element');
goog.require('i18n.input.chrome.inputview.elements.content.GaussianEstimator');
goog.require('i18n.input.chrome.inputview.util');



goog.scope(function() {



/**
 * The base soft key class.
 *
 * @param {string} id The id.
 * @param {!i18n.input.chrome.inputview.elements.ElementType} type The element
 *     type.
 * @param {goog.events.EventTarget=} opt_eventTarget The event target.
 * @constructor
 * @extends {i18n.input.chrome.inputview.elements.Element}
 */
i18n.input.chrome.inputview.elements.content.SoftKey = function(id, type,
    opt_eventTarget) {
  goog.base(this, id, type, opt_eventTarget);

  /**
   * The available width.
   *
   * @type {number}
   */
  this.availableWidth = 0;

  /**
   * The available height.
   *
   * @type {number}
   */
  this.availableHeight = 0;

  /**
   * The nearby keys.
   *
   * @type {!Array.<!SoftKey>}
   */
  this.nearbyKeys = [];
};
goog.inherits(i18n.input.chrome.inputview.elements.content.SoftKey,
    i18n.input.chrome.inputview.elements.Element);
var SoftKey = i18n.input.chrome.inputview.elements.content.SoftKey;


/**
 * The covariance for gaussian estimator.
 * TODO: Needs training here.
 *
 * @type {number}
 * @private
 */
SoftKey.GAUSSIAN_COVARIANCE_ = 120;


/**
 * The coordinate of the center point.
 *
 * @type {!goog.math.Coordinate}
 */
SoftKey.prototype.centerCoordinate;


/**
 * The coordinate of the top-left point.
 *
 * @type {!goog.math.Coordinate}
 */
SoftKey.prototype.topLeftCoordinate;


/**
 * The gaussian estimator.
 *
 * @type {!i18n.input.chrome.inputview.elements.content.GaussianEstimator}
 */
SoftKey.prototype.estimator;


/** @override */
SoftKey.prototype.createDom = function() {
  goog.base(this, 'createDom');

  goog.dom.classlist.add(this.getElement(),
      i18n.input.chrome.inputview.Css.SOFT_KEY);
};


/** @override */
SoftKey.prototype.resize = function(width,
    height) {
  goog.base(this, 'resize', width, height);

  var elem = this.getElement();
  var borderWidth = i18n.input.chrome.inputview.util.getPropertyValue(
      elem, 'borderWidth');
  var marginTop = i18n.input.chrome.inputview.util.getPropertyValue(
      elem, 'marginTop');
  var marginBottom = i18n.input.chrome.inputview.util.getPropertyValue(
      elem, 'marginBottom');
  var marginLeft = i18n.input.chrome.inputview.util.getPropertyValue(
      elem, 'marginLeft');
  var marginRight = i18n.input.chrome.inputview.util.getPropertyValue(
      elem, 'marginRight');
  var w = width - borderWidth * 2 - marginLeft - marginRight;
  var h = height - borderWidth * 2 - marginTop - marginBottom;
  elem.style.width = w + 'px';
  elem.style.height = h + 'px';

  this.availableWidth = w;
  this.availableHeight = h;

  this.topLeftCoordinate = goog.style.getClientPosition(elem);
  this.centerCoordinate = new goog.math.Coordinate(
      this.topLeftCoordinate.x + width / 2,
      this.topLeftCoordinate.y + height / 2);
  this.estimator = new i18n.input.chrome.inputview.elements.content.
      GaussianEstimator(this.centerCoordinate, SoftKey.GAUSSIAN_COVARIANCE_,
          this.availableHeight / this.availableWidth);
};

});  // goog.scope
