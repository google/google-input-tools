goog.require('i18n.input.chrome.inputview.layouts.CompactSpaceRow');
goog.require('i18n.input.chrome.inputview.layouts.RowsOfCompactNordic');
goog.require('i18n.input.chrome.inputview.layouts.util');


(function() {
  i18n.input.chrome.inputview.layouts.util.keyIdPrefix = 'compactkbd-k-';

  var topThreeRows =
      i18n.input.chrome.inputview.layouts.RowsOfCompactNordic.create();
  var spaceRow =
      i18n.input.chrome.inputview.layouts.CompactSpaceRow.create(true);

  // Keyboard view.
  var keyboardView = i18n.input.chrome.inputview.layouts.util.createLayoutView({
    'id': 'keyboardView',
    'children': [topThreeRows, spaceRow],
    'widthPercent': 100,
    'heightPercent': 100
  });

  var keyboardContainer = i18n.input.chrome.inputview.layouts.util.
      createLinearLayout({
        'id': 'keyboardContainer',
        'children': [keyboardView]
      });

  var data = {
    'layoutID': 'compactkbd-nordic',
    'widthInWeight': 15,
    'children': [keyboardContainer]
  };

  google.ime.chrome.inputview.onLayoutLoaded(data);

}) ();
