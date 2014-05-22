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

var g_feeds;
var g_max_items;

SetButtonEnabled(addbutton, false);
SetButtonEnabled(removebutton, false);

function OnSize() {
  feedname_div.width = view.width - addbutton.offsetWidth - 5;
  feedname.width = feedname_div.offsetWidth - 2;
  feeds_div.height = view.height - 105;
  feeds.height = feeds_div.offsetHeight - 2;
  feeds.width = feeds_div.offsetWidth - 2;
  removebutton.y = feeds_div.offsetY + feeds_div.offsetHeight + 5;
  maxitems_label.y = removebutton.offsetY + removebutton.offsetHeight + 5;
  maxitems_div.y = maxitems_label.offsetY;
}

function OnOpen() {
  g_max_items = options.getValue(kMaxItemsOption);
  g_feeds = options.getValue(kFeedListOption);
  if (!g_feeds) {
    g_feeds = new Array();
  }

  // fill listbox
  for (var i = 0; i < g_feeds.length; i++) {
    AddFeedToList(g_feeds[i]);
  }

  // fill edit box
  maxitems.value = g_max_items + "";
}

function SelectedFeedChanged() {
  SetButtonEnabled(removebutton, feeds.selectedIndex != -1);
}

function FeedBoxChanged() {
  SetButtonEnabled(addbutton, !!feedname.value);
}

function OKClicked() {
  g_max_items = parseInt(maxitems.value);
  if (g_max_items > 0) {
    options.putValue(kMaxItemsOption, g_max_items);
  }

  options.putValue(kFeedListOption, g_feeds);
  options.putValue(kPrefsOption, true);
}

function AddFeed() {
  var url = feedname.value;
  feedname.value = "";
  url = TrimString(NormalizeURL(url));
  if (!url) {
    return;
  }
  if (url.indexOf("://") == -1) {
    url = "http://" + url;
  }

  var feed = {};
  feed.url = url;
  g_feeds.push(feed);
  AddFeedToList(feed);
}

function RemoveFeed() {
  var index = feeds.selectedIndex;
  if (index == -1) { // shouldn't happen
    gadget.trace.error("Invalid feed to remove.");
  }

  feedname.value = g_feeds[index].url;

  g_feeds.splice(index, 1);
  feeds.removeElement(feeds.selectedItem);

  if (feeds.selectedIndex == -1) {
    SetButtonEnabled(removebutton, false);
  }
}

function SetButtonEnabled(button, value) {
  button.enabled = value;
  button.opacity = value ? 255 : 127;
}

function AddFeedToList(feed) {
  gadget.debug.trace("Adding feed: " + feed.url);
  var title = feed.title;
  if (!title) {
    title = strings.GADGET_UNTITLED;
  }
  var item = title +  "\n" + feed.url;
  feeds.appendString(item);
}
