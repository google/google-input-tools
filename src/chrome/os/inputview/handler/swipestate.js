goog.provide('i18n.input.chrome.inputview.handler.SwipeState');



/**
 * The state for the swipe action.
 *
 * @constructor
 */
i18n.input.chrome.inputview.handler.SwipeState = function() {
  /**
   * The offset in x-axis.
   *
   * @type {number}
   */
  this.offsetX = 0;

  /**
   * The offset in y-axis.
   *
   * @type {number}
   */
  this.offsetY = 0;

  /**
   * The previous x coordinate.
   *
   * @type {number}
   */
  this.previousX = 0;

  /**
   * The previous y coordinate.
   *
   * @type {number}
   */
  this.previousY = 0;
};


/**
 * Resets the state.
 */
i18n.input.chrome.inputview.handler.SwipeState.prototype.reset =
    function() {
  this.offsetX = this.offsetY = this.previousX = this.previousY = 0;
};

