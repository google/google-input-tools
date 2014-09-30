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

var SECOND_NONE = 0;
var SECOND_QUARTZ = 1;
var SECOND_SMOOTH = 2;

var OPTION_SECONDHAND = "SecondHand";
options.putDefaultValue(OPTION_SECONDHAND, SECOND_QUARTZ);

var _SecondInterval = 0;

var _minimized = false;

//
// view_onopen
//
function _view_onopen()
{
  ConfigureSecondHand();
  OnMinuteTimer();
}

//
// view_onminimize
//
function _view_onminimize()
{
  _minimized = true;

  OnMinuteTimer();

  // Not updating the title every second so cancel the timer
  if(_SecondInterval != 0)
  {
    clearInterval(_SecondInterval);
    _SecondInterval = 0;
  }
}

//
// view_onrestore
//
function _view_onrestore()
{
  _minimized = false;
  view.caption = strings.PLUGIN_TITLE;

  // Reset the hands on the clock
  ConfigureSecondHand();
  OnMinuteTimer();
}

//
// view_onpopout
//
function _view_onpopout() {
  if (_minimized) {
    // Reset the hands on the clock
    ConfigureSecondHand();
    OnMinuteTimer();
  }
}

//
// OnMinuteTimer -- also updates the title when minimized
//
function OnMinuteTimer()
{
  var Now = new Date();

  if (_minimized) {
    // Draw the time in the title bar in 12 hour format
    var Hours = Now.getHours();
    if(Hours > 12)
      Hours -= 12;

    if( Hours < 10 )
      Hours = "0" + Hours;

    var Minutes = Now.getMinutes();
    if( Minutes < 10 )
      Minutes = "0" + Minutes;

    view.caption = Hours + ':' + Minutes;

  } else {
    UpdateMinuteHand(Now);
    UpdateHourHand(Now);
  }

  var Timeout = (61 - Now.getSeconds()) * 1000
  SetTimeout(OnMinuteTimer, Timeout);
}

//
// OnSecondTimer
//
function OnSecondTimer()
{
  var Now = new Date();
  UpdateSecondHand(Now);
  UpdateMinuteHand(Now);
}

//
// UpdateHourHand
//
function UpdateHourHand(Now)
{
  var Hours = Now.getHours();
  if(Hours >= 12) Hours -= 12;

  var Minutes = Now.getMinutes() + (60 * Hours);

  HourHand.rotation = Minutes / 2;
}

//
// UpdateMinuteHand
//
function UpdateMinuteHand(Now)
{
  var Seconds = Now.getSeconds() + (60 * Now.getMinutes());
  MinuteHand.rotation = Seconds / 10;
}

//
// UpdateSecondHand
//
var _UpdateSecondHandInterval = 0;
var _BounceRotationIncrement = 300 * .006;
var _NewRotation = 0;
function UpdateSecondHand(Now)
{
  if (_UpdateSecondHandInterval != 0)
  {
    clearInterval(_UpdateSecondHandInterval);
    _UpdateSecondHandInterval = 0;
  }

  var Msecs = Now.getMilliseconds() + (Now.getSeconds() * 1000);
  _NewRotation = Msecs * .006;

  SecondHand.rotation = _NewRotation + _BounceRotationIncrement;
  _UpdateSecondHandInterval = SetInterval(UpdateSecondHandRotation, 50);
}

function UpdateSecondHandRotation()
{
  SecondHand.rotation = _NewRotation;
}

//
// ConfigureSecondHand
//
function ConfigureSecondHand()
{
  if(_SecondInterval != 0)
  {
    clearInterval(_SecondInterval);
    _SecondInterval = 0;
  }

  switch(options.getValue(OPTION_SECONDHAND))
  {
  case SECOND_NONE:
    ShowSecondHand(false);
    break;

  case SECOND_SMOOTH:
    ShowSecondHand(true);
    OnSecondTimer();
    _SecondInterval = SetInterval(OnSecondTimer, 25);
    break;

  case SECOND_QUARTZ:
    ShowSecondHand(true);
    OnSecondTimer();
    _SecondInterval = SetInterval(OnSecondTimer, 1000);
    break;
  }
}

//
// ShowSecondHand
//
var _SecondHandFade = 0;
function ShowSecondHand(Show)
{
  var Opacity = Show ? 255 : 0;

  if(_SecondHandFade != 0)
  {
    cancelAnimation(_SecondHandFade);
  }

  if(Opacity != SecondHand.opacity)
  {
    _SecondHandFade = beginAnimation(OnFadeSecondHand, SecondHand.opacity, Opacity, Math.abs(SecondHand.opacity - Opacity) * 5);
  }
}

//
// OnFadeSecondHand
//
function OnFadeSecondHand()
{
  SecondHand.opacity = event.value;
}
