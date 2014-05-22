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

// TODO: Remove definitions not applicable to this version.

// This script is included before executing a script plugin. This declares
// the various flags used in the API.

// Details view flags
var gddDetailsViewFlagNone = 0;
var gddDetailsViewFlagToolbarOpen = 1;
var gddDetailsViewFlagNegativeFeedback = 2;
var gddDetailsViewFlagRemoveButton = 4;
var gddDetailsViewFlagShareWithButton = 8;
var gddDetailsViewFlagDisableAutoClose = 16;
var gddDetailsViewFlagNoFrame = 32;

// Plugin flags
var gddPluginFlagNone = 0;
var gddPluginFlagToolbarBack = 1;
var gddPluginFlagToolbarForward = 2;

// Info masks
var gddInfoMaskNone = 0;
var gddInfoMaskMinSize = 1;
var gddInfoMaskMaxSize = 2;
var gddInfoMaskIdealSize = 4;

// Plugin commands
var gddCmdAboutDialog = 1;
var gddCmdToolbarBack = 2;
var gddCmdToolbarForward = 3;

// Content item layouts
var gddContentItemLayoutNowrapItems = 0;
var gddContentItemLayoutNews = 1;
var gddContentItemLayoutEmail = 2;

// Content flags
var gddContentFlagNone = 0;
var gddContentFlagHaveDetails = 1;
var gddContentFlagPinnable = 2;
var gddContentFlagManualLayout = 4;
var gddContentFlagNoAutoMinSize = 8;

// Content item flags
var gddContentItemFlagNone              = 0x0000;
var gddContentItemFlagStatic            = 0x0001;
var gddContentItemFlagHighlighted       = 0x0002;
var gddContentItemFlagPinned            = 0x0004;
var gddContentItemFlagTimeAbsolute      = 0x0008;
var gddContentItemFlagNegativeFeedback  = 0x0010;
var gddContentItemFlagLeftIcon          = 0x0020;
var gddContentItemFlagNoRemove          = 0x0040;
var gddContentItemFlagsShareable        = 0x0080;
var gddContentItemFlagsShared           = 0x0100;
var gddContentItemFlagsInteracted       = 0x0200;
var gddContentItemFlagShareable         = 0x0080;
var gddContentItemFlagShared            = 0x0100;
var gddContentItemFlagInteracted        = 0x0200;
var gddContentItemFlagDisplayAsIs       = 0x0400;
var gddContentItemFlagHtml              = 0x0800;
var gddContentItemFlagHidden            = 0x1000;

// Tile display state
var gddTileDisplayStateHidden = 0;
var gddTileDisplayStateRestored = 1;
var gddTileDisplayStateMinimized = 2;
var gddTileDisplayStatePoppedOut = 3;
var gddTileDisplayStateResized = 4;

// Target device
var gddTargetSidebar = 0;
var gddTargetNotifier = 1;
var gddTargetFloatingView = 2;

// ContentItem display options
var gddItemDisplayInSidebar = 1;
var gddItemDisplayInSidebarIfVisible = 2;
var gddItemDisplayAsNotification = 4;
var gddItemDisplayAsNotificationIfSidebarHidden = 8;

// Window classes
var gddWndCtrlClassLabel = 0;
var gddWndCtrlClassEdit = 1;
var gddWndCtrlClassList = 2;
var gddWndCtrlClassButton = 3;

var gddWndCtrlTypeNone = 0;

// List control types
var gddWndCtrlTypeListOpen = 0;
var gddWndCtrlTypeListDrop = 1;
// Button control types
var gddWndCtrlTypeButtonPush = 2;
var gddWndCtrlTypeButtonCheck = 3;
// Edit control types
var gddWndCtrlTypeEditPassword = 10;

// Font IDs
var gddFontNormal = -703;
var gddFontBold = 577;
var gddFontSnippet = 575;
var gddFontExtraInfo = 576;

// Text Color Names
var gddColorNormalText = "#000000";
var gddColorNormalBackground = "#FBFBFB";
var gddColorSnippet = "#666666";
var gddColorExtraInfo = "#224499";

// Text flags
var gddTextFlagCenter = 1;
var gddTextFlagRight = 2;
var gddTextFlagVCenter = 4;
var gddTextFlagBottom = 8;
var gddTextFlagWordBreak = 16;
var gddTextFlagSingleLine = 32;

// Menu item flags
var gddMenuItemFlagGrayed = 1;
var gddMenuItemFlagChecked = 8;

// Button IDs which appear in the options dialog
var gddIdOK = 1;
var gddIdCancel = 2;

// Logging levels
var gddLogLevelDebug = 0;
var gddLogLevelInfo = 100;
var gddLogLevelWarning = 200;
var gddLogLevelError = 300;

// Friend status
var gddFriendStatusOnline = 0;
var gddFriendStatusIdle = 1;
var gddFriendStatusBusy = 2;

// Send Talk data flags
var gddSendDataFlagNone = 0;
var gddSendDataFlagSilent = 1;

// Google Talk status
var gddTalkStatusNotRunning = 0;
var gddTalkStatusDisconnected = 1;
var gddTalkStatusConnected = 2;

// Item "Send to" destinations
var gddSendToSidebar = 0;
var gddSendToIM = 1;
var gddSendToEmail = 2;

// Sound clip playback state
var gddSoundStateError = -1;
var gddSoundStateStopped = 0;
var gddSoundStatePlaying = 1;
var gddSoundStatePaused = 2;

// Sound clip error code
var gddSoundErrorNoError = 0;
var gddSoundErrorUnknown = 1;
var gddSoundErrorBadClipSrc = 2;
var gddSoundErrorFormatNotSupported = 3;

// Network connection types
// (needs to match with the NDIS_MEDIUM enum in ntddndis.h)
var gddConnectionType802_3 = 0;
var gddConnectionType802_5 = 1;
var gddConnectionTypeFddi = 2;
var gddConnectionTypeWan = 3;
var gddConnectionTypeLocalTalk = 4;
var gddConnectionTypeDix = 5;
var gddConnectionTypeArcnetRaw = 6;
var gddConnectionTypeArcnet878_2 = 7;
var gddConnectionTypeAtm = 8;
var gddConnectionTypeWirelessWan = 9;
var gddConnectionTypeIrda = 10;
var gddConnectionTypeBpc= 11;
var gddConnectionTypeCoWan = 12;
var gddConnectionType1394 = 13;
var gddConnectionTypeInfiniBand = 14;
var gddConnectionTypeTunnel = 15;
var gddConnectionTypeNative802_11 = 16;
var gddConnectionTypeXdsl = 17;

// Network connection physical media types
// (needs to match with the NDIS_PHYSICAL_MEDIUM enum in ntddndis.h)
var gddPhysicalMediaTypeUnspecified = 0;
var gddPhysicalMediaTypeWirelessLan = 1;
var gddPhysicalMediaTypeCableModem = 2;
var gddPhysicalMediaTypePhoneLine = 3;
var gddPhysicalMediaTypePowerLine = 4;
var gddPhysicalMediaTypeDSL = 5;
var gddPhysicalMediaTypeFibreChannel = 6;
var gddPhysicalMediaType1394 = 7;
var gddPhysicalMediaTypeWirelessWan = 8;
var gddPhysicalMediaTypeNative802_11 = 9;
var gddPhysicalMediaTypeBluetooth = 10;

// Wireless network types
// (needs to match with the DOT11_BSS_TYPE enum in windot11.h)
var gddWirelessTypeInfrastructure = 0;
var gddWirelessTypeIndependent = 1;
var gddWirelessTypeAny = 2;

// Topic Ids
var gddTopicIdUnknown       = -1;
var gddTopicIdBusiness      = 0;
var gddTopicIdFinance       = 1;
var gddTopicIdNews          = 2;
var gddTopicIdKids          = 3;
var gddTopicIdGames         = 4;
var gddTopicIdHealth        = 5;
var gddTopicIdTravel        = 6;
var gddTopicIdScience       = 7;
var gddTopicIdShopping      = 8;
var gddTopicIdComputers     = 9;
var gddTopicIdTechnology    = 10;
var gddTopicIdProgrammer    = 11;
var gddTopicIdWeblogs       = 12;
var gddTopicIdSocial        = 13;
var gddTopicIdSports        = 14;
var gddTopicIdEntertainment = 15;
var gddTopicIdMovies        = 16;
var gddTopicIdTV            = 17;
var gddTopicIdCareers       = 18;
var gddTopicIdWeather       = 19;
var gddTopicIdRealestate    = 20;

// Element opacity
var gddElementMaxOpacity   = 255;
var gddElementMinOpacity   = 0;

// Content Ranking Flags
var contentRankingFlag = {};
contentRankingFlag.None = 0;
contentRankingFlag.AlwaysTop = 1;
contentRankingFlag.AlwaysBottom = 2;

// Content Ranking Algorithm Types
var contentRankingAlgorithm = {};
contentRankingAlgorithm.None = 0;
contentRankingAlgorithm.DirectlyProportional = 1;
contentRankingAlgorithm.InverselyProportional = 2;
contentRankingAlgorithm.LeastSteep = 4;
contentRankingAlgorithm.MostSteep = 8;

// BrowseForFile() and BrowseForFiles() modes
var gddBrowseFileModeOpen = 0;
var gddBrowseFileModeFolder = 1;
var gddBrowseFileModeSaveAs = 2;

// Return value of view.confirm
var gddConfirmResponseYes = 1;
var gddConfirmResponseNo = 0;
var gddConfirmResponseCancel = -1;

// Windows Enumerator adapter
function Enumerator(coll) {
  var pos_ = 0;
  var last_item_ = null;
  if (coll.length == undefined && coll.count == undefined)
    throw "Enumerator's argument must have either length or count property.";
  if (coll.constructor != Array && typeof(coll.item) != "function")
    throw "Enumerator's argument must be either a JavaScript array" +
          " or an object providing item(integer) method.";
  if (coll.count != undefined && coll.moveFirst != undefined &&
      coll.moveNext != undefined && coll.item != undefined &&
      coll.atEnd != undefined)
    return coll;

  this.atEnd = function() {
    try {
      return pos_ >= (coll.length == undefined ? coll.count : coll.length);
    } catch (e) {
      return true;
    }
  };

  this.item = function() {
    if (this.atEnd()) return null;
    return Array.prototype.isPrototypeOf(coll) ? coll[pos_] : coll.item(pos_);
  };

  this.moveFirst = function() {
    pos_ = 0;
    last_item_ = this.item();
  };

  this.moveNext = function() {
    if (!this.atEnd()) {
      if (this.item() == last_item_) pos_++;
      last_item_ = this.item();
    }
  };

  last_item_ = this.item();
}

function VBArray(array) {
  this.toArray = function() { return array; };
}

function _ActiveXObject_OpenUrl_(url) {
  debug.trace("Open URL by ActiveX object:" + url);
  // If the url doesn't have prefix, assume it's a file path.
  if (url.indexOf("://") < 0)
    url = encodeURI("file://" + url);
  framework.openUrl(url);
}

// Emulates the WebBrowser class, only with very limited functionalities.
function WebBrowser() {
  this.Navigate = _ActiveXObject_OpenUrl_;
  this.navigate = _ActiveXObject_OpenUrl_;
  // Only the first parameter of Navigate2() is supported.
  this.Navigate2 = _ActiveXObject_OpenUrl_;
  this.navigate2 = _ActiveXObject_OpenUrl_;
}

// Emulates the ADODB.Stream class, only with very limited functionalities.
var adModeRead = 1;
var adModeWrite = 2;
var adModeUnknown = 0;
var adTypeBinary = 1;
var adTypeText = 2;
var adWriteChar = 0;
var adWriteLine = 1;

function ADODBStream() {
}

ADODBStream.prototype.Type = adTypeBinary;
ADODBStream.prototype.Mode = adModeUnknown;
ADODBStream.prototype.read_file_stream_ = null;
ADODBStream.prototype.write_data_ = [];

ADODBStream.prototype.getType_ = function() {
  return (this.type != undefined ? this.type : this.Type);
};

ADODBStream.prototype.Cancel = function() {
};
ADODBStream.prototype.cancel = ADODBStream.prototype.Cancel;

ADODBStream.prototype.Close = function() {
  if (this.read_file_stream_)
    this.read_file_stream_.Close();
  this.read_file_stream_ = null;
};
ADODBStream.prototype.close = ADODBStream.prototype.Close;

ADODBStream.prototype.CopyTo = function() {
  throw "ADODB.Stream: Unimplemented: CopyTo";
};
ADODBStream.prototype.copyTo = ADODBStream.prototype.CopyTo;

ADODBStream.prototype.Flush = function() {
};
ADODBStream.prototype.flush = ADODBStream.prototype.Flush;

ADODBStream.prototype.LoadFromFile = function(fileName) {
  var type = this.getType_();
  this.Mode = adModeRead; // Switch to read mode.
  if (type == adTypeBinary) { // binary type.
    this.read_file_stream_ =
      framework.system.filesystem.OpenBinaryFile(fileName, 1, false);
  } else if (type == adTypeText) { // text type.
    this.read_file_stream_ =
      framework.system.filesystem.OpenTextFile(fileName, 1, false);
  } else {
    throw "ADODB.Stream: Invalid Type: " + this.Type;
  }

  if (!this.read_file_stream_) {
    throw "ADODB.Stream: Failed to load from file: " + fileName;
  }
};
ADODBStream.prototype.loadFromFile = ADODBStream.prototype.LoadFromFile;

ADODBStream.prototype.Open = function(source, mode) {
  if (mode != undefined)
    this.Mode = mode;
};
ADODBStream.prototype.open = ADODBStream.prototype.Open;

ADODBStream.prototype.Read = function(numBytes) {
  if (this.Mode != adModeRead || !this.read_file_stream_)
    throw "ADODB.Stream: Not a readable stream.";

  var type = this.getType_();
  if (type != adTypeBinary)
    throw "ADODB.Stream: Read can not be called against a text stream.";

  if (numBytes == undefined || numBytes < 0)
    return this.read_file_stream_.ReadAll();

  return this.read_file_stream_.Read(numBytes);
};
ADODBStream.prototype.read = ADODBStream.prototype.Read;

ADODBStream.prototype.ReadText = function(numChars) {
  if (this.Mode != adModeRead || !this.read_file_stream_)
    throw "ADODB.Stream: Not a readable stream.";

  var type = this.getType_();
  if (type != adTypeText)
    throw "ADODB.Stream: ReadText can not be called against a binary stream.";

  if (numChars == undefined || numChars < 0)
    return this.read_file_stream_.ReadAll();

  return this.read_file_stream_.Read(numChars);
};
ADODBStream.prototype.readText = ADODBStream.prototype.ReadText;

ADODBStream.prototype.SaveToFile = function(fileName, options) {
  if (this.Mode != adModeWrite)
    throw "ADODB.Stream: Not a writable stream.";

  var type = this.getType_();
  var stream = null;
  if (type == adTypeBinary) {
    stream = framework.system.filesystem.OpenBinaryFile(fileName, 2, true);
  } else if (type == adTypeText) {
    stream = framework.system.filesystem.OpenTextFile(fileName, 2, true);
  } else {
    throw "ADODB.Stream: Invalid Type: " + this.Type;
  }

  if (!stream) {
    throw "ADODB.Stream: Failed to create stream for writing.";
  }

  for (i in this.write_data_) {
    stream.Write(this.write_data_[i]);
  }
  stream.Close();
};
ADODBStream.prototype.saveToFile = ADODBStream.prototype.SaveToFile;

ADODBStream.prototype.SetEOS = function() {
};
ADODBStream.prototype.setEOS = ADODBStream.prototype.SetEOS;

ADODBStream.prototype.SkipLine = function() {
  if (this.Mode != adModeRead || !this.read_file_stream_)
    throw "ADODB.Stream: Not a readable stream.";

  var type = this.getType_();
  if (type != adTypeText)
    throw "ADODB.Stream: SkipLine can not be called against a binary stream.";

  this.read_file_stream_.SkipLine();
};
ADODBStream.prototype.skipLine = ADODBStream.prototype.SkipLine;

ADODBStream.prototype.Write = function(data) {
  this.Mode = adModeWrite;
  this.write_data_.push(data);
};
ADODBStream.prototype.write = ADODBStream.prototype.Write;

ADODBStream.prototype.WriteText = function(data, options) {
  this.Mode = adModeWrite;
  data = data + "";
  if (options == adWriteLine &&
      (!data.length || data.charAt(data.length - 1) != '\n'))
    this.write_data_.push(data + "\n");
  else
    this.write_data_.push(data);
};
ADODBStream.prototype.writeText = ADODBStream.prototype.WriteText;

// Emulates some popular ActiveX objects
// "Microsoft.XMLHTTP"
// "Microsoft.XMLDOM"
// "Shell.Application"
// "WScript.Shell"
// "InternetExplorer.Application"
// "Scripting.FileSystemObject"
// "Msxml.DOMDocument"
// "Msxml2.ServerXMLHTTP.3.0"
// "Msxml2.DOMDocument.3.0"
// TODO: Is it necessary to emulate "ADODB.Stream"?
function ActiveXObject(name) {
  name = name.toLowerCase();
  debug.warning("new ActiveXObject: " + name);
  if (name == "shell.application") {
    this.open = _ActiveXObject_OpenUrl_;
    this.Open = _ActiveXObject_OpenUrl_;
    this.ShellExecute = _ActiveXObject_OpenUrl_;
    this.shellExecute = _ActiveXObject_OpenUrl_;
    this.shellexecute = _ActiveXObject_OpenUrl_;
  } else if (name == "wscript.shell") {
    this.Run = _ActiveXObject_OpenUrl_;
    this.run = _ActiveXObject_OpenUrl_;
  } else if (name == "internetexplorer.application") {
    return new WebBrowser();
  } else if (name == "scripting.filesystemobject") {
    return framework.system.filesystem;
  } else if (name.match(/^(microsoft|msxml2|msxml)\.(xmlhttp|serverxmlhttp)/)) {
    return new XMLHttpRequest();
  } else if (name.match(/^(microsoft|msxml2|msxml)\.(xmldom|domdocument)/)) {
    return new DOMDocument();
  } else if (name.match(/^wmplayer.ocx/)) {
    return view.appendElement('<object classId="progid:WMPlayer.OCX.7"/>')
        .object;
  } else if (name == "adodb.stream") {
    return new ADODBStream();
  } else {
    throw "Unsupported ActiveXObject: " + name;
  }
}

// Date.prototype.getVarDate() and Array.prototype.toArray() are defined in
// C++ programs to avoid the methods from being enumerated.
