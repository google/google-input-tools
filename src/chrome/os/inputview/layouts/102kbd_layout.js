
goog.require('i18n.input.chrome.inputview.layouts.RowsOf102');
goog.require('i18n.input.chrome.inputview.layouts.SpaceRow');
goog.require('i18n.input.chrome.inputview.layouts.util');


(function() {
  i18n.input.chrome.inputview.layouts.util.keyIdPrefix = '102kbd-k-';


  var topFourRows = i18n.input.chrome.inputview.layouts.RowsOf102.create();
  var spaceRow = i18n.input.chrome.inputview.layouts.SpaceRow.create();

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
    'layoutID': '102kbd',
    'widthInWeight': 15,
    'children': [keyboardContainer]
  };

  google.ime.chrome.inputview.onLayoutLoaded(data);

}) ();
