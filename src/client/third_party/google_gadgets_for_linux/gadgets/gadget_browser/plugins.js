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

var kPluginDownloadURLPrefix = "http://desktop.google.com";

// gPlugins is a two-level table. It is first indexed with language names,
// then with category names. The elements of the table are arrays of plugins.
// For example, gPlugins may contain the following value:
// {en:{news:[plugin1,plugin2,...],...}}.
var gPlugins = {};
var gPluginIdIndex = {};

// Plugin updated in recent two months are listed in the "new" category.
var kNewPluginPeriod = 2 * 30 * 86400000;
var kIGoogleFeedModuleId = 25;

var kPluginTypeSearch = "search";
var kPluginTypeSidebar = "sidebar";
var kPluginTypeIGoogle = "igoogle";
var kPluginTypeFeed = "feed";

var kAllLanguage = "all";
var kAnyLanguage = "any";

var kCategoryAll = "all";
var kCategoryNew = "new";
var kCategoryRecommendations = "recommendations";
var kCategoryGoogle = "google";
var kCategoryRecentlyUsed = "recently_used";
var kCategoryUpdates = "updates";

var kCategoryNews = "news";
var kCategorySports = "sports";
var kCategoryLifestyle = "lifestyle";
var kCategoryTools = "tools";
var kCategoryFinance = "finance";
var kCategoryFunGames = "fun_games";
var kCategoryTechnology = "technology";
var kCategoryCommunication = "communication";
var kCategoryHoliday = "holiday";

var kMaxRecentlyUsedPlugins = 24;

var kIGoogleMoreInfoURLPrefix = "http://www.google.com/ig/directory?url=";
var kIGoogleURLGDIdentifier = "&synd=gd";

var kTopCategories = [
  kCategoryAll, kCategoryNew, kCategoryRecommendations, kCategoryGoogle,
  kCategoryRecentlyUsed, kCategoryUpdates,
];

var kBottomCategories = [
  kCategoryNews, kCategorySports, kCategoryLifestyle, kCategoryTools,
  kCategoryFinance, kCategoryFunGames, kCategoryTechnology,
  kCategoryCommunication, kCategoryHoliday,
];

function SplitValues(src) {
  return src ? src.toLowerCase().split(",") : [];
}

function LoadMetadata() {
  debug.trace("begin loading metadata");
  var metadata = gadgetBrowserUtils.gadgetMetadata;
  if (!metadata || !metadata.length)
    return false;

  gPlugins = {};
  var currentTime = new Date().getTime();
  for (var i = 0; i < metadata.length; i++) {
    var plugin = metadata[i];
    gPluginIdIndex[plugin.id] = plugin;
    plugin.download_status = kDownloadStatusNone;

    var attrs = plugin.attributes;
    if ((attrs.module_id != kIGoogleFeedModuleId ||
         attrs.type != kPluginTypeIGoogle) &&
        attrs.sidebar == "false") {
      // This plugin is not for desktop, ignore it.
      // IGoogle feeding gadgets are supported even if marked sidebar="false".
      continue;
    }

    var languages = SplitValues(attrs.language);
    if (languages.length == 0)
      languages.push("en");
    languages.push(kAllLanguage);

    var categories = SplitValues(attrs.category);
    // TODO: other special categories.
    plugin.rank = parseFloat(attrs.rank);

    for (var lang_i in languages) {
      var language = languages[lang_i];
      var language_categories = gPlugins[language];
      if (!language_categories) {
        language_categories = gPlugins[language] = {};
        language_categories[kCategoryAll] = [];
        language_categories[kCategoryNew] = [];
        language_categories[kCategoryRecentlyUsed] = [];
        language_categories[kCategoryUpdates] = [];
        // TODO: recommendations.
      }

      language_categories[kCategoryAll].push(plugin);
      if (currentTime - plugin.updated_date.getTime() < kNewPluginPeriod)
        language_categories[kCategoryNew].push(plugin);

      for (var cate_i in categories) {
        var category = categories[cate_i];
        var category_plugins = language_categories[category];
        if (!category_plugins)
          category_plugins = language_categories[category] = [];
        category_plugins.push(plugin);
      }
    }
  }
  debug.trace("finished loading metadata");
  return true;
}

function GetPluginTitle(plugin, language) {
  if (!plugin) return "";
  var result = plugin.titles[language];
  if (!result) result = plugin.titles["en"];
  if (!result) result = plugin.attributes.name;
  return result;
}

function GetPluginDescription(plugin, language) {
  if (!plugin) return "";
  var result = plugin.descriptions[language];
  if (!result) result = plugin.descriptions["en"];
  if (!result) result = plugin.attributes.product_summary;
  return result;
}

function GetPluginOtherData(plugin) {
  if (!plugin) return "";
  var result = "";
  var result_arr = new Array();
  var attrs = plugin.attributes;
  if (attrs.version)
    result_arr.push(strings.PLUGIN_VERSION + attrs.version);
  if (attrs.size_kilobytes)
    result_arr.push(attrs.size_kilobytes + strings.PLUGIN_KILOBYTES);
  if (plugin.updated_date.getTime())
    result_arr.push(plugin.updated_date.toLocaleDateString());
  if (attrs.author)
    result_arr.push(attrs.author);
  // For debug:
  // result_arr.push(attrs.uuid);
  // result_arr.push(attrs.id);
  // result_arr.push(attrs.rank);
  if (result_arr.length) {
    result = strings.PLUGIN_DATA_BULLET +
             result_arr.join(strings.PLUGIN_DATA_SEPARATOR);
  }
  return result;
}

function GetPluginInfoURL(plugin) {
  if (!plugin) return "";
  if (plugin.attributes.type != kPluginTypeIGoogle)
    return plugin.attributes.more_info_url;
  return kIGoogleMoreInfoURLPrefix +
         encodeURIComponent(plugin.attributes.download_url) +
         kIGoogleURLGDIdentifier;
}

function AddPlugin(plugin) {
  var result = gadgetBrowserUtils.addGadget(plugin.id);
  if (result >= 0) {
    // Make this gadget listed in the recently_used category.
    plugin.local_accessed_date = new Date();
    if (gPlugins[gCurrentLanguage] &&
        gPlugins[gCurrentLanguage][kCategoryRecentlyUsed])
      gPlugins[gCurrentLanguage][kCategoryRecentlyUsed].sorted = false;
  }
  return result;
}

var gActiveDownloads = [];

function DownloadPlugin(plugin, is_updating) {
  var download_url = plugin.attributes.download_url;
  if (!download_url) {
    SetDownloadStatus(plugin, kDownloadStatusError);
    return;
  }

  if (!download_url.match(/^https?:\/\//))
    download_url = kPluginDownloadURLPrefix + download_url;
  gadget.debug.trace("Start downloading gadget: " + download_url);

  var request = new XMLHttpRequest();
  try {
    request.open("GET", download_url);
    request.onreadystatechange = OnRequestStateChange;
    request.send();
    gActiveDownloads.push(request);
  } catch (e) {
    gadget.debug.error("Request error: " + e);
  }

  function OnRequestStateChange() {
    if (request.readyState == 4) {
      if (request.status == 200) {
        gadget.debug.trace("Finished downloading a gadget: " + download_url);
        // gPlugins may have been updated during the download, so re-get the
        // plugin with its id.
        plugin = gPluginIdIndex[plugin.id];
        if (is_updating)
          plugin = plugin.copy_for_update;
        if (plugin) {
          if (gadgetBrowserUtils.saveGadget(plugin.id, request.responseBody)) {
            if (!is_updating) {
              if (AddPlugin(plugin) >= 0)
                SetDownloadStatus(plugin, kDownloadStatusAdded);
              else
                SetDownloadStatus(plugin, kDownloadStatusNone);
            } else {
              SetDownloadStatus(plugin, kDownloadStatusAdded);
            }
          } else {
            SetDownloadStatus(plugin, kDownloadStatusError);
          }
        }
      } else {
        gadget.debug.error("Download request " + download_url +
                           " returned status: " + request.status);
        SetDownloadStatus(plugin, kDownloadStatusError);
      }

      for (var i = 0; i < gActiveDownloads.length; i++) {
        if (gActiveDownloads[i] == request) {
          gActiveDownloads.splice(i, 1);
          break;
        }
      }
    }
  }
}

function ClearPluginDownloadTasks() {
  for (var i = 0; i < gActiveDownloads.length; i++) {
    gActiveDownloads[i].onreadystatechange = null;
    gActiveDownloads[i].abort();
  }
  gActiveDownloads = [];
}

function SearchPlugins(search_string, language) {
  if (!gPlugins[language] || !gPlugins[language][kCategoryAll])
    return [];

  debug.trace("search begins");
  var terms = search_string.toUpperCase().split(" ");
  var plugins = gPlugins[language][kCategoryAll];
  var result = [];
  if (plugins && plugins.length) {
    for (var i = 0; i < plugins.length; i++) {
      var plugin = plugins[i];
      var attrs = plugin.attributes;
      var indexable_text = [
        attrs.author ? attrs.author : "",
        plugin.updated_date ? plugin.updated_date.toLocaleDateString() : "",
        attrs.download_url ? attrs.download_url : "",
        attrs.info_url ? attrs.info_url : "",
        attrs.keywords ? attrs.keywords : "",
        GetPluginTitle(plugin, language),
        GetPluginDescription(plugin, language),
      ].join(" ").toUpperCase();

      var ok = true;
      for (var j in terms) {
        if (terms[j] && indexable_text.indexOf(terms[j]) == -1) {
          ok = false;
          break;
        }
      }
      if (ok) result.push(plugin);
    }
  }

  debug.trace("search ends");
  return result;
}

function ComparePluginID(p1, p2) {
  // Ascending order.
  return p1.id == p2.id ? 0 : p1.id > p2.id ? 1 : -1;
}

function ComparePluginDate(p1, p2) {
  var date1 = p1.updated_date.getTime();
  var date2 = p2.updated_date.getTime();
  // Descending order.
  return date1 == date2 ? ComparePluginID(p1, p2) : date1 < date2 ? 1 : -1;
}

function ComparePluginRank(p1, p2) {
  var rank1 = p1.rank;
  var rank2 = p2.rank;
  // Ascending order. Plugin with the lowest rank goes the first.
  return rank1 == rank2 ? ComparePluginID(p1, p2) : rank1 > rank2 ? 1 : -1;
}

function CompareRecency(p1, p2) {
  var date1 = p1.local_accessed_date.getTime();
  var date2 = p2.local_accessed_date.getTime();
  // Descending order.
  return date1 == date2 ? ComparePluginRank(p1, p2) : date1 < date2 ? 1 : -1;
}

function GetRecentlyUsedPlugins(language, result) {
  result.splice(0, result.length);
  var all_plugins = gPlugins[language][kCategoryAll];
  for (var i = 0; i < all_plugins.length; i++) {
    var plugin = all_plugins[i];
    // Copy plugin.accessed_date (constant) to plugin.local_accessed_date
    // to allow the downloader to change it, so we can see the just added
    // gadgets in the recently_used category.
    if (!plugin.local_accessed_date &&
        plugin.accessed_date && plugin.accessed_date.getTime() > 0)
      plugin.local_accessed_date = plugin.accessed_date;
    if (plugin.local_accessed_date)
      result.push(plugin);
  }
  result.sort(CompareRecency);
  if (result.length > kMaxRecentlyUsedPlugins) {
    result.splice(kMaxRecentlyUsedPlugins,
                  result.length - kMaxRecentlyUsedPlugins);
  }
}

function GetUpdatablePlugins(language, result) {
  result.splice(0, result.length);
  var all_plugins = gPlugins[language][kCategoryAll];
  for (var i = 0; i < all_plugins.length; i++) {
    var plugin = all_plugins[i];
    if (gadgetBrowserUtils.needUpdateGadget(plugin.id)) {
      // Make a copy of the plugin info because the updated status should be
      // independent from the added status.
      var plugin_copy_func = new Function();
      plugin_copy_func.prototype = plugin;
      var plugin_copy = new plugin_copy_func();
      plugin.copy_for_update = plugin_copy;
      result.push(plugin_copy);
    }
  }
  result.sort(ComparePluginDate);
}

function GetPluginsOfCategory(language, category) {
  if (!gPlugins[language] || !gPlugins[language][category])
    return [];

  var result = gPlugins[language][category];
  if (!result.sorted) {
    if (gPlugins[kAnyLanguage] && gPlugins[kAnyLanguage][category])
      Array.prototype.push.apply(result, gPlugins[kAnyLanguage][category]); 
    debug.trace("begin sorting");
    if (category == kCategoryRecentlyUsed) {
      GetRecentlyUsedPlugins(language, result);
    } else if (category == kCategoryNew) {
      result.sort(ComparePluginDate);
    } else if (category == kCategoryUpdates) {
      GetUpdatablePlugins(language, result);
    } else {
      result.sort(ComparePluginRank);
    }
    result.sorted = true;
    debug.trace("end sorting");
  }
  return result;
}
