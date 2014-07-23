goog.require('i18n.input.chrome.inputview.layouts.CompactSpaceRow');
goog.require('i18n.input.chrome.inputview.layouts.RowsOfCompactAzerty');
goog.require('i18n.input.chrome.inputview.layouts.util');


(function() {
  i18n.input.chrome.inputview.layouts.util.keyIdPrefix = 'compactkbd-k-';

  var topThreeRows =
      i18n.input.chrome.inputview.layouts.RowsOfCompactAzerty.create();
  var spaceRow =
      i18n.input.chrome.inputview.layouts.CompactSpaceRow.create(false);

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
    'layoutID': 'compactkbd-azerty',
    'widthInWeight': 15,
    'children': [keyboardContainer]
  };

  google.ime.chrome.inputview.onLayoutLoaded(data);

}) ();
