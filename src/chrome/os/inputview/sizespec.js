goog.provide('i18n.input.chrome.inputview.SizeSpec');



goog.scope(function() {

var SizeSpec = i18n.input.chrome.inputview.SizeSpec;


/**
 * The height of the keyboard in a11y mode.
 *
 * @type {number}
 */
SizeSpec.A11Y_HEIGHT = 280;


/**
 * The height of the keyboard in non-a11y mode.
 *
 * @type {number}
 */
SizeSpec.NON_A11Y_HEIGHT = 372;


/** @type {number} */
SizeSpec.A11Y_CANDIDATE_VIEW_HEIGHT = 32;


/** @type {number} */
SizeSpec.NON_A11Y_CANDIDATE_VIEW_HEIGHT = 48;


/**
 * The width percent of a11y keyboard in horizontal mode or vertical mode.
 *
 * @enum {number}
 */
SizeSpec.A11Y_WIDTH_PERCENT = {
  HORIZONTAL: 0.74,
  VERTICAL: 0.88
};


/**
 * The width percent of non-a11y keyboard in horizontal mode or vertical mode.
 *
 * @enum {number}
 */
SizeSpec.NON_A11Y_WIDTH_PERCENT = {
  HORIZONTAL: 0.84,
  HORIZONTAL_WIDE_SCREEN: 0.8,
  VERTICAL: 0.88
};

});  // goog.scope

