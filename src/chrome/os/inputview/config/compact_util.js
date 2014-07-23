goog.provide('i18n.input.chrome.inputview.content.compact.util');

goog.require('i18n.input.chrome.inputview.Css');
goog.require('i18n.input.chrome.inputview.elements.ElementType');

goog.scope(function() {
var util = i18n.input.chrome.inputview.content.compact.util;


/**
 * The compact layout keyset type.
 *
 * @enum {string}
 */
util.CompactKeysetSpec = {
  ID: 'id',
  LAYOUT: 'layout',
  DATA: 'data'
};


/**
 * @desc The name of the layout providing numbers from 0-9 and common
 *     symbols.
 */
util.MSG_NUMBER_AND_SYMBOL =
    goog.getMsg('number and symbol layout');


/**
 * @desc The name of the layout providing more symbols.
 */
util.MSG_MORE_SYMBOLS =
    goog.getMsg('more symbols layout');


/**
 * @desc The name of the main layout.
 */
util.MSG_MAIN_LAYOUT =
    goog.getMsg('main layout');


/**
 * Creates the compact key data.
 *
 * @param {!Object} keysetSpec The keyset spec.
 * @param {string} viewIdPrefix The prefix of the view.
 * @param {string} keyIdPrefix The prefix of the key.
 * @return {!Object} The key data.
 */
util.createCompactData = function(keysetSpec, viewIdPrefix, keyIdPrefix) {
  var keyList = [];
  var mapping = {};
  var keysetSpecNode = util.CompactKeysetSpec;
  for (var i = 0; i < keysetSpec[keysetSpecNode.DATA].length; i++) {
    var keySpec = keysetSpec[keysetSpecNode.DATA][i];
    if (keySpec ==
        i18n.input.chrome.inputview.content.constants.NonLetterKeys.MENU) {
      keySpec['toKeyset'] = keysetSpec[keysetSpecNode.ID].split('.')[0];
    }
    var id = keySpec['id'] ? keySpec['id'] : keyIdPrefix + i;
    var key = util.createCompactKey(id, keySpec);
    keyList.push(key);
    mapping[key['spec']['id']] = viewIdPrefix + i;
  }

  return {
    'keyList': keyList,
    'mapping': mapping,
    'layout': keysetSpec[keysetSpecNode.LAYOUT]
  };
};


/**
 * Creates a key in the compact keyboard.
 *
 * @param {string} id The id.
 * @param {!Object} keySpec The specification.
 * @return {!Object} The compact key.
 */
util.createCompactKey = function(id, keySpec) {
  var spec = keySpec;
  spec['id'] = id;
  if (!spec['type'])
    spec['type'] = i18n.input.chrome.inputview.elements.ElementType.COMPACT_KEY;

  var newSpec = {};
  for (var key in spec) {
    newSpec[key] = spec[key];
  }

  return {
    'spec': newSpec
  };
};


/**
 * Customize the switcher keys in key characters.
 *
 * @param {!Array.<!Object>} keyCharacters The key characters.
 * @param {!Array.<!Object>} switcherKeys The customized switcher keys.
 */
util.customizeSwitchers = function(keyCharacters, switcherKeys) {
  var j = 0;
  for (var i = 0; i < keyCharacters.length; i++) {
    if (keyCharacters[i] ==
        i18n.input.chrome.inputview.content.constants.NonLetterKeys.SWITCHER) {
      if (j >= switcherKeys.length) {
        console.error('The number of switcher key spec is less than' +
            ' the number of switcher keys in the keyset.');
        return;
      }
      var newSpec = switcherKeys[j++];
      for (var key in keyCharacters[i]) {
        newSpec[key] = keyCharacters[i][key];
      }
      keyCharacters[i] = newSpec;
    }
  }
  if (j < switcherKeys.length - 1) {
    console.error('The number of switcher key spec is more than' +
        ' the number of switcher keys in the keyset.');
  }
};


/**
 * Generates letter, symbol and more compact keysets and load them.
 *
 * @param {!Object} letterKeysetSpec The spec of letter keyset.
 * @param {!Object} symbolKeysetSpec The spec of symbol keyset.
 * @param {!Object} moreKeysetSpec The spec of more keyset.
 * @param {!function(!Object): void} onLoaded The function to call once a keyset
 *     data is ready.
 */
util.generateCompactKeyboard =
    function(letterKeysetSpec, symbolKeysetSpec, moreKeysetSpec, onLoaded) {
  // Creates compacty qwerty keyset.
  var keysetSpecNode = util.CompactKeysetSpec;
  util.customizeSwitchers(
      letterKeysetSpec[keysetSpecNode.DATA],
      [{ 'name': '?123',
         'toKeyset': symbolKeysetSpec[keysetSpecNode.ID],
         'toKeysetName': util.MSG_NUMBER_AND_SYMBOL
       }]);

  var data = util.createCompactData(
      letterKeysetSpec, 'compactkbd-k-', 'compactkbd-k-key-');
  data['id'] = letterKeysetSpec[keysetSpecNode.ID];
  data['showMenuKey'] = true;
  onLoaded(data);

  // Creates compacty symbol keyset.
  util.customizeSwitchers(
      symbolKeysetSpec[keysetSpecNode.DATA],
      [{ 'name': '~[<',
         'toKeyset': moreKeysetSpec[keysetSpecNode.ID],
         'toKeysetName': util.MSG_MORE_SYMBOLS
       },
       { 'name': '~[<',
         'toKeyset': moreKeysetSpec[keysetSpecNode.ID],
         'toKeysetName': util.MSG_MORE_SYMBOLS
       },
       { 'name': 'abc',
         'toKeyset': letterKeysetSpec[keysetSpecNode.ID],
         'toKeysetName': util.MSG_MAIN_LAYOUT
       }]);
  data = util.createCompactData(
      symbolKeysetSpec, 'compactkbd-k-', 'compactkbd-k-key-');
  data['id'] = symbolKeysetSpec[keysetSpecNode.ID];
  data['showMenuKey'] = false;
  onLoaded(data);

  // Creates compact more keyset.
  util.customizeSwitchers(
      moreKeysetSpec[keysetSpecNode.DATA],
      [{ 'name': '?123',
         'toKeyset': symbolKeysetSpec[keysetSpecNode.ID],
         'toKeysetName': util.MSG_NUMBER_AND_SYMBOL
       },
       { 'name': '?123',
         'toKeyset': symbolKeysetSpec[keysetSpecNode.ID],
         'toKeysetName': util.MSG_NUMBER_AND_SYMBOL
       },
       { 'name': 'abc',
         'toKeyset': letterKeysetSpec[keysetSpecNode.ID],
         'toKeysetName': util.MSG_MAIN_LAYOUT
       }]);
  data = util.createCompactData(moreKeysetSpec, 'compactkbd-k-',
      'compactkbd-k-key-');
  data['id'] = moreKeysetSpec[keysetSpecNode.ID];
  data['showMenuKey'] = false;
  onLoaded(data);
};

});  // goog.scope
