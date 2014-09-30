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

var kURLOption = "rss_url";
var kMaxItemsDefault = 10;
var kMinRefreshInterval = 600000; // every 10 minutes
var g_refresh_interval = kMinRefreshInterval +
                         Math.round(Math.random() / 3 * kMinRefreshInterval);

var kItemHeadingColor = "#ffffff";
var kItemSourceColor = "#00ffff";
var kItemTimeColor = "#00ffff";
var kWidthOffset = 12;
var kHeightOffset = 23;

var g_feed_items = null;
var g_feeds_loading = 0;

var g_feeds = options.getValue(kFeedListOption); // load saved feeds
if (!g_feeds) {
  g_feeds = new Array();
}

options.putDefaultValue(kMaxItemsOption, kMaxItemsDefault);
var g_max_items = options.getValue(kMaxItemsOption);

options.putDefaultValue(kURLOption, "unset");
var url = options.getValue(kURLOption);
if (url != "unset") {
  // Gadget added by GadgetManager
  url = NormalizeURL(url);
 } else {
  // Always add the default URL if:
  // user added gadget & no feeds are present in feeds list
  url = strings.GADGET_DEFAULTURL;
}
if (g_feeds.length == 0 && url != "unset") {
  var item = {};
  item.url = url;
  g_feeds.push(item);
}

options.putValue(kPrefsOption, false);

contents.contentFlags = gddContentFlagHaveDetails;

plugin.onAddCustomMenuItems = OnAddCustomMenuItems;
plugin.about_text = strings.GADGET_NAME + "\n\n"
  + strings.GADGET_COPYRIGHT + "\n\n" + strings.GADGET_DESC;

var g_refresh_timer = 0;
Refresh();

function OnAddCustomMenuItems(menu) {
  menu.AddItem(strings.GADGET_REFRESH, 0, RefreshMenuHandler);
}

function RefreshMenuHandler(item_text) {
  Refresh();
}

function OnSize() {
  contents.width = view.width - kWidthOffset;
  contents.height = view.height - kHeightOffset;
}

function Refresh() {
  // Check feeds_loading to make sure no feeds are currently loading.
  if (g_feeds.length > 0 && g_feeds_loading == 0) {
    gadget.debug.trace("Refreshing page.");

    UpdateCaption(strings.GADGET_LOADING);

    if (g_refresh_timer != 0)
      view.clearTimeout(g_refresh_timer);

    // Load in parallel.
    g_feeds_loading = g_feeds.length;
    for (var i = 0; i < g_feeds.length; i++) {
      var feed = g_feeds[i];
      LoadDocument(feed);
    }

    g_refresh_timer = view.setTimeout(Refresh, g_refresh_interval);
  }
}

function LoadDocument(feed) {
  var url = feed.url;

  gadget.debug.trace("Loading " + url);

  try {
    var xml_request = new XMLHttpRequest();
    xml_request.onreadystatechange = function() {
      ProcessRequest(xml_request, feed);
    }
    xml_request.open("GET", url, true);
    xml_request.send(null);
  } catch (e) {
    gadget.debug.error(e);
    UpdateCaption(strings.GADGET_ERROR);
  }
}

function ProcessRequest(xml_request, feed) {
  if (g_feeds_loading <= 0) {
    return; // cancelled
  }

  var readystate = xml_request.readyState;

  if (4 == readystate) { // complete
    if (200 == xml_request.status) {
      ParseDocument(xml_request, feed);
    } else {
      UpdateCaption(strings.GADGET_ERROR + ": " + xml_request.status);
    }

    if (--g_feeds_loading <= 0) {
      options.putValue(kFeedListOption, g_feeds);
    }
  }
}

function ParseDocument(xml_request, feed) {
  var xml = xml_request.responseXML;
  if (xml == null) {
    // Some sites may return the content as text/html format.
    // We try to parse it as XML here.
    xml = new DOMDocument();
    if (!xml.loadXML(xml_request.responseText)) {
      UpdateCaption(strings.GADGET_UNKNOWN);
      return;
    }
  }

  // Try various formats
  if (BuildAtomDoc(xml, feed)) {
    gadget.debug.trace("ATOM document parsed");
  } else if (BuildRSSDoc(xml, feed)) {
    gadget.debug.trace("RSS document parsed");
  } else { // Unsupported format.
    UpdateCaption(strings.GADGET_UNKNOWN);
    return;
  }

  gadget.debug.trace("finished parsing");
  DisplayFeedItems();
}

function BuildAtomDoc(xml, feed) {
  var feed_elem = GetElement("feed", xml);
  if (!feed_elem) return false;

  feed.title = GetElementText("title", feed_elem);
  feed.description = GetElementText("subtitle", feed_elem);
  feed.copyright = GetElementText("rights", feed_elem);
  feed_items = new Array();

  var items = xml.getElementsByTagName("entry");
  if (null != items) {
    var len  = items.length;
    for (var i = 0; i < len; i++) {
      var item_elem = items[i];
      var item = {};

      item.feed_title = feed.title;
      item.title = GetElementText("title", item_elem);
      item.url = GetElementAttrib("link", item_elem, "href");

      // description
      var desc = GetElementText("content", item_elem) ||
                 GetElementText("summary", item_elem);
      item.description = desc;

      var date = GetElementText("modified", item_elem) ||
                 GetElementText("updated", item_elem) ||
                 GetElementText("created", item_elem) ||
                 GetElementText("published", item_elem);
      item.date = ParseISO8601Date(date);

      feed_items[i] = item;
    }
  }

  feed.items = feed_items;
  UpdateGlobalItems(feed);

  return true;
}

function BuildRSSDoc(xml, feed) {
  if (feed.ttl && feed.ttl > g_refresh_interval) {
    feed.ttl -= g_refresh_interval;
    return true;
  }

  // Older feeds use "RDF"; newer ones use "rss".
  var rss_elem = GetElement("rss", xml);
  if (!rss_elem) {
    rss =  GetElement("rdf:RDF", xml);
    if (!rss_elem) return false;
    gadget.debug.trace("RDF document found");
  } else {
    gadget.debug.trace("RSS2 document found");
  }

  var chan_elem = GetElement("channel", rss_elem);
  if (!chan_elem) return false;

  feed.title = GetElementText("title", chan_elem);
  feed.description = GetElementText("description", chan_elem);
  var ttl = parseInt(GetElementText("ttl", chan_elem)) * 60 * 1000;
  if (ttl) {
    feed.ttl = ttl > kMinRefreshInterval ? ttl : kMinRefreshInterval;
  }
  feed.copyright = GetElementText("copyright", chan_elem);
  feed_items = new Array();

  // In older feeds, the "item" elements are under "rdf".
  // Newer ones have them under "channel," but fortunately
  // getElementsByTagName will search all nodes under rss/rdf tree
  // regardless of the item parent.
  var items = rss_elem.getElementsByTagName("item");
  if (null != items) {
    var len  = items.length;
    for (var i = 0; i < len; i++) {
      var item_elem = items[i];
      var item = {};

      item.feed_title = feed.title;
      item.title = GetElementText("title", item_elem);
      item.url = GetElementText("link", item_elem);
      item.description = GetElementText("description", item_elem);
      item.date = ParseDate(GetElementText("pubDate", item_elem));

      feed_items[i] = item;
    }
  }

  feed.items = feed_items;
  UpdateGlobalItems(feed);

  return true;
}

function UpdateGlobalItems(feed) {
  var items = feed.items;
  // Match based on URL. Could be more efficient here.
  var len  = items.length;
  gadget.debug.trace("Read " + len + " items.");

  if (len == 0) {
    return;
  }

  var feedtitle = feed.title;

  if (len > g_max_items) {
    items.splice(g_max_items, len - g_max_items);
    len = g_max_items;
  }
  // Newer items appear earlier in the array.
  for (var i = 0; i < len; i++) {
    var item = items[i];
    var olditem = FindFeedItem(item.url, feedtitle);
    if (olditem == null) {
      item.is_new = true;
      item.is_read = item.is_hidden = false;
    } else {
      item.is_new = false;
      item.is_read = olditem.is_read;
      item.is_hidden = olditem.is_hidden;
    }
  }

  // Reconstruct global item list
  g_feed_items = new Array();
  for (var i = 0; i < g_feeds.length; i++) {
    var itemlist = g_feeds[i].items;
    if (itemlist && itemlist.length > 0) {
      g_feed_items = g_feed_items.concat(itemlist);
    }
  }

  g_feed_items.sort(ItemCompare);
}

function GetElement(elem_name, parent) {
  for (var node = parent.firstChild; node; node = node.nextSibling) {
    if (node.nodeType == 1 && node.tagName == elem_name)
      return node;
  }
  return null;
}

function GetElementText(elem_name, parent) {
  var element = GetElement(elem_name, parent);
  return element ? element.text : "";
}

function GetElementAttrib(elem_name, parent, attr_name) {
  var element = GetElement(elem_name, parent);
  return element ? element.getAttribute(attr_name) : "";
}

function UpdateCaption(text) {
  if (!text) {
    view.caption = strings.GADGET_NAME;
  } else {
    view.caption = text;
  }
  if (g_feeds.length > 0 && 
      (!g_feed_items || g_feed_items.length == 0)) {
    // Display the text as a static content item if there is no real item.
    contents.removeAllContentItems();
    var item = new ContentItem();
    item.heading = text;
    item.headingColor = kItemHeadingColor;
    item.flags = gddContentItemFlagStatic;
    contents.addContentItem(item, 0);
  }
}

function DisplayFeedItems() {
  contents.removeAllContentItems();

  if (g_feeds.length == 1) {
    var feed = g_feeds[0];
    UpdateCaption(feed.title); 
    plugin.about_text = feed.title + "\n\n" + feed.copyright + "\n\n"
      + feed.description;

  } else {
    UpdateCaption(null);
    plugin.about_text = strings.GADGET_NAME + "\n\n" 
      + strings.GADGET_COPYRIGHT + "\n\n" + strings.GADGET_DESC;
  }

  if (g_feed_items) {
    // Append in reverse.
    var len = g_feed_items.length;
    for (var i = len - 1; i >= 0; i--) {
      var item = g_feed_items[i];

      if (item.is_hidden) {
        continue;
      }

      var c_item = new ContentItem();
      c_item.heading = c_item.tooltip = item.title;
      c_item.snippet = item.description;
      c_item.source = item.feed_title;
      c_item.open_command = item.url;
      c_item.headingColor = kItemHeadingColor;
      c_item.sourceColor = kItemSourceColor;
      c_item.timeColor = kItemTimeColor;

      if (!item.is_read) {
        c_item.flags |= gddContentItemFlagHighlighted;
      }

      c_item.time_created = item.date;// getvardate?
      // gadget.debug.trace("print time " + c_item.time_created.toString());

      c_item.onDetailsView = OnDetailsView;
      c_item.onRemoveItem = OnRemoveItem;

      c_item.layout = gddContentItemLayoutNews;

      var options = gddItemDisplayInSidebar;
      if (item.is_new) {
        options |= gddItemDisplayAsNotification;
        c_item.flags = gddContentItemFlagHighlighted;
      }
      contents.addContentItem(c_item, options);
    }
  }
}

function MarkItemAsRead(c_item) {
  c_item.flags &= ~gddContentItemFlagHighlighted;

  var item = FindFeedItem(c_item.open_command, c_item.source);
  if (item != null) {
    item.is_read = true;
  }
}

function OnDetailsView(item) {
  var html = "<html><meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">";
  html += "<head><style type=\"text/css\">body {font-size: 12px;}</style></head><body>";
  html += item.snippet;
  html += "</body></html>";

  var control = new DetailsView();
  control.SetContent("", item.time_created, html, true, item.layout);
  control.html_content = true;

  var details = new Object();
  // Let the native code set the title automatically. The native code knows
  // how to strip HTML if the content item is in HTML.
  // details.title = item.heading;
  details.details_control = control;
  details.flags = gddDetailsViewFlagRemoveButton |
                  gddDetailsViewFlagToolbarOpen;

  MarkItemAsRead(item);

  return details;
}

function OnRemoveItem(c_item) {
  var item = FindFeedItem(c_item.open_command, c_item.source);
  if (item != null) {
    item.is_hidden = true;
  }

  return false;
}

function FindFeedItem(url, feedtitle) {
  var i = FindFeedItemIndex(url, feedtitle);
  if (i != -1) {
    return g_feed_items[i];
  }

  return null;
}

function FindFeedItemIndex(url, feedtitle) {
  if (null != g_feed_items) {
    for (var j = 0; j < g_feed_items.length; j++) {
      var item = g_feed_items[j];
      if (feedtitle == item.feed_title && url == item.url) {
        return j;
      }
    }
  }

  return -1;
}

function FeedListUpdated() {
  if (event.propertyName == kPrefsOption && options.getValue(kPrefsOption)) {
    options.putValue(kPrefsOption, false);
    g_feeds_loading = 0; // cancel all loads
    g_feeds = options.getValue(kFeedListOption);
    g_max_items = options.getValue(kMaxItemsOption);
    UpdateCaption(null);
    contents.removeAllContentItems();
    contents.maxContentItems = g_max_items * g_feeds.length;
    Refresh();
  }
}

function ParseDate(date) {
  var d = new Date();
  if ("" != date) {
    var time = Date.parse(date);
    d.setTime(time);
  }
  return d;
}

// Based on http://delete.me.uk/2005/03/iso8601.html
function ParseISO8601Date(date) {
  var regexp = "([0-9]{4})(-([0-9]{2})(-([0-9]{2})";
  regexp += "(T([0-9]{2}):([0-9]{2})(:([0-9]{2})(\.([0-9]+))?)?";
  regexp += "(Z|(([-+])([0-9]{2}):([0-9]{2})))?)?)?)?";

  var arr = date.match(new RegExp(regexp));
  var offset = 0;
  var d = new Date(arr[1], 0, 1);

  if (arr[3]) {
    d.setMonth(arr[3] - 1);
  }
  if (arr[5]) {
    d.setDate(arr[5]);
  }
  if (arr[7]) {
    d.setHours(arr[7]);
  }
  if (arr[8]) {
    d.setMinutes(arr[8]);
  }
  if (arr[10]) {
    d.setSeconds(arr[10]);
  }
  if (arr[12]) {
    d.setMilliseconds(Number(arr[12]) / 1000);
  }
  if (arr[14]) {
    offset = (Number(arr[16]) * 60) + Number(arr[17]);
    if (arr[15] != '-') {
      offset *= -1;
    }
  }

  offset -= d.getTimezoneOffset();
  d.setTime(d.getTime() + (offset * 60000));

  return d;
}

function FindFeedByURL(url) {
  for (var i = 0; i < g_feeds.length; i++) {
    if (url == g_feeds[i].url) {
      return i;
    }
  }
  return -1;
}

function FindFeedByTitle(title) {
  for (var i = 0; i < g_feeds.length; i++) {
    if (title == g_feeds[i].title) {
      return i;
    }
  }
  return -1;
}

function ItemCompare(a, b) {
  // g_feed_items have items arranged in descending order by date
  return b.date - a.date;
}
