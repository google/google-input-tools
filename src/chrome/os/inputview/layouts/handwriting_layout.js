goog.require('i18n.input.chrome.inputview.layouts.SpaceRow');
goog.require('i18n.input.chrome.inputview.layouts.util');


(function() {
  var util = i18n.input.chrome.inputview.layouts.util;
  util.keyIdPrefix = 'handwriting-k-';

  var verticalRows = [];
  var baseKeySpec = {
    'widthInWeight': 1,
    'heightInWeight': 1
  };
  for (var i = 0; i < 3; i++) {
    verticalRows.push(util.createKey(baseKeySpec));
  }
  var leftSideColumn = util.createVerticalLayout({
    'id': 'leftSideColumn',
    'children': verticalRows
  });

  verticalRows = [];
  for (var i = 0; i < 3; i++) {
    verticalRows.push(util.createKey(baseKeySpec));
  }
  var rightSideColumn = util.createVerticalLayout({
    'id': 'rightSideColumn',
    'children': verticalRows
  });

  var spec = {
    'id': 'canvasView',
    'widthInWeight': 11.2,
    'heightInWeight': 4
  };

  var canvasView = util.createCanvasView(spec);
  var panelView = util.createHandwritingLayout({
    'id': 'panelView',
    'children': [canvasView, leftSideColumn, rightSideColumn]
  });

  // Keyboard view.
  var keyboardView = util.createLayoutView({
    'id': 'keyboardView',
    'children': [panelView],
    'widthPercent': 100,
    'heightPercent': 100
  });


  var keyboardContainer = util.createLinearLayout({
    'id': 'keyboardContainer',
    'children': [keyboardView]
  });

  var data = {
    'layoutID': 'handwriting',
    'heightPercentOfWidth': 0.275,
    'minimumHeight': 350,
    'fullHeightInWeight': 5.6,
    'children': [keyboardContainer]
  };

  google.ime.chrome.inputview.onLayoutLoaded(data);

}) ();
