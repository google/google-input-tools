goog.require('i18n.input.chrome.inputview.ConditionName');
goog.require('i18n.input.chrome.inputview.layouts.RowsOfJP');
goog.require('i18n.input.chrome.inputview.layouts.util');


(function() {
  var ConditionName = i18n.input.chrome.inputview.ConditionName;
  var util = i18n.input.chrome.inputview.layouts.util;
  i18n.input.chrome.inputview.layouts.util.keyIdPrefix = 'jpkbd-k-';

  var topFourRows = i18n.input.chrome.inputview.layouts.RowsOfJP.create();

  // Creates the space row.
  var globeKey = util.createKey({
    'condition': ConditionName.SHOW_GLOBE_OR_SYMBOL,
    'widthInWeight': 1
  });
  var menuKey = util.createKey({
    'condition': ConditionName.SHOW_MENU,
    'widthInWeight': 1
  });
  var ctrlKey = util.createKey({
    'widthInWeight': 1
  });
  var altKey = util.createKey({
    'widthInWeight': 1
  });

  var leftIMEKey = util.createKey({'widthInWeight': 1});
  var spaceKey = util.createKey({'widthInWeight': 6});
  var rightIMEKey = util.createKey({'widthInWeight': 1});

  // If globeKey or altGrKey is not shown, give its weight to space key.
  globeKey['spec']['giveWeightTo'] = spaceKey['spec']['id'];
  menuKey['spec']['giveWeightTo'] = spaceKey['spec']['id'];

  var leftKey = util.createKey({
    'widthInWeight': 1
  });
  var rightKey = util.createKey({
    'widthInWeight': 1
  });
  var hideKeyboardKey = util.createKey({
    'widthInWeight': 1
  });

  var keys = [
    globeKey,
    menuKey,
    ctrlKey,
    altKey,
    leftIMEKey,
    spaceKey,
    rightIMEKey,
    leftKey,
    rightKey,
    hideKeyboardKey
  ];

  var spaceRow = util.createLinearLayout({
    'id': 'spaceKeyrow',
    'children': keys
  });


  // Keyboard view.
  var keyboardView = i18n.input.chrome.inputview.layouts.util.createLayoutView({
    'id': 'keyboardView',
    'children': [topFourRows, spaceRow],
    'widthPercent': 100,
    'heightPercent': 100
  });

  var keyboardContainer = i18n.input.chrome.inputview.layouts.util.
      createLinearLayout({
        'id': 'keyboardContainer',
        'children': [keyboardView]
      });

  var data = {
    'layoutID': 'jpkbd',
    'widthInWeight': 15,
    'children': [keyboardContainer]
  };

  google.ime.chrome.inputview.onLayoutLoaded(data);

}) ();
