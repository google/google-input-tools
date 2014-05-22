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

// Gadget options
plugin.onAddCustomMenuItems = AddCustomMenuItems;
view.onoptionchanged = onOptionChanged;

// Global Constants
var kBlinkInterval = 1000;  // interval for flashing during alarm in ms
var kAlarmRepeatBuffer = 400; // time before repeating alarm sound (if not stopped)
var kSnoozeIntervalMs = 300000; // interval for snoozing alarm in ms
var kDefaultCoord = -74; // default longitude coordinate for display
var kBackupIntervalMs = 47000; // update clock every so often just in case...
var kMapOffset = 4; // map is off of GMT by a few pixels, add this to fix


// Set default options
options.putDefaultValue(OPT_MODE, MODE_AMPM);
options.putDefaultValue(OPT_ALARM, false);
options.putDefaultValue(OPT_DEFAULT_RINGER, true);
options.putDefaultValue(OPT_FILE_PATH, DEFAULT_PATH);
options.putDefaultValue(OPT_HOURS, 12);
options.putDefaultValue(OPT_MINUTES, 0);
options.putDefaultValue(OPT_DISPLAY, DISPLAY_WORLD); 
options.putDefaultValue(OPT_CITY, DEFAULT_CITY);
options.putDefaultValue(OPT_ADDRESS, DEFAULT_CITY);
options.putDefaultValue(OPT_COORD, kDefaultCoord);

// Global variables
var gUpdateToken = null;  // interval token for clock display update
var gTimeDisplay = new TimeDisplay(eDateTime);  // object for display
var gModeAMPM = false;  // true when mode is am/pm
var gAlarmSounding = false;  // true after alarm is triggered until off'ed
var gAlarmTriggered = false;
var gLightsToken = null;  // interval token for alarm lights animation
var gAlarmToken = null; // interval token for alarm sound
var gSnoozeToken = null;
var gAlarmClip = null;  // holds alarm audioclip when sounding
var gSnoozing = false;  // true after alarm is snoozed, before it's off'ed
var gLocation = options.getValue(OPT_CITY); // holds string for current displayed location
var gCollapsed = false; // true if gadget is collapsed

/**
 * Setup function on gadget load
 */
function _onOpen() {
  eAlarmOff.innerText = ALARM_OFF;
  eAlarmSnooze.innerText = ALARM_SNOOZE;
  changeMode(options.getValue(OPT_MODE));
  changeDisplay(options.getValue(OPT_DISPLAY));
  eCity.innerText = options.getValue(OPT_CITY);
  if (options.getValue(OPT_CITY) != "") {
    eMapTime.align = "right";
  }
  shiftMap(options.getValue(OPT_COORD));
  setInterval(backupUpdate, kBackupIntervalMs);
}

/**
 * Called when gadget is collapsed
 */
function _onMinimize()
{
  gCollapsed = true;
  gTimeDisplay.update();
}

/**
 * Called when gadget is uncollapsed
 */
function _onRestore()
{
  gCollapsed = false;
  view.caption = GADGET_NAME;
}

/**
 * Menu creation handler
 * @param {Object} menu object supplied by the callback
 */
function AddCustomMenuItems(menu) {
  var subMenu = menu.AddPopup(DISPLAY);
  subMenu.AddItem(DISPLAY_TIME,
      options.getValue(OPT_DISPLAY) == DISPLAY_TIME ? 
      gddMenuItemFlagChecked : 0, changeDisplay);
  subMenu.AddItem(DISPLAY_WORLD,
      options.getValue(OPT_DISPLAY) == DISPLAY_WORLD ? 
      gddMenuItemFlagChecked : 0, changeDisplay);

  subMenu = menu.AddPopup(MODE);
  subMenu.AddItem(MODE_AMPM,
      options.getValue(OPT_MODE) == MODE_AMPM ? gddMenuItemFlagChecked : 0, 
      changeMode);
  subMenu.AddItem(MODE_24H, 
      options.getValue(OPT_MODE) == MODE_24H ? gddMenuItemFlagChecked : 0, 
      changeMode);
}

/**
 * Checks for options changes that should change gadget behavior
 */
function onOptionChanged() {
  eBell.visible = options.getValue(OPT_ALARM);
  if (options.getValue(OPT_CITY) != gLocation &&
      options.getValue(OPT_CITY) != options.getValue(OPT_ADDRESS)) {
    debug.warning("fetching map");
    gLocation = options.getValue(OPT_CITY);
    queryMaps();
  }
}

/**
 * Handler for display mode option select
 * @param {String} display The string value of the option chosen
 */
function changeDisplay(display) {
  if (display == DISPLAY_TIME) {
    options.putValue(OPT_DISPLAY, DISPLAY_TIME);
    eDateTime.visible = true;
    eWorldMap.visible = false;
  } else {
    options.putValue(OPT_DISPLAY, DISPLAY_WORLD);
    eDateTime.visible = false;
    eWorldMap.visible = true;
  }
}

/**
 * Handler for mode option selection
 * @param {String} mode The string value of the option chosen
 */
function changeMode(mode) {
  if (mode == MODE_AMPM) {
    options.putValue(OPT_MODE, MODE_AMPM);
    gModeAMPM = true;
  } else {
    options.putValue(OPT_MODE, MODE_24H);
    gModeAMPM = false;
  }

  clearInterval(gUpdateToken);
  gTimeDisplay.update();  
}

/**
 * Handles gadget mouse over event
 */
function _onMouseOver() {
  if (gAlarmSounding) {
    eButtons.visible = true;
  }
}

/**
 * Handles gadget mouse out event
 */
function _onMouseOut() {
  eButtons.visible = false;
}

/**
 * Update clock every some seconds in case the precise update interval
 * fails for some reason.  Still less aggressive than constant polling
 */
function backupUpdate() {
  gTimeDisplay.update();
}

/**
 * Sound the alarm!!!
 */
function soundTheAlarm() {
  debug.warning("sound the alarm!");
  gAlarmSounding = true;
  eLights.visible = true;
  eBell.src = "images\\bell_ringing.png";
  gLightsToken = setInterval(blinkLights, kBlinkInterval);
  gSnoozing = false;

  // try playing file, if error change to default
  if (!options.getValue(OPT_DEFAULT_RINGER)) {
    try {
      gAlarmClip = framework.audio.open(options.getValue(OPT_FILE_PATH));
    } catch(e) {
      debug.error("cannot play sound file!");
      options.putValue(OPT_DEFAULT_RINGER, true);
    }
  }

  // try playing default, if error alert user
  if (options.getValue(OPT_DEFAULT_RINGER)) {
    try {
      gAlarmClip = framework.audio.open("alarm.wav");
    } catch(e) {
      debug.error("cannot play default alarm!");
      alert(ERROR);
      return;
    }
  }
  gAlarmClip.onStateChange = clipStateChange;
  gAlarmClip.play();
}

/**
 * Handles state change for audio clip
 */
function clipStateChange(clip, state) {
  debug.trace("clip state changed to: " + state);
  if (state == gddSoundStateStopped) {
    debug.trace("clip stopped");
    if (gAlarmSounding && !gSnoozing) {
      // loop clip if alarm is still going
      gAlarmToken = setTimeout(function() { gAlarmClip.play(); }, 
                               kAlarmRepeatBuffer);
    }
  }
  if (gAlarmClip.state == gddSoundStateError) {
    debug.trace("error playing alarm");
    alert(ERROR);
  }
}

/**
 * Toggle lights
 */
function blinkLights() {
  eLights.visible = !eLights.visible;
}  

/**
 * Stop the alarm from sounding
 */
function _alarmOff() {
  gAlarmSounding = false;
  eLights.visible = false;
  eButtons.visible = false;
  gSnoozing = false;
  eBell.src = "images\\bell.png";
  clearInterval(gLightsToken);
  clearInterval(gAlarmToken);
  clearInterval(gSnoozeToken);
  framework.audio.stop(gAlarmClip);
}

/**
 * Stop the alarm from sounding, but set the alarm to sound
 * after a snooze interval.
 */
function _alarmSnooze() {
  gSnoozing = true;
  eLights.visible = false;
  eButtons.visible = false;
  clearInterval(gLightsToken);
  clearInterval(gAlarmToken);
  clearInterval(gSnoozeToken);
  framework.audio.stop(gAlarmClip);
  gSnoozeToken = setTimeout(function() { soundTheAlarm(); }, kSnoozeIntervalMs);

}

/**
 * Query maps api for location coords
 */
function queryMaps() {
  var url = "http://maps.google.com/maps/geo?q=";
  url += encodeURIComponent(options.getValue(OPT_CITY)).replace(" ","+");
  url += "&output=xml&key=" + MAPS_KEY;

  try {
    var http = new XMLHttpRequest();
    http.onreadystatechange = HTTPData;
    http.open("GET", url, true);
    http.send(null);
  } catch(err) {
    debug.error("cannot connect");
    options.putValue(OPT_ADDRESS, "");
    options.putValue(OPT_CITY, "");
    options.putValue(OPT_COORD, kDefaultCoord);
    shiftMap(kDefaultCoord);
    eMapTime.align = "center";
    eCity.innerText = "";
    return;
  }

  function HTTPData() {
    var feed;

    if (http.readyState == 4) {
      parseCoords(http.responseXML);
    } else {
      return;
    }
  }
}

/**
 * Take the maps xml response and find the coords, then center map
 * @param {Object} doc dom document returned from maps query
 */
function parseCoords(doc) {
  var locale = "";
  try {
    locale = doc.getElementsByTagName(LOC_TAG)[0].firstChild.nodeValue;
    debug.trace("locale: " + locale);
  } catch(e) {
    locale = options.getValue(OPT_CITY);
    debug.trace("no locale, using name: " + locale);
  }

  try {
    var coords = doc.getElementsByTagName(COORD_TAG)[0].firstChild.nodeValue;
    var address = doc.getElementsByTagName(ADDRESS_TAG)[0].firstChild.nodeValue;
    var longitude = parseFloat(coords.split(",")[0]);
    debug.trace(locale + "\t" + coords);
    eCity.innerText = locale;
    eMapTime.align = "right";
    gLocation = locale;
    options.putValue(OPT_CITY, locale);
    options.putValue(OPT_ADDRESS, address);
    options.putValue(OPT_COORD, longitude);
    shiftMap(longitude);
  } catch(e) {
    debug.error("can't get locality info");
    options.putValue(OPT_ADDRESS, "");
    options.putValue(OPT_CITY, "");
    options.putValue(OPT_COORD, kDefaultCoord);
    shiftMap(kDefaultCoord);
    eMapTime.align = "center";
    eCity.innerText = "";
    return;
  }
}

/**
 * Rotate the map to center on the location
 * @param {Number} coord the longitude coordinate, used to center map
 */
function shiftMap(coord) {
  debug.trace("shifting map to " + coord);
  coord = coord;
  var coeff = coord / 360.0;
  var offset = -1 * coeff * eWorldMap.width + kMapOffset;
  var oldx = (eMap1.x < eMap2.x ? eMap1.x : eMap2.x);

  // rotate the map
  eMap1.x = offset;
  if (eMap1.x > 0) {
    eMap2.x = eMap1.x - eWorldMap.width;
  } else {
    eMap2.x = eMap1.x + eWorldMap.width;
  }

  // shift the daylight mask appropriately
  var newx = (eMap1.x < eMap2.x ? eMap1.x : eMap2.x);
  eLight1.x += newx - oldx;
  
  if (eLight1.x + eWorldMap.width < 0) {
    eLight1.x += eWorldMap.width;
  }

  if (eLight1.x <= 0) {
    eLight2.x = eLight1.x + eWorldMap.width;
  } else {
    eLight2.x = eLight1.x - eWorldMap.width;
  }  
}