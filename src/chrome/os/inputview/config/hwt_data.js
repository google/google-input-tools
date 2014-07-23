goog.require('i18n.input.chrome.inputview.Css');
goog.require('i18n.input.chrome.inputview.SpecNodeName');
goog.require('i18n.input.chrome.inputview.content.util');
goog.require('i18n.input.chrome.inputview.elements.ElementType');

(function() {
  var util = i18n.input.chrome.inputview.content.util;
  var ElementType = i18n.input.chrome.inputview.elements.ElementType;
  var SpecNodeName = i18n.input.chrome.inputview.SpecNodeName;
  var Css = i18n.input.chrome.inputview.Css;

  var viewIdPrefix = 'handwriting-k-';

  var spec = {};
  spec[SpecNodeName.ID] = 'Comma';
  spec[SpecNodeName.TYPE] = ElementType.CHARACTER_KEY;
  spec[SpecNodeName.CHARACTERS] = [','];
  var commaKey = util.createKey(spec);

  spec = {};
  spec[SpecNodeName.ID] = 'Period';
  spec[SpecNodeName.TYPE] = ElementType.CHARACTER_KEY;
  spec[SpecNodeName.CHARACTERS] = ['.'];
  var periodKey = util.createKey(spec);

  spec = {};
  spec[SpecNodeName.TEXT] = '';
  spec[SpecNodeName.ICON_CSS_CLASS] = Css.SPACE_ICON;
  spec[SpecNodeName.TYPE] = ElementType.SPACE_KEY;
  spec[SpecNodeName.ID] = 'Space';
  var spaceKey = i18n.input.chrome.inputview.content.util.createKey(spec);

  var keyList = [
    commaKey,
    periodKey,
    spaceKey,
    util.createBackspaceKey(),
    util.createEnterKey(),
    util.createHideKeyboardKey()
  ];

  var mapping = {};
  for (var i = 0; i < keyList.length; i++) {
    var key = keyList[i];
    mapping[key['spec'][SpecNodeName.ID]] = viewIdPrefix + i;
  }

  var result = [];
  result[SpecNodeName.KEY_LIST] = keyList;
  result[SpecNodeName.MAPPING] = mapping;
  result[SpecNodeName.LAYOUT] = 'handwriting';
  result[SpecNodeName.HAS_ALTGR_KEY] = false;
  result[SpecNodeName.HAS_COMPACT_KEYBOARD] = false;
  result['id'] = 'hwt';
  google.ime.chrome.inputview.onConfigLoaded(result);
}) ();
