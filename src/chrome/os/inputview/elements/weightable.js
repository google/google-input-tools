goog.provide('i18n.input.chrome.inputview.elements.Weightable');



/**
 * Interface defines the weight operation to an element which is computed
 * with weight.
 *
 * @interface
 */
i18n.input.chrome.inputview.elements.Weightable = function() {};


/**
 * Gets the width of the element in weight.
 *
 * @return {number} The width.
 */
i18n.input.chrome.inputview.elements.Weightable.prototype.getWidthInWeight =
    goog.abstractMethod;


/**
 * Gets the height of the element in weight.
 *
 * @return {number} The height.
 */
i18n.input.chrome.inputview.elements.Weightable.prototype.getHeightInWeight =
    goog.abstractMethod;


