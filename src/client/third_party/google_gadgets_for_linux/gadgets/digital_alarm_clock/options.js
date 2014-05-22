/*
  Copyright 2008 Google Inc.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

// global vars
var gModeAMPM = null;
var gAlarmAM = null;
var gAlarmDefault = null;
var gRevertKeypress = false;
var gSavedEditValue = "";

/**
 * Method called when options are opened
 */
function _onOpen() {
  oString1.innerText = ALARM_SET;
  oString2.innerText = DEFAULT_ALARM;
  oString3.innerText = USE_FILE;
  oString5.innerText = CITY + ":";
  oString6.innerText = DISPLAY_WORLD;
  oString7.innerText = BROWSE;
  oStringAM.innerText = AM;
  oStringPM.innerText = PM;

  oCheckbox1.value = options.getValue(OPT_ALARM);
  gAlarmDefault = options.getValue(OPT_DEFAULT_RINGER);
  oFilePath.value = options.getValue(OPT_FILE_PATH);
  oCityName.value = options.getValue(OPT_ADDRESS);

  gModeAMPM = options.getValue(OPT_MODE) == MODE_AMPM ? true : false;
  oAMPM.visible = gModeAMPM;

  var hours = options.getValue(OPT_HOURS);
  gAlarmAM = hours < 12 ? true : false;

  if (gModeAMPM && hours > 12) {
    hours = hours - 12;
  }

  if (gAlarmAM && gModeAMPM && hours == 0) {
    hours = 12;
  }

  oHours.value = timeString(hours);
  oMinutes.value = timeString(options.getValue(OPT_MINUTES));

  if (gAlarmAM) {
    oAM.src = "images\\radiobutton_chk.png";
    oPM.src = "images\\radiobutton.png";
  } else {
    oAM.src = "images\\radiobutton.png";
    oPM.src = "images\\radiobutton_chk.png";
  }

  if (gAlarmDefault) {
    oCheckbox2.src = "images\\radiobutton_chk.png";
    oCheckbox3.src = "images\\radiobutton.png";
  } else {
    oCheckbox2.src = "images\\radiobutton.png";
    oCheckbox3.src = "images\\radiobutton_chk.png";
  }

  if (oCheckbox1.value) {
    oAlarmOptions.visible = true;
    if (gAlarmDefault) {
      oFileOptions.visible = false;
    } else {
      oFileOptions.visible = true;
    }
  } else {
    oAlarmOptions.visible = false;
  }
}

/**
 * Method called when user hits "ok" button
 */
function _onOk() {
  // check validity of file path
  var path = oFilePath.value;
  if (oCheckbox1.value && !gAlarmDefault) {
    var fso;
    fso = framework.system.filesystem;
    if (!fso.FileExists(path)) {
      gAlarmDefault = true;
    }
  }

  debug.trace(oHours.value + ":" + oMinutes.value);
  var hours = -1;
  var minutes = -1;

  try {
    hours = Math.floor(parseFloat(oHours.value));
  } catch(e) {
    hours = -1;
  }

  try {
    minutes = Math.floor(parseFloat(oMinutes.value));
  } catch(e) {
    minutes = -1;
  }

  // Convert hours as necesary for am/pm business
  if (gModeAMPM && !gAlarmAM && hours != 12) {
    hours += 12;
  }

  if (gModeAMPM && gAlarmAM && hours == 12) {
    hours -= 12;
  }

  // Checks for bad hours and minutes inputs.
  // This should never happen, but handle just in case
  if (!(hours >= 0 && hours < 25)) {
    hours = 12;
  }

  if (!(minutes >= 0 && minutes <= 59)) {
    minutes = 0;
  }

  options.putValue(OPT_ALARM, oCheckbox1.value);
  options.putValue(OPT_DEFAULT_RINGER, gAlarmDefault);
  options.putValue(OPT_FILE_PATH, oFilePath.value);
  options.putValue(OPT_CITY, oCityName.value);
  options.putValue(OPT_HOURS, hours);
  options.putValue(OPT_MINUTES, minutes);
}

/**
 * Called when user hits cancel button
 */
function _onCancel() {
}

/**
 * Called when alarm checkbox changes
 */
function _alarmChk() {
  if (oCheckbox1.value) {
    oAlarmOptions.visible = true;
    if (gAlarmDefault) {
      oFileOptions.visible = false;
    } else {
      oFileOptions.visible = true;
    }
  } else {
    oAlarmOptions.visible = false;
  }
}

/**
 * Handles default alarm checkbox click
 */
function _defaultAlarmChk() {
  debug.trace("check default");
  gAlarmDefault = true;
  oCheckbox2.src = "images\\radiobutton_chk.png";
  oCheckbox3.src = "images\\radiobutton.png";
  if (oCheckbox1.value) {
    oAlarmOptions.visible = true;
    if (gAlarmDefault) {
      oFileOptions.visible = false;
    } else {
      oFileOptions.visible = true;
    }
  } else {
    oAlarmOptions.visible = false;
  }
}

/**
 * Handles use file checkbox click
 */
function _useFileChk() {
  debug.trace("check file");
  gAlarmDefault = false;
  oCheckbox2.src = "images\\radiobutton.png";
  oCheckbox3.src = "images\\radiobutton_chk.png";
  if (oCheckbox1.value) {
    oAlarmOptions.visible = true;
    if (gAlarmDefault) {
      oFileOptions.visible = false;
    } else {
      oFileOptions.visible = true;
    }
  } else {
    oAlarmOptions.visible = false;
  }
}

/**
 * Handles AM checkbox click
 */
function _amChk() {
  debug.trace("check am");
  gAlarmAM = true;
  oAM.src = "images\\radiobutton_chk.png";
  oPM.src = "images\\radiobutton.png";
}

/**
 * Handles PM checkbox click
 */
function _pmChk() {
  debug.trace("check pm");
  gAlarmAM = false;
  oAM.src = "images\\radiobutton.png";
  oPM.src = "images\\radiobutton_chk.png";
}

/**
 * checkbox change
 */
function chkchange() {
  debug.trace("am: " + oAM.value + " pm: " + oPM.value + " def: " + oCheckbox2.value + " fil: " + oCheckbox3.value);
}

/**
 * Turns an int into a 2 digit time string
 * @param {Number} n is a time field
 */
function timeString(n) {
  if (n < 10) {
    return "0" + n;
  } else {
    return "" + n;
  }
}

/**
 * Open a browse dialog
 */
function _doBrowse() {
  debug.trace("launch browse");
  var file = BrowseForFile("Music Files|*.mp3;*.wma;*.wav");
  oFilePath.value = file;
}

/**
 * Handles event that gives keycode of what's about to be entered into time box
 * @param {String} id 'h' for hours and 'm' for minutes tells which box
 */
function _keyPress(id) {
  var elem;
  if (id == 'h') {
    elem = oHours;
  } else {
    elem = oMinutes;
  }

  gSavedEditValue = elem.value;

  if (event.keycode == 9) {
    gRevertKeypress = true;
    if (id == 'h') {
      // give focus to minutes
      gSavedEditValue = oMinutes.value;
      oMinutes.focus();
    }
  } else if (event.keycode == 35 ||
             event.keycode == 36 ||
             event.keycode == 46 ||
             event.keycode == 8 ||
             (event.keycode > 47 && event.keycode < 58) ||
             (event.keycode > 95 && event.keycode < 106)) {
    gRevertKeypress = false;
  } else {
    gRevertKeypress = true;
  }
}

/**
 * Handles event that fires after change to time box is made
 * @param {String} id 'h' for hours and 'm' for minutes
 */
function _valueChange(id) {
  var elem;
  var numValue;

  if (id == 'h') {
    elem = oHours;
  } else {
    elem = oMinutes;
  }

  if (gRevertKeypress || elem.value.length > 2) {
    elem.value = gSavedEditValue;
  }

  try {
    numValue = parseInt(elem.value);
    if (id == 'h') {
      if (gModeAMPM && numValue > 12) {
        gRevertKeypress = true;
      } else if (numValue > 23) {
        gRevertKeypress = true;
      }
    } else if (id == 'm') {
      if (numValue > 59) {
        gRevertKeypress = true;
      }
    }
  } catch(e) {
    gRevertKeypress = true;
  }

  if (gRevertKeypress) {
    elem.value = gSavedEditValue;
  }

  gRevertKeypress = false;
}

/**
 * Handles key down for path edit box
 */
function _pathKeyDown() {
  // Save value prior to change
  gRevertKeypress = true;
  gSavedEditValue = oFilePath.value;
}

/**
 * Handles value change for path edit box
 */
function _pathChange() {
  // don't allow user to edit value.
  // revert changes while still letting user navigate box
  if (oFilePath.value != gSavedEditValue && gRevertKeypress) {
    oFilePath.value = gSavedEditValue;
  }

  gRevertKeypress = false;
}
