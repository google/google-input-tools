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

var kDownloadURLOption = "download_url";
var kModuleURLOption = "module_url_prefix";
var kBrowserMarginX = 14;
var kBrowserMarginY = 19;

var g_user_pref_names = null;
var g_user_pref_required = null;
var g_gadget_attribs = null;

var kUserPrefPrefix = "up_";
var kUserAgent = "Mozilla/5.0 (compatible; Google Desktop)";

var kDefaultLocale = "en-US";
var kDefaultModuleURLPrefix = "http://gmodules.com/ig/ifr?url=";
options.putDefaultValue(kModuleURLOption, kDefaultModuleURLPrefix);

var g_download_url = options.getValue(kDownloadURLOption);
var g_module_url_prefix = options.getValue(kModuleURLOption);
var g_commonparams = null;
var g_xml_request_gadget = null;
var g_xml_request_options = null;

/// Gadget init code.
function ViewOnOpen() {
  browser.external = {
    ShowOptions: function() {
      setTimeout(function() {
        // Dispatch this asynchronously.
        plugin.ShowOptionsDialog();
      }, 0);
    }
  };

  try {
    browser.onerror = OnBrowserError;
  } catch (e) {
  }

  plugin.onAddCustomMenuItems = OnAddCustomMenuItems;
  SetUpCommonParams();
  LoadGadget();
}

/// Functions Start

function OnAddCustomMenuItems(menu) {
  // Grayed if gadget hasn't been loaded yet.
  var refresh_flags = (g_user_pref_names == null) ? gddMenuItemFlagGrayed : 0;
  menu.AddItem(strings.GADGET_REFRESH, refresh_flags, RefreshMenuHandler);
  var unload_flags = addGadgetUI.visible ? gddMenuItemFlagGrayed : 0;
  menu.AddItem(strings.GADGET_UNLOAD, unload_flags, UnloadMenuHandler);
}

function OnBrowserError(url) {
  view.setTimeout(function() {
    DisplayMessage(strings.GADGET_ERROR);
  }, 100);
  return true;
}

function RefreshMenuHandler(item_text) {
  RefreshGadget();
}

function UnloadMenuHandler(item_text) {
  g_user_pref_names = null;
  g_user_pref_required = null;
  g_gadget_attribs = null;
  g_download_url = null;
  options.putValue(kDownloadURLOption, g_download_url);

  view.caption = strings.GADGET_NAME;
  plugin.about_text = strings.GADGET_DESC;

  if (view.width < 200)
    view.width = 200;
  if (view.height < 200)
    view.height = 200;

  LoadGadget();
}

function LoadOptions() {
  // Cannot use a frame to load the content here because of security
  // restrictions involving scripts on pages from different domains.
  g_xml_request_options = new XMLHttpRequest();
  g_xml_request_options.onreadystatechange = ProcessOptions;
  g_xml_request_options.open("GET", GetOptionsURL(), true);
  g_xml_request_options.setRequestHeader("User-Agent", kUserAgent);
  g_xml_request_options.send(null);
}

function ProcessOptions() {
  var readystate = g_xml_request_options.readyState;
  if (4 == readystate) { // complete
    if (200 == g_xml_request_options.status) {
      options.putValue(kOptionsHTML, g_xml_request_options.responseText);
    } else {
      gadget.debug.error("Error loading options HTML for gadget.");
    }
    g_xml_request_options = null;
  }
}

function OnOptionChanged() {
  if (event.propertyName == kRefreshOption) {
    RefreshGadget();
  }
}

/// Main view

function RefreshGadget() {
  if (HasUnsetUserPrefs()) {
    // Must show options dialog before showing gadget.
    var msg = strings.GADGET_REQUIRED
      + "<br><br><button onclick=\"window.external.ShowOptions()\">"
      + strings.GADGET_SHOWOPTIONS
      + "</button>";
    DisplayMessage(msg);
  } else {
    gadget.debug.trace("Showing gadget.");

    // We can show the gadget.
    var html = "<html>"
      + kHTMLMeta
      + "<frameset border=\"0\" cols=\"100%\" frameborder=\"no\">"
      + "<frame height=\"100%\" width=\"100%\" scrolling=\"no\" src=\""
      + GetGadgetURL() + "\" marginheight=\"0\" marginwidth=\"0\"/>"
      + "</frameset></html>";
    browser.innerText = html;
  }
}

function LoadGadget() {
  if (!g_download_url) {
    addGadgetUI.visible = true;
    browser.visible = false;
    DisplayMessage(strings.GADGET_ERROR);
    return;
  }

  addGadgetUI.visible = false;
  browser.visible = true;

  DisplayMessage(strings.GADGET_LOADING);

  try {
    LoadRawXML();
    LoadOptions();
  } catch (e) {
    gadget.debug.error(e);
    DisplayMessage(strings.GADGET_ERROR);
  }
}

function LoadRawXML() {
  gadget.debug.trace("Loading raw XML.");

  g_xml_request_gadget = new XMLHttpRequest();
  g_xml_request_gadget.onreadystatechange = ProcessXML;
  g_xml_request_gadget.open("GET", GetConfigURL(), true);
  g_xml_request_gadget.setRequestHeader("User-Agent", kUserAgent);
  g_xml_request_gadget.send(null);
}

function ProcessXML() {
  var readystate = g_xml_request_gadget.readyState;
  if (4 == readystate) { // complete
    if (200 == g_xml_request_gadget.status) {
      ParseRawXML();
    } else {
      DisplayMessage(g_xml_request_gadget.status + ": " + strings.GADGET_ERROR);
    }
    g_xml_request_gadget = null;
  }
}

function HasUnsetUserPrefs() {
  var result = false;

  // Also generate preset options script fragment here.
  var preset = "";

  var len  = g_user_pref_names.length;
  for (var i = 0; i < len; i++) {
    var pref = g_user_pref_names[i];
    var value = options.getValue(pref);
    if (value == null) {
      gadget.debug.trace("Unset pref: " + pref +
                         " Required: " + g_user_pref_required[i]);
      // Do not break to continue generating script fragment.
      if (g_user_pref_required[i]) {
        result = true;
      }
    } else { // pref is set, add to prefix
      if (preset != "") {
        preset += "else ";
      }
      preset += "if(obj.name=='m_" + EncodeJSString(pref) + "'){"
          + "obj.value='" + EncodeJSString(value) + "';}";
    }
  }

  options.putValue(kPresetOptions, preset);

  return result;
}

function ViewOnSizing() {
  if (null != g_gadget_attribs) {
    var w = g_gadget_attribs.width;
    var h = g_gadget_attribs.height;
    // Disallow resizing to larger than specified w, h values.
    if (null != w && event.width > w + kBrowserMarginX) {
      event.width = w + kBrowserMarginX;
    }
    if (null != h && event.height > h + kBrowserMarginY) {
      event.height = h + kBrowserMarginY;
    }
  }

  // Disallow resizing to smaller than resize border's margin.
  if (event.width < kBrowserMarginX + 10)
    event.width = kBrowserMarginX + 10;
  if (event.height < kBrowserMarginY + 10)
    event.height = kBrowserMarginY + 10;

  gadget.debug.trace("OnSizing: " + event.width + ", " + event.height);
}

function ViewOnSize() {
  addGadgetUI.width = browser.width = view.width - kBrowserMarginX;
  addGadgetUI.height = browser.height = view.height - kBrowserMarginY;
  urlHint.width = urlBox.width = addGadgetUI.width - 4;
  urlHint.height = Math.max(addGadgetUI.height - 68, 10);
  urlEntry.width = urlBox.width - 2;
}

function ShowGadget() {
  view.caption = g_gadget_attribs.title;
  plugin.about_text = g_gadget_attribs.title + "\n\n"
      + g_gadget_attribs.author
      + (g_gadget_attribs.author_email ?
         " (" + g_gadget_attribs.author_email + ")" : "")
      + "\n\n" + g_gadget_attribs.description;

  var w = g_gadget_attribs.width;
  if (w) view.width = w + kBrowserMarginX;
  var h = g_gadget_attribs.height;
  if (h) view.height = h + kBrowserMarginY;

  RefreshGadget();
}

function ParseRawXML() {
  var xml = g_xml_request_gadget.responseXML;
  if (xml == null) {
    DisplayMessage(strings.GADGET_ERROR);
    return false;
  }

  // Read gadget attributes.
  var mod_prefs = xml.getElementsByTagName("ModulePrefs");
  if (null == mod_prefs) {
    DisplayMessage(strings.GADGET_ERROR);
    return false;
  }

  var attribs = mod_prefs[0];
  g_gadget_attribs = {};
  g_gadget_attribs.title = GetElementAttrib(attribs, "title");
  g_gadget_attribs.description = GetElementAttrib(attribs, "description");
  g_gadget_attribs.author = GetElementAttrib(attribs, "author");
  g_gadget_attribs.author_email = GetElementAttrib(attribs, "author_email");

  var param = GetElementAttrib(attribs, "width");
  g_gadget_attribs.width = (param == "") ? null : Number(param);
  param = GetElementAttrib(attribs, "height");
  g_gadget_attribs.height = (param == "") ? null : Number(param);

  // Note: it is important that the loop below do not overwrite prefs
  // that the user may have already set in a previous run of this gadget
  // instance. Thus only the default value is set here.
  var prefs = xml.getElementsByTagName("UserPref");
  g_user_pref_names = new Array();
  g_user_pref_required = new Array();
  if (null != prefs) {
    var len  = prefs.length;
    for (var i = 0; i < len; i++) {
      var pref = prefs[i];
      var name = g_user_pref_names[i] =
          kUserPrefPrefix + GetElementAttrib(pref, "name");
      var required = GetElementAttrib(pref, "required");
      // According to Gadgets API reference, the value of required attribute is
      // false by default.
      g_user_pref_required[i] = (required == "true" || required == "True" ||
                                 required == "TRUE" || required == "1");
      var def_node = pref.getAttributeNode("default_value");
      if (def_node == null) {
        options.putDefaultValue(name, null);
      } else {
        var def = pref.getAttribute("default_value");
        options.putDefaultValue(name, def);
      }
    }
  }

  ShowGadget();

  return true;
}

function GetElementAttrib(elem, attrib_name) {
  var attrib = elem ? elem.getAttribute(attrib_name) : "";
  return TrimString(attrib);
}

function TrimString(s) {
  return s.replace(/\s+$/gm, "").replace(/^\s+/gm, "");
}

function EncodeJSString(s) {
  s = s.replace(/\"/g, "\\\"");
  s = s.replace(/\\/g, "\\\\");
  s = s.replace(/\n/g, "\\n");
  s = s.replace(/\r/g, "\\r");

  return s;
}

function DisplayMessage(msg) {
  // Assume these messages are properly escaped.
  browser.innerText = "<html>"
    + kHTMLMeta
    + "<body><center><h6>"
    + msg
    + "</h6></center></body></html>";
}

function SetUpCommonParams() {
  var locale = framework.system.languageCode();
  var parts = locale.split('-', 2);
  g_commonparams = "&.lang=" + locale + "&.country=" + parts[1] + "&synd=open";
}

function GetConfigURL() {
  return g_module_url_prefix + encodeURI(g_download_url)
    + "&output=rawxml" + g_commonparams;
}

function GetGadgetURL() {
  var len  = g_user_pref_names.length;
  var params = "";
  for (var i = 0; i < len; i++) {
    var pref = g_user_pref_names[i];
    var value = options.getValue(pref);
    if (value == null) {
      continue;
    }
    params += "&" + encodeURIComponent(pref) + "=" + encodeURIComponent(value);
  }

  return g_module_url_prefix + encodeURI(g_download_url)
    + params + g_commonparams;
}

function GetOptionsURL() {
  return g_module_url_prefix + encodeURI(g_download_url)
    + "&output=edithtml" + g_commonparams;
}

function OnDragOver() {
  if (event.dragUrls.length != 1)
    event.returnValue = false;
}

function OnDragDrop() {
  urlEntry.value = event.dragUrls[0];
  urlEntry.selectAll();
  urlEntry.focus();
}

function OnLoadGadget() {
  var url_patterns = [
    /^\w+:\/\/\w+\.google\.\w+\/ig\/directory.*type=gadgets.*url=(.*.xml)/,
    /^\w+:\/\/\w+\.google\.\w+\/ig\/adde.*moduleurl=(.*.xml)/
  ];

  var gadget_url = urlEntry.value;
  if (gadget_url) {
    for (i in url_patterns) {
      var match_result = gadget_url.match(url_patterns[i]);
      if (match_result != null) {
        gadget_url = match_result[1];
        break;
      }
    }
  }

  if (gadget_url) {
    if (!gadget_url.match(/^\w+:\/\//))
      gadget_url = "http://" + gadget_url;
    g_download_url = gadget_url;
    options.putValue(kDownloadURLOption, g_download_url);
    LoadGadget();
  } else {
    alert(strings.INVALID_URL);
  }
}
