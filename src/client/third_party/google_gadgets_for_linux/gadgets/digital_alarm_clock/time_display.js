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

/**
 * Object that handles the time and date parameters
 * @param {Object} div is the ui container for the clock display
 */
function TimeDisplay(div) {
  var digits = [];
  digits.push(div.appendElement('<img x="0" y="0" opacity="77"/>'));
  digits.push(div.appendElement('<img x="26" y="0" opacity="77"/>'));
  digits.push(div.appendElement('<img x="62" y="0" opacity="77"/>'));
  digits.push(div.appendElement('<img x="88" y="0" opacity="77"/>'));
  digits.push(div.appendElement('<img x="0" y="0" opacity="10"/>'));
  digits.push(div.appendElement('<img x="26" y="0" opacity="10"/>'));
  digits.push(div.appendElement('<img x="62" y="0" opacity="10"/>'));
  digits.push(div.appendElement('<img x="88" y="0" opacity="10"/>'));

  var day = div.appendElement('<label x="0" y="44" size="7" opacity="102"/>');
  var date = div.appendElement('<label x="0" y="44" width="113" ' +
      'align="right" size="7" opacity="102"/>');
  var timeDisplayed = null;
  
  var digi = [];
  for (var i = 0; i < 10; i++) {
    digi.push('images\\digit_' + i + '.png');
  }

  for (var i = 4; i < 8; i++) {
    digits[i].src = digi[8];
  }

  var days = [SUN, MON, TUE, WED, THU, FRI, SAT];
  var months = [JAN, FEB, MAR, APR, MAY, JUN, JUL, AUG, SEP, OCT, NOV, DEC];

  /**
   * Update function takes a new time and uses it to update the display
   * @param {Date} newTime this is a js Date object to update with
   */
  this.update = function(newTime) {
    debug.trace("*");
    newTime = new Date();
    var newHours = newTime.getHours();
    var newMins = newTime.getMinutes();
    var newDay = newTime.getDay();
    var newMonth = newTime.getMonth();
    var newDate = newTime.getDate();
    var newYear = newTime.getFullYear();
    var isPM = newHours > 11 ? true : false;
    var gmtHours = newTime.getUTCHours();
    
    if (!timeDisplayed || timeDisplayed.getHours() != newHours || 
        timeDisplayed.getMinutes() != newMins) {
      // Check to see if its time to alarm
      if (options.getValue(OPT_ALARM) && newHours == options.getValue(OPT_HOURS) && 
          newMins == options.getValue(OPT_MINUTES)) {
        soundTheAlarm();
      }
    }
    
    // Convert to pm time if mode is set to am/pm
    if (gModeAMPM && newHours > 12) {
      newHours -= 12;
    }

    if (gModeAMPM && newHours == 0) {
      newHours += 12;
    }

    // Set the display with the time
    if (newHours > 9) {
      digits[0].src = digi[Math.floor(newHours / 10)];
    } else {
      digits[0].src = "";
    }

    // Make string version of time
    var stringTime = newHours + ":" + (newMins < 10 ? "0" : "") + newMins;

    if (gModeAMPM) {
      stringTime += isPM ? "pm" : "am";
    }

    // map time display
    eMapTime.innerText = stringTime;

    // collapsed time display
    if (gCollapsed) {
      view.caption = stringTime;
    }

    // set daylight mask position
    var light_coeff = gmtHours / 24.0;
    var offset = light_coeff * eWorldMap.width;
    offset += 1/24 * newMins / 60 * eWorldMap.width;
    eLight1.x = eMap1.x - offset - kMapOffset;
    eLight2.x = eLight1.x + eWorldMap.width;
    if (eLight2.x < 0) {
      eLight1.x = eLight2.x + eWorldMap.width;
    } else if (eLight1.x > 0) {
      eLight2.x = eLight1.x - eWorldMap.width;
    }

    // default time display
    digits[1].src = digi[newHours % 10];
    digits[2].src = digi[Math.floor(newMins / 10)];
    digits[3].src = digi[newMins % 10];
    day.innerText = days[newDay];
    date.innerText = months[newMonth] + ' ' + newDate + ', ' + newYear;

    // Set a smart time for the next update
    clearInterval(gUpdateToken);
    var nextUpdate = (61 - newTime.getSeconds()) * 1000;
    nextUpdate = nextUpdate - 800 - newTime.getMilliseconds();
    gUpdateToken = setTimeout(function() { 
                     gTimeDisplay.update(); 
                   }, nextUpdate);

    // Save the current displayed time
    timeDisplayed = newTime;
  }
}
