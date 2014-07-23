goog.provide('i18n.input.chrome.inputview.PointerConfig');


/**
 * The configuration of the pointer.
 *
 * @param {boolean} dblClick .
 * @param {boolean} longPressWithPointerUp .
 * @param {boolean} longPressWithoutPointerUp .
 * @constructor
 */
i18n.input.chrome.inputview.PointerConfig = function(dblClick,
    longPressWithPointerUp, longPressWithoutPointerUp) {
  /**
   * True to enable double click.
   *
   * @type {boolean}
   */
  this.dblClick = dblClick;

  /**
   * True to enable long press and not cancel the next pointer up.
   *
   * @type {boolean}
   */
  this.longPressWithPointerUp = longPressWithPointerUp;

  /**
   * True to enable long press and cancel the next pointer up.
   *
   * @type {boolean}
   */
  this.longPressWithoutPointerUp = longPressWithoutPointerUp;

  /**
   * The flicker direction.
   *
   * @type {number}
   */
  this.flickerDirection = 0;

  /**
   * The delay of the long press.
   *
   * @type {number}
   */
  this.longPressDelay = 0;
};

