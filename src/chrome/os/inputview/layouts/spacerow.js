goog.provide('i18n.input.chrome.inputview.layouts.SpaceRow');

goog.require('i18n.input.chrome.inputview.ConditionName');
goog.require('i18n.input.chrome.inputview.layouts.util');


/**
 * Creates the spaceKey row.
 *
 * @return {!Object} The spaceKey row.
 */
i18n.input.chrome.inputview.layouts.SpaceRow.create = function() {
  var globeKey = i18n.input.chrome.inputview.layouts.util.createKey({
    'condition': i18n.input.chrome.inputview.ConditionName.SHOW_GLOBE_OR_SYMBOL,
    'widthInWeight': 1
  });
  var menuKey = i18n.input.chrome.inputview.layouts.util.createKey({
    'condition': i18n.input.chrome.inputview.ConditionName.SHOW_MENU,
    'widthInWeight': 1
  });
  var ctrlKey = i18n.input.chrome.inputview.layouts.util.createKey({
    'widthInWeight': 1
  });
  var altKey = i18n.input.chrome.inputview.layouts.util.createKey({
    'widthInWeight': 1
  });
  var spaceKey = i18n.input.chrome.inputview.layouts.util.createKey({
    'widthInWeight': 6.75
  });
  var altGrKey = i18n.input.chrome.inputview.layouts.util.createKey({
    'widthInWeight': 1.25,
    'condition': i18n.input.chrome.inputview.ConditionName.
        SHOW_ALTGR
  });
  // If globeKey or altGrKey is not shown, give its weight to space key.
  globeKey['spec']['giveWeightTo'] = spaceKey['spec']['id'];
  menuKey['spec']['giveWeightTo'] = spaceKey['spec']['id'];
  altGrKey['spec']['giveWeightTo'] = spaceKey['spec']['id'];

  var leftKey = i18n.input.chrome.inputview.layouts.util.createKey({
    'widthInWeight': 1
  });
  var rightKey = i18n.input.chrome.inputview.layouts.util.createKey({
    'widthInWeight': 1
  });
  var hideKeyboardKey = i18n.input.chrome.inputview.layouts.util.createKey({
    'widthInWeight': 1
  });
  var spaceKeyRow = i18n.input.chrome.inputview.layouts.util.
      createLinearLayout({
        'id': 'spaceKeyrow',
        'children': [globeKey, menuKey, ctrlKey, altKey, spaceKey,
              altGrKey, leftKey, rightKey, hideKeyboardKey]
      });
  return spaceKeyRow;
};

