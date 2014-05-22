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

var g_change_by_color_edit = false;

function view_onopen() {
  EnableTab(e_color_tab, optionsViewData.enable_color);
  EnableTab(e_image_tab, optionsViewData.enable_image);
  var value = optionsViewData.value;
  if (!optionsViewData.enable_color ||
      (optionsViewData.enable_image && value.indexOf('.') != -1)) {
    image_tab_onclick();
  } else {
    color_tab_onclick();
    e_color_edit.value = value;
  }
}

function view_onok() {
  if (e_color_div.visible) {
    if (e_invalid_color.visible) {
      event.returnValue = false;
    } else {
      optionsViewData.value = e_color_edit.value;
    }
  } else {
    var selected = e_image_list.selectedItem;
    optionsViewData.value = selected ? selected.children.item(0).innerText : ""; 
  }
}

function color_tab_onclick() {
  UnselectTab(e_image_tab);
  SelectTab(e_color_tab);
  e_image_div.visible = false;
  EnsureColors();
  e_color_div.visible = true;
  e_color_edit.focus();
}

function image_tab_onclick() {
  UnselectTab(e_color_tab);
  SelectTab(e_image_tab);
  e_color_div.visible = false;
  EnsureImageList();
  e_image_div.visible = true;
  e_image_list.focus();
}

function EnableTab(tab, enabled) {
  tab.enabled = enabled;
  tab.opacity = enabled ? 255 : 128;
}

function SelectTab(tab) {
  tab.y = 7;
  tab.height = 23;
  tab.children.item(0).src = "images/tab_active.png";
}

function UnselectTab(tab) {
  tab.y = 9;
  tab.height = 20;
  tab.children.item(0).src = "images/tab_inactive.png";
}

var kColors = [
  "black", "dimgray", "gray", "darkgray", "silver", "lightgray", "#EAEAEA", "white",
  "red", "orange", "yellow", "lime", "cyan", "blue", "#9900FF", "magenta",
  "#FFCCCC", "#FFDD99", "#FFFFCC", "#CCFFCC", "#99FFFF", "#CCCCFF", "#DD99FF", "#FFCCFF",
  "#EE9999", "#FFCC66", "#EEEE66", "#99EE99", "#66DDDD", "#9999EE", "#CC66FF", "#FF66DD",
  "#DD6666", "#EE9933", "#CCCC33", "#66DD66", "#33CCCC", "#6666DD", "#9933EE", "#EE33CC",
  "#CC3333", "#CC6600", "#999933", "#00CC00", "#339999", "#3333CC", "#6600CC", "#CC3399",
  "#990000", "#993300", "#666600", "#009900", "#006666", "#000099", "#660099", "#990066",
  "#660000", "#663300", "#333300", "#006600", "#003333", "#000066", "#330066", "#660033"
];

function EnsureColors() {
  if (e_colors.children.count > 0)
    return;
  var index = 0;
  var y = 1;
  for (var i = 0; i < 8; i++) {
    var x = 1;
    for (var j = 0; j < 8; j++) {
      e_colors.appendElement("<div enabled='true' x='" + x + "' y='" + y +
          "' background='" + kColors[index] + "' width='29' height='19'" +
          " onclick='PickColor(" + index + ")'/>");
      index++;
      x += 30;
    }
    y += 20;
  }
}

function PickColor(i) {
  e_color_edit.value = kColors[i];
}

function color_edit_onchange() {
  g_change_by_color_edit = true;
  var color = designerUtils.parseColor(e_color_edit.value);
  if (!color) {
    e_opacity_edit.value = e_red_edit.value = e_green_edit.value =
        e_blue_edit.value = "";
    e_invalid_color.visible = true;
    e_color_display.background = "";
  } else {
    e_opacity_edit.value = color.opacity;
    e_red_edit.value = color.red;
    e_green_edit.value = color.green;
    e_blue_edit.value = color.blue;
    e_invalid_color.visible = false;
    e_color_display.background = e_color_edit.value;
  }
  g_change_by_color_edit = false;
}

function rgba_edit_onchange() {
  if (!g_change_by_color_edit) {
    e_color_edit.value = e_color_display.background =
        designerUtils.toColorString(e_red_edit.value, e_green_edit.value,
                                    e_blue_edit.value, e_opacity_edit.value);
  }
}

function EnsureImageList() {
  if (e_image_list.children.count > 0 || e_no_image.visible)
    return;

  for (var i = 0; i < optionsViewData.images.length; i++) {
    var file = optionsViewData.images[i];
    e_image_list.appendString(file);
    var label = e_image_list.children.item(i).children.item(0)
    label.tooltip = file;
    label.size = 9;
    label.width = "100%";
    label.trimming = "character-ellipsis";
    if (file == optionsViewData.value)
      e_image_list.selectedIndex = i;
  }

  if (e_image_list.children.count == 0) {
    e_no_image.visible = true;
  } else if (e_image_list.selectedIndex == -1) {
    e_image_list.selectedIndex = 0;
  }
}

function image_list_onchange() {
  if (e_image_list.selectedItem) {
    var image = e_image_list.selectedItem.children.item(0).innerText;
    e_image_display.src = e_image_display1.src = "gadget://" + image;
    e_image_display.x = e_image_display1.x =
        (e_image_display.parentElement.width - e_image_display.srcWidth) / 2;
    e_image_display.y = e_image_display1.y =
        (e_image_display.parentElement.height - e_image_display.srcHeight) / 2;
  } else {
    e_image_display.src = e_image_display1.src = "";
  }
}
