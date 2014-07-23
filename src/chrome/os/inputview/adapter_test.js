goog.provide('i18n.input.chrome.inputview.AdapterTest');
goog.setTestOnly('i18n.input.chrome.inputview.AdapterTest');

goog.require('goog.testing.MockControl');
goog.require('goog.testing.jsunit');
goog.require('i18n.input.chrome.inputview.Adapter');
goog.require('i18n.input.chrome.inputview.privateapi');

function setUp() {
  mockControl = new goog.testing.MockControl;
  mockExtensionApis(mockControl);
}

function tearDown() {
  mockControl.$tearDown();
}

function testInitialize() {
  // Simulate loading of configuration.
  var settingsTest = function(keyboardConfig, inputMethods, isA11y, showGlobe) {
    var adapter = new i18n.input.chrome.inputview.Adapter();
    assertTrue('Unable to instantiate Adapter object', !!adapter);
    assertFalse(adapter.isA11yMode);
    assertFalse(adapter.showGlobeKey);
    // Record expectations and set data for callbacks.
    mockControl.$resetAll();
    var Type = i18n.input.chrome.message.Type;
    chrome.virtualKeyboardPrivate.getKeyboardConfig.$callbackData(
        keyboardConfig);
    chrome.inputMethodPrivate.getInputMethods.$callbackData(inputMethods);
    chrome.runtime.getBackgroundPage.$callbackData(undefined);
    chrome.runtime.sendMessage({type: Type.CONNECT});
    chrome.runtime.sendMessage({type: Type.VISIBILITY_CHANGE,
        visibility: true});
    // Perform tests.
    mockControl.$replayAll();
    adapter.initialize();
    // Verify results.
    assertEquals(isA11y, adapter.isA11yMode);
    assertEquals(showGlobe, adapter.showGlobeKey);
    mockControl.$verifyAll();
  };

  var defaultSettings = {
    layout: 'qwerty',
    a11ymode: false,
    experiemntal: false
  };
  var a11ySettings = {
    layout: 'system-qwerty',
    a11ymode: true,
    experiemntal: false
  };
  var emptyImeList = [];
  var defaultImeList = [
    {id: 'dummy1', name: 'US Keyboard', indicator: 'US'}
  ];
  var multiImeList = [
    {id: 'dummy1', name: 'US Keyboard', indicator: 'US'},
    {id: 'dummy2', name: 'US Dvorak Keyboard', indicator: 'DV'}
  ];

  settingsTest(defaultSettings, emptyImeList, false, false);
  settingsTest(a11ySettings, emptyImeList, true, false);
  settingsTest(defaultSettings, defaultImeList, false, true);
  settingsTest(a11ySettings, defaultImeList, true, true);
  settingsTest(defaultSettings, multiImeList, false, true);
  settingsTest(a11ySettings, multiImeList, true, true);
}

