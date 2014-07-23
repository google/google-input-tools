goog.require('goog.Uri');
goog.require('i18n.input.chrome.inputview.Controller');


(function() {
  window.onload = function() {
    var uri = new goog.Uri(window.location.href);
    var code = uri.getParameterValue('id') || 'us';
    var language = uri.getParameterValue('language') || 'en';
    var passwordLayout = uri.getParameterValue('passwordLayout') || 'us';
    var name = chrome.i18n.getMessage(
        uri.getParameterValue('name') || 'English');

    var controller = new i18n.input.chrome.inputview.Controller(code,
        language, passwordLayout, name);
    window.unload = function() {
      goog.dispose(controller);
    };

    // Methods below are used for automation test.
    window.isKeyboardReady = function() {
      return controller.isKeyboardReady;
    };

    window.startTest = function() {
      controller.resize(true);
    };
  };
})();

