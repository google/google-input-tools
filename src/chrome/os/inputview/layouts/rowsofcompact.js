goog.provide('i18n.input.chrome.inputview.layouts.RowsOfCompact');
goog.provide('i18n.input.chrome.inputview.layouts.RowsOfCompactAzerty');
goog.provide('i18n.input.chrome.inputview.layouts.RowsOfCompactNordic');

goog.require('i18n.input.chrome.inputview.layouts.util');


/**
 * Creates the top three rows for compact qwerty keyboard.
 *
 * @return {!Array.<!Object>} The rows.
 */
i18n.input.chrome.inputview.layouts.RowsOfCompact.create = function() {
  var baseKeySpec = {
    'widthInWeight': 1,
    'heightInWeight': 1
  };

  // Row1
  var keySequenceOf10 = i18n.input.chrome.inputview.layouts.util.
      createKeySequence(baseKeySpec, 10);
  var backspaceKey = i18n.input.chrome.inputview.layouts.util.createKey({
    'widthInWeight': 1.2
  });
  var row1 = i18n.input.chrome.inputview.layouts.util.createLinearLayout({
    'id': 'row1',
    'children': [keySequenceOf10, backspaceKey]
  });

  // Row2
  // How to add padding
  var leftKeyWithPadding = i18n.input.chrome.inputview.layouts.util.createKey({
      'widthInWeight': 1.5
  });
  var keySequenceOf8 = i18n.input.chrome.inputview.layouts.util.
      createKeySequence(baseKeySpec, 8);
  var enterKey = i18n.input.chrome.inputview.layouts.util.createKey({
    'widthInWeight': 1.7
  });
  var row2 = i18n.input.chrome.inputview.layouts.util.createLinearLayout({
    'id': 'row2',
    'children': [leftKeyWithPadding, keySequenceOf8, enterKey]
  });

  // Row3
  var shiftLeftKey = i18n.input.chrome.inputview.layouts.util.createKey({
    'widthInWeight': 1.1
  });
  var keySequenceOf9 = i18n.input.chrome.inputview.layouts.util.
      createKeySequence(baseKeySpec, 9);
  var shiftRightKey = i18n.input.chrome.inputview.layouts.util.createKey({
    'widthInWeight': 1.1
  });
  var row3 = i18n.input.chrome.inputview.layouts.util.createLinearLayout({
    'id': 'row3',
    'children': [shiftLeftKey, keySequenceOf9, shiftRightKey]
  });

  return [row1, row2, row3];
};

/**
 * Creates the top three rows for compact azerty keyboard.
 *
 * @return {!Array.<!Object>} The rows.
 */
i18n.input.chrome.inputview.layouts.RowsOfCompactAzerty.create = function() {
  var baseKeySpec = {
    'widthInWeight': 1,
    'heightInWeight': 1
  };

  // Row1
  var keySequenceOf10 = i18n.input.chrome.inputview.layouts.util.
      createKeySequence(baseKeySpec, 10);
  var backspaceKey = i18n.input.chrome.inputview.layouts.util.createKey({
    'widthInWeight': 1.2
  });
  var row1 = i18n.input.chrome.inputview.layouts.util.createLinearLayout({
    'id': 'row1',
    'children': [keySequenceOf10, backspaceKey]
  });

  // Row2
  keySequenceOf10 = i18n.input.chrome.inputview.layouts.util.
      createKeySequence(baseKeySpec, 10);
  var enterKey = i18n.input.chrome.inputview.layouts.util.createKey({
    'widthInWeight': 1.2
  });
  var row2 = i18n.input.chrome.inputview.layouts.util.createLinearLayout({
    'id': 'row2',
    'children': [keySequenceOf10, enterKey]
  });

  // Row3
  var shiftLeftKey = i18n.input.chrome.inputview.layouts.util.createKey({
    'widthInWeight': 1.1
  });
  var keySequenceOf9 = i18n.input.chrome.inputview.layouts.util.
      createKeySequence(baseKeySpec, 9);
  var shiftRightKey = i18n.input.chrome.inputview.layouts.util.createKey({
    'widthInWeight': 1.1
  });
  var row3 = i18n.input.chrome.inputview.layouts.util.createLinearLayout({
    'id': 'row3',
    'children': [shiftLeftKey, keySequenceOf9, shiftRightKey]
  });

  return [row1, row2, row3];
};

/**
 * Creates the top three rows for compact nordic keyboard.
 *
 * @return {!Array.<!Object>} The rows.
 */
i18n.input.chrome.inputview.layouts.RowsOfCompactNordic.create = function() {
  var baseKeySpec = {
    'widthInWeight': 1,
    'heightInWeight': 1
  };

  // Row1
  var keySequenceOf11 = i18n.input.chrome.inputview.layouts.util.
      createKeySequence(baseKeySpec, 11);
  var backspaceKey = i18n.input.chrome.inputview.layouts.util.createKey({
    'widthInWeight': 1.2
  });
  var row1 = i18n.input.chrome.inputview.layouts.util.createLinearLayout({
    'id': 'row1',
    'children': [keySequenceOf11, backspaceKey]
  });

  // Row2
  keySequenceOf11 = i18n.input.chrome.inputview.layouts.util.
      createKeySequence(baseKeySpec, 11);
  var enterKey = i18n.input.chrome.inputview.layouts.util.createKey({
    'widthInWeight': 1.2
  });
  var row2 = i18n.input.chrome.inputview.layouts.util.createLinearLayout({
    'id': 'row2',
    'children': [keySequenceOf11, enterKey]
  });

  // Row3
  var shiftLeftKey = i18n.input.chrome.inputview.layouts.util.createKey({
    'widthInWeight': 1.1
  });
  var leftKeyWithPadding = i18n.input.chrome.inputview.layouts.util.createKey({
      'widthInWeight': 1.5
  });
  var keySequenceOf7 = i18n.input.chrome.inputview.layouts.util.
      createKeySequence(baseKeySpec, 7);
  var rightKeyWithPadding = i18n.input.chrome.inputview.layouts.util.createKey({
      'widthInWeight': 1.5
  });
  var shiftRightKey = i18n.input.chrome.inputview.layouts.util.createKey({
    'widthInWeight': 1.1
  });
  var row3 = i18n.input.chrome.inputview.layouts.util.createLinearLayout({
    'id': 'row3',
    'children': [shiftLeftKey, leftKeyWithPadding, keySequenceOf7,
        rightKeyWithPadding, shiftRightKey]
  });
  return [row1, row2, row3];
};
