var xkb = {};

/**
 * @param {number} num .
 * @param {Function} callback .
 * @constructor
 */
xkb.DataSource = function(num, callback) {};


/**
 * @param {string} language .
 */
xkb.DataSource.prototype.setLanguage = function(language) {};


/**
 * @param {string} query .
 * @param {!Object=} opt_spatialData .
 */
xkb.DataSource.prototype.sendCompletionRequest = function(query,
    opt_spatialData) {};


/**
 * @param {string} query .
 */
xkb.DataSource.prototype.sendPredictionRequest = function(query) {};


/**
 * @param {number} level .
 */
xkb.DataSource.prototype.setCorrectionLevel = function(level) {};


/**
 * @return {boolean} .
 */
xkb.DataSource.prototype.isReady = function() {return false};


/**
 * @param {!Object} keyData .
 */
xkb.handleSendKeyEvent = function(keyData) {};


var inputview = {};


/**
 * @param {!Function} callback .
 */
inputview.getKeyboardConfig = function(callback) {};


/**
 * @param {!Function} callback .
 */
inputview.getInputMethods = function(callback) {};


/**
 * @param {string} inputMethodId .
 */
inputview.switchToInputMethod = function(inputMethodId) {};

