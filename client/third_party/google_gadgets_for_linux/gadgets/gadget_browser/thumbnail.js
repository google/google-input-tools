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

var kThumbnailPrefix = "http://desktop.google.com/plugins/images/";
var kMaxWorkingTasks = 4;
var gPendingTasks = [];
var gWorkingTasks = [];

function ClearThumbnailTasks() {
  gPendingTasks = [];
  for (var i in gWorkingTasks) {
    gWorkingTasks[i].request.onreadystatechange = null;
    gWorkingTasks[i].request.abort();
  }
  gWorkingTasks = [];
}

function AddThumbnailTask(plugin, index, thumbnail_element1, thumbnail_element2) {
  var thumbnail_url = plugin.attributes.thumbnail_url;
  if (!thumbnail_url)
    return;

  var image_data = gadgetBrowserUtils.loadThumbnailFromCache(thumbnail_url);
  if (image_data) {
    // Show the cached image first, and then check if it needs to be updated.
    thumbnail_element1.src = image_data;
    thumbnail_element2.src = image_data;
    if (plugin.thumbnail_checked)
      return;
  }

  var full_thumbnail_url = thumbnail_url.match(/^https?:\/\//) ?
                           thumbnail_url : kThumbnailPrefix + thumbnail_url;
  var new_task = {
    plugin: plugin,
    index: index,
    thumbnail_url: thumbnail_url,
    full_thumbnail_url: full_thumbnail_url,
    thumbnail_element1: thumbnail_element1,
    thumbnail_element2: thumbnail_element2
  };
  gPendingTasks.push(new_task);
}

function RunThumbnailTasks() {
  while (gWorkingTasks.length < kMaxWorkingTasks && gPendingTasks.length > 0)
    StartThumbnailTask(gPendingTasks.shift());
}

function StartThumbnailTask(task) {
  gadget.debug.trace("Start loading thumbnail: " + task.full_thumbnail_url +
                     " index=" + task.index);

  var request = new XMLHttpRequest();
  try {
    request.open("GET", task.full_thumbnail_url);
    task.request = request;

    var cached_date =
        gadgetBrowserUtils.getThumbnailCachedDate(task.thumbnail_url);
    if (cached_date && cached_date.getTime() > 0) {
      request.setRequestHeader("If-Modified-Since",
                               // Windows compatibility: Windows JScript engine
                               // returns UTC string ended with "UTC", which
                               // doesn't conform to RFC2616.
                               cached_date.toUTCString().replace("UTC", "GMT"));
    }

    request.onreadystatechange = OnRequestStateChange;
    request.send();
    gWorkingTasks.push(task);
  } catch (e) {
    gadget.debug.error("Request error: " + e);
  }

  function OnRequestStateChange() {
    if (request.readyState == 4) {
      if (request.status == 200) {
        gadget.debug.trace("Finished loading a thumbnail: " +
                           task.full_thumbnail_url + " index=" + task.index);
        var data = request.responseStream;
        task.thumbnail_element1.src = data;
        task.thumbnail_element2.src = data;
        if (task.thumbnail_element1.srcWidth > 0) {
          // Save the thumbnail only if the image is good.
          gadgetBrowserUtils.saveThumbnailToCache(task.thumbnail_url, data);
          task.plugin.thumbnail_checked = true;
        }
      } else if (request.status == 304) {
        gadget.debug.trace("Thumbnail not modified: " +
                           task.full_thumbnail_url);
        task.plugin.thumbnail_checked = true;
      } else {
        gadget.debug.error("Request " + task.full_thumbnail_url +
                           " returned status: " + request.status);
      }

      for (var i = 0; i < gWorkingTasks.length; i++) {
        if (gWorkingTasks[i] == task) {
          gWorkingTasks.splice(i, 1);
          break;
        }
      }
      // Start left pending tasks.
      RunThumbnailTasks();
    }
  }
}
