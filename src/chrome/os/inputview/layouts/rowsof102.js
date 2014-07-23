goog.provide('i18n.input.chrome.inputview.layouts.RowsOf102');

goog.require('i18n.input.chrome.inputview.layouts.util');


/**
 * Creates the top four rows for 102 keyboard.
 *
 * @return {!Array.<!Object>} The rows.
 */
i18n.input.chrome.inputview.layouts.RowsOf102.create = function() {
  var baseKeySpec = {
    'widthInWeight': 1,
    'heightInWeight': 1
  };

  // Row1
  var keySequenceOf13 = i18n.input.chrome.inputview.layouts.util.
      createKeySequence(baseKeySpec, 13);
  var backspaceKey = i18n.input.chrome.inputview.layouts.util.createKey({
    'widthInWeight': 2
  });
  var row1 = i18n.input.chrome.inputview.layouts.util.createLinearLayout({
    'id': 'row1',
    'children': [keySequenceOf13, backspaceKey]
  });


  // Row2 and row3

  // First linear layout at the left of the enter.
  var tabKey = i18n.input.chrome.inputview.layouts.util.createKey({
    'widthInWeight': 1.5
  });
  var keySequenceOf11 = i18n.input.chrome.inputview.layouts.util.
      createKeySequence(baseKeySpec, 11);
  var slashKey = i18n.input.chrome.inputview.layouts.util.createKey({
    'widthInWeight': 1.25
  });
  var row2 = i18n.input.chrome.inputview.layouts.util.createLinearLayout({
    'id': 'row2',
    'children': [tabKey, keySequenceOf11, slashKey]
  });

  // Second linear layout at the right of the enter.
  var capslockKey = i18n.input.chrome.inputview.layouts.util.createKey({
    'widthInWeight': 1.75
  });
  var keySequenceOf12 = i18n.input.chrome.inputview.layouts.util.
      createKeySequence(baseKeySpec, 12);
  var row3 = i18n.input.chrome.inputview.layouts.util.createLinearLayout({
    'id': 'row3',
    'children': [capslockKey, keySequenceOf12]
  });

  // Vertical layout contains the two rows at the left of the enter.
  var vLayout = i18n.input.chrome.inputview.layouts.util.createVerticalLayout({
    'id': 'row2-3-left',
    'children': [row2, row3]
  });

  // Vertical layout contains enter key.
  var enterKey = i18n.input.chrome.inputview.layouts.util.createKey({
    'widthInWeight': 1.25,
    'heightInWeight': 2
  });
  var enterLayout = i18n.input.chrome.inputview.layouts.util.
      createVerticalLayout({
        'id': 'row2-3-right',
        'children': [enterKey]
      });

  // Linear layout contains the two vertical layout.
  var row2and3 = i18n.input.chrome.inputview.layouts.util.createLinearLayout({
    'id': 'row2-3',
    'children': [vLayout, enterLayout]
  });

  // Row4
  var shiftLeft = i18n.input.chrome.inputview.layouts.util.createKey({
    'widthInWeight': 1.25
  });
  keySequenceOf11 = i18n.input.chrome.inputview.layouts.util.createKeySequence(
      baseKeySpec, 11);
  var shiftRight = i18n.input.chrome.inputview.layouts.util.createKey({
    'widthInWeight': 2.75
  });
  var row4 = i18n.input.chrome.inputview.layouts.util.createLinearLayout({
    'id': 'row4',
    'children': [shiftLeft, keySequenceOf11, shiftRight]
  });

  return [row1, row2and3, row4];
};
