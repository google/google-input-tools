/**
 * Provides mock implementation of Chrome extension APIs for testing.
 */

var chrome = {};

/**
 * Adds mocks for chrome extension API calls.
 *
 * @param {goog.testing.MockControl} mockControl Controller for validating
 *     calls with expectations.
 */
function mockExtensionApis(mockControl) {

  var addMocks = function(base, name, methods) {
    base[name] = {};
    methods.forEach(function(m) {
      var fn = base[name][m] = mockControl.createFunctionMock(m);
      // Method to arm triggering a callback function with the specified
      // arguments. As it is awkward to require an exact signature match for
      // methods with a callback, the verification process is relaxed to simply
      // require that the sole argument is a function for each recorded call.
      fn.$callbackData = function(args) {
        fn().$anyTimes().$does(function(callback) {
          callback.call(undefined, args);
        });
        fn.$verifyCall = function(currentExpectation, name, args) {
          return args.length == 1 && typeof args[0] == 'function';
        };
      };
    });
  };

  addMocks(chrome, 'virtualKeyboardPrivate', ['getKeyboardConfig']);
  addMocks(chrome, 'inputMethodPrivate', ['getInputMethods']);
  addMocks(chrome, 'runtime', ['getBackgroundPage', 'sendMessage']);
  addMocks(chrome.runtime, 'onMessage', ['addListener']);

  // Ignore calls to addListener. Reevaluate if important to properly track the
  // flow of events.
  chrome.runtime.onMessage.addListener = function() {};
}

