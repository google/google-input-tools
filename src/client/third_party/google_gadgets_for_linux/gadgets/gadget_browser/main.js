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

// UI constants.
var kCategoryButtonHeight = category_active_img.height;
var kPluginBoxWidth = 134;
var kPluginBoxHeight = 124;
var kDefaultPluginRows = 3;
var kDefaultPluginColumns = 4;
var kCategoryGap = 15;
var kFixedExtraWidth = 142;
var kFixedExtraHeight = 183;
var kBorderMarginH = 12;
var kBorderMarginV = 30;
var kWindowMarginH = 12;
var kCategoryItemWidth = 140;

var kMinWidth = kFixedExtraWidth + 3 * kPluginBoxWidth + kBorderMarginH;
var kMinHeight = kFixedExtraHeight + kPluginBoxHeight + kBorderMarginV;

// Default layout: 714x555.
var gPluginRows = kDefaultPluginRows;
var gPluginColumns = kDefaultPluginColumns;
var gPluginsPerPage = gPluginRows * gPluginColumns;
var gPluginBoxGapX = 0;
var gPluginBoxGapY = 0;

var gCurrentCategory = "";
var gCurrentLanguage = "";
var gCurrentPageStartIndex = 0;
var gCurrentPlugins = [];

// Plugin download status.
var kDownloadStatusNone = 0;
var kDownloadStatusAdding = 1;
var kDownloadStatusAdded = 2;
var kDownloadStatusError = 3;

function init() {
  LoadMetadata();
  UpdateLanguageBox();
}

function init_layout() {
  var category_width = 0;
  for (var i in kTopCategories) {
    var category = kTopCategories[i];
    category_item_ruler.value = GetDisplayCategory(category);
    var width = category_item_ruler.idealBoundingRect.width;
    category_width = Math.max(category_width, width);
  }
  for (var i in kBottomCategories) {
    var category = kBottomCategories[i];
    category_item_ruler.value = GetDisplayCategory(category);
    var width = category_item_ruler.idealBoundingRect.width;
    category_width = Math.max(category_width, width);
  }
  kCategoryItemWidth = category_width;
  category_width = category_width + 20;
  if (category_width > categories_div.offsetWidth) {
    categories_div.width = category_width;
    plugins_div.x = categories_div.offsetX + category_width + 10;
    plugin_info_div.x = plugins_div.offsetX;
    kFixedExtraWidth = category_width + kWindowMarginH + 10;
    kMinWidth = kFixedExtraWidth + 3 * kPluginBoxWidth + kBorderMarginH;
    debug.trace("category_width:" + category_width);
    debug.trace("kFixedExtraWidth:" + kFixedExtraWidth);
  }
  view.removeElement(category_item_ruler);
}

function view_onopen() {
  var screen_width = system.screen.size.width;
  var screen_height = system.screen.size.height;
  var width, height;
  init_layout();
  if (screen_width >= 1024) {
    width = kFixedExtraWidth + 4 * kPluginBoxWidth + kBorderMarginH;
  } else if (screen_width >= 800) {
    width = kFixedExtraWidth + 3 * kPluginBoxWidth + kBorderMarginH;
  } else {
    width = kMinWidth;
  }
  if (screen_height >= 768) {
    height = kFixedExtraHeight + 3 * kPluginBoxHeight + kBorderMarginV;
  } else if (screen_height >= 600) {
    height = kFixedExtraHeight + 2 * kPluginBoxHeight + kBorderMarginV;
  } else {
    height = kMinHeight;
  }
  view.resizeTo(width, height);

  // We do the init in timer because gadgetBrowserUtils is not ready when
  // gadget is created and will be registered by c++ code right after the
  // gadget is created.
  setTimeout(init, 0);
}

function view_onclose() {
  ClearThumbnailTasks();
  ClearPluginDownloadTasks();
}

// Disable context menus.
function view_oncontextmenu() {
  event.returnValue = false;
}

function view_onsizing() {
  if (event.width < kMinWidth) {
    event.width = kMinWidth;
  }
  if (event.height < kMinHeight) {
    event.height = kMinHeight;
  }
}

function view_onsize() {
  var view_width = view.width;
  var view_height = view.height;
  bg_top_middle.width = view_width -
    bg_top_left.offsetWidth - bg_top_right.offsetWidth;
  bg_bottom_middle.width = view_width -
    bg_bottom_left.offsetWidth - bg_bottom_right.offsetWidth;
  bg_middle_left.height = view_height -
    bg_top_left.offsetHeight - bg_bottom_left.offsetHeight;
  bg_middle_middle.width = view_width -
    bg_middle_left.offsetWidth - bg_middle_right.offsetWidth;
  bg_middle_middle.height = view_height -
    bg_top_middle.offsetHeight - bg_bottom_middle.offsetHeight;
  bg_middle_right.height = view_height -
    bg_top_right.offsetHeight - bg_bottom_right.offsetHeight;

  window_body.width = view_width - kBorderMarginH;
  window_body.height = view_height - kBorderMarginV;
}

function window_onsize() {
  var plugins_width = window_body.width - kFixedExtraWidth;
  var plugins_height = window_body.height - kFixedExtraHeight;
  var columns = Math.floor(plugins_width / kPluginBoxWidth);
  var rows = Math.floor(plugins_height / kPluginBoxHeight);
  plugins_div.width = plugins_width;
  plugins_div.height = plugins_height;
  plugin_info_div.width = plugins_width - 6;
  categories_div.height = plugins_height + 28;
  language_box.height = Math.min(440, window_body.height - 30);
  gPluginBoxGapX = Math.floor((plugins_width - kPluginBoxWidth * columns) /
                              (columns + 1));
  gPluginBoxGapY = Math.floor((plugins_height - kPluginBoxHeight * rows) /
                              (rows + 1));
  if (rows != gPluginRows || columns != gPluginColumns) {
    gPluginRows = rows;
    gPluginColumns = columns;
    gPluginsPerPage = rows * columns;
    if (plugins_div.children.count > 0)
      SelectPage(gCurrentPageStartIndex);
  } else {
    var index = 0;
    for (var i = 0; i < rows; i++) {
      for (var j = 0; j < columns; j++) {
        if (index >= plugins_div.children.count)
          break;
        var box = plugins_div.children.item(index);
        box.x = Math.round(j * (kPluginBoxWidth + gPluginBoxGapX) +
                           gPluginBoxGapX / 2);
        box.y = Math.round(i * (kPluginBoxHeight + gPluginBoxGapY) +
                           gPluginBoxGapY / 2);
        index++;
      }
    }
  }
}

function language_label_onsize() {
  language_box_div.x = language_label.offsetX +
                       language_label.offsetWidth + 7;
}

function plugin_description_onsize() {
  var max_height = 2 * plugin_title.offsetHeight;
  if (plugin_description.offsetHeight > max_height) {
    plugin_description.height = max_height;
    plugin_other_data.y = plugin_description.offsetY + max_height;
  } else {
    plugin_other_data.y = plugin_description.offsetY +
                          plugin_description.offsetHeight + 2;
  }
}

function page_label_onsize() {
  if (page_label.innerText) {
    page_label.width = page_label.offsetWidth + 40;
    page_label.x = next_button.x - page_label.width;
    previous_button.x = page_label.x;
    page_label.onsize = null;
  }
}

function previous_button_onclick() {
  if (gCurrentPageStartIndex > gPluginsPerPage) {
    SelectPage(gCurrentPageStartIndex - gPluginsPerPage);
  } else if (gCurrentPageStartIndex > 0) {
    SelectPage(0);
  }
}

function next_button_onclick() {
  if (gCurrentPageStartIndex <  gCurrentPlugins.length - gPluginsPerPage)
    SelectPage(gCurrentPageStartIndex + gPluginsPerPage);
}

function language_box_onchange() {
  if (!gUpdatingLanguageBox)
    SelectLanguage(language_box.children.item(language_box.selectedIndex).name);
}

function GetDisplayLanguage(language) {
  return strings["LANGUAGE_" + language.replace("-", "_").toUpperCase()];
}

function GetDisplayCategory(category) {
  return strings["CATEGORY_" + category.toUpperCase()];
}

var gUpdatingLanguageBox = false;
function UpdateLanguageBox() {
  gUpdatingLanguageBox = true;

  language_box.removeAllElements();
  var languages = [];
  for (var language in gPlugins) {
    if (language != kAllLanguage) {
      var disp_lang = GetDisplayLanguage(language);
      if (disp_lang)
        languages.push({lang: language, disp: disp_lang});
    }
  }
  languages.sort(function(a, b) { return a.disp.localeCompare(b.disp); });
  languages.unshift({
    lang: kAllLanguage,
    disp: GetDisplayLanguage(kAllLanguage)
  });
  for (var i = 0; i < languages.length; i++) {
    var language = languages[i].lang;
    language_box.appendElement(
      "<item name='" + language +
      "'><label vAlign='middle' size='10'>" + languages[i].disp +
      "</label></item>");
  }
  language_box.selectedIndex = 0;
  SelectLanguage(kAllLanguage);
  gUpdatingLanguageBox = false;
}

function SelectLanguage(language) {
  gCurrentLanguage = language;
  UpdateCategories();
}

function AddCategoryButton(category, y) {
  categories_div.appendElement(
    "<label x='10' width='" + kCategoryItemWidth + "' height='" +
    kCategoryButtonHeight + "' y='" + y +
    "' align='left' vAlign='middle' enabled='true' color='#FFFFFF' name='" +
    category + "' size='10' trimming='character-ellipsis'" +
    " onmouseover='category_onmouseover()' onmouseout='category_onmouseout()'" +
    " onclick='SelectCategory(\"" + category + "\")'>" +
    GetDisplayCategory(category) + "</label>");
}

function category_onmouseover() {
  category_hover_img.y = event.srcElement.offsetY;
  category_hover_img.visible = true;
}

function category_onmouseout() {
  category_hover_img.visible = false;
}

function SelectCategory(category) {
  gCurrentCategory = category;
  if (category) {
    category_active_img.y = categories_div.children.item(category).offsetY;
    category_active_img.visible = true;
    gCurrentPlugins = GetPluginsOfCategory(gCurrentLanguage, gCurrentCategory);
    SelectPage(0);
    ResetSearchBox();
  } else {
    category_active_img.visible = false;
    gCurrentPlugins = [];
  }
}

function AddPluginBox(plugin, index, row, column) {
  var x = Math.round(column * (kPluginBoxWidth + gPluginBoxGapX) +
                     gPluginBoxGapX / 2);
  var y = Math.round(row * (kPluginBoxHeight + gPluginBoxGapY) +
                     gPluginBoxGapY / 2);
  var info_url = GetPluginInfoURL(plugin);
  var box = plugins_div.appendElement(
    "<div x='" + x + "' y='" + y +
    "' width='" + kPluginBoxWidth + "' height='" + kPluginBoxHeight +
    "' enabled='true' onmouseover='pluginbox_onmouseover(" + index + ")'" +
    " onmouseout='pluginbox_onmouseout(" + index + ")'>" +
    " <img width='100%' height='100%' stretchMiddle='true'/>" +
    (info_url ?
      " <a x='7' y='6' size='10' width='120' align='center' color='#FFFFFF'" +
      "  overColor='#FFFFFF' underline='false' trimming='character-ellipsis'" +
      "  onmouseover='plugin_title_onmouseover(" + index + ")'" +
      "  onmouseout='plugin_title_onmouseout(" + index + ")'/>" :
      " <label x='7' y='6' size='10' width='120' align='center' " +
      "  color='#FFFFFF' trimming='character-ellipsis'/>") +
    " <img x='16' y='75' opacity='70' src='images/thumbnails_shadow.png'/>" +
    " <div x='27' y='33' width='80' height='83' background='#FFFFFF'>" +
    "  <img width='80' height='60' src='images/default_thumbnail.jpg'" +
    "   cropMaintainAspect='true'/>" +
    "  <img y='60' width='80' height='60' src='images/default_thumbnail.jpg'" +
    "   flip='vertical' cropMaintainAspect='true'/>" +
    "  <img src='images/thumbnails_default_mask.png'/>" +
    " </div>" +
    " <button x='22' y='94' width='90' height='30' visible='false' size='10'" +
    "  color='#FFFFFF' stretchMiddle='true' trimming='character-ellipsis'" +
    "  downImage='images/add_button_down.png' " +
    "  overImage='images/add_button_hover.png'" +
    "  onmousedown='add_button_onmousedown(" + index + ")'" +
    "  onmouseover='add_button_onmouseover(" + index + ")'" +
    "  onmouseout='add_button_onmouseout(" + index + ")'" +
    "  onclick='add_button_onclick(" + index + ")'/>" +
    "</div>");

  // Set it here to prevent problems caused by special chars in the title.
  var title = box.children.item(1);
  title.innerText = GetPluginTitle(plugin, gCurrentLanguage);
  if (info_url)
    title.href = info_url;

  var thumbnail_element1 = box.children.item(3).children.item(0);
  var thumbnail_element2 = box.children.item(3).children.item(1);
  if (plugin.source == 1) { // built-in gadgets
    thumbnail_element1.src = plugin.attributes.thumbnail_url;
    thumbnail_element2.src = plugin.attributes.thumbnail_url;
  } else if (plugin.source == 2) { // from plugins.xml
    AddThumbnailTask(plugin, index, thumbnail_element1, thumbnail_element2);
  }

  plugin.button = box.children.item(4);
  UpdateAddButtonVisualStatus(plugin);
}

function SetDownloadStatus(plugin, status) {
  plugin.download_status = status;
  var button = plugin.button;
  if (!button)
    return;

  try {
    // Test if the button has been destroyed.
    button.enabled = button.enabled;
  } catch (e) {
    plugin.button = null;
    // The button has been destroyed, so don't update its visual status.
    return;
  }
  UpdateAddButtonVisualStatus(plugin);
}

var kAddingStatusLabels = [
  strings.ADD_BUTTON_LABEL,
  strings.STATUS_ADDING,
  strings.STATUS_ADDED,
  strings.STATUS_ERROR_ADDING,
];
var kUpdatingStatusLabels = [
  strings.UPDATE_BUTTON_LABEL,
  strings.STATUS_UPDATING,
  strings.STATUS_UPDATED,
  strings.STATUS_ERROR_UPDATING,
];
var kStatusButtonImages = [
  "images/add_button_default.png",
  "images/status_adding_default.png",
  "images/status_added_default.png",
  "images/status_failed_default.png",
];

function UpdateAddButtonVisualStatus(plugin) {
  var button = plugin.button;
  var status = plugin.download_status;
  var labels = gCurrentCategory == kCategoryUpdates ?
               kUpdatingStatusLabels : kAddingStatusLabels;
  button.caption = labels[status];
  button.image = kStatusButtonImages[status];
  // Don't disable the button when the download status is kDownloadStatusAdding
  // to ensure the button can get mouseout.

  if (status == kDownloadStatusNone) {
    button.overImage = "images/add_button_hover.png";
    button.visible = plugin.mouse_over;
  } else {
    button.visible = true;
    button.overImage = kStatusButtonImages[status];
  }
  button.downImage = status == kDownloadStatusAdding ?
      kStatusButtonImages[status] : "images/add_button_down.png";
}

function add_button_onmousedown(index) {
  // Reset the button status if the user clicks on it.
  if (gCurrentCategory != kCategoryUpdates)
    SetDownloadStatus(gCurrentPlugins[index], kDownloadStatusNone);
}

function add_button_onclick(index) {
  var plugin = gCurrentPlugins[index];
  if (plugin.download_status != kDownloadStatusAdding) {
    var is_updating = (gCurrentCategory == kCategoryUpdates);
    plugin.button = event.srcElement;
    SetDownloadStatus(plugin, kDownloadStatusAdding);
    if (gadgetBrowserUtils.needDownloadGadget(plugin.id)) {
      DownloadPlugin(plugin, is_updating);
    } else if (is_updating) {
      // The gadget has already been updated.
      SetDownloadStatus(plugin, kDownloadStatusAdded);
    } else {
      if (AddPlugin(plugin) >= 0)
        SetDownloadStatus(plugin, kDownloadStatusAdded);
      else
        SetDownloadStatus(plugin, kDownloadStatusNone);
    }
  }
}

function pluginbox_onmouseover(index) {
  MouseOverPlugin(event.srcElement, index);
}

function pluginbox_onmouseout(index) {
  MouseOutPlugin(event.srcElement, index);
}

function plugin_title_onmouseover(index) {
  MouseOverPlugin(event.srcElement.parentElement, index);
}

function plugin_title_onmouseout(index) {
  MouseOutPlugin(event.srcElement.parentElement, index);
}

function add_button_onmouseover(index) {
  if (gCurrentCategory != kCategoryUpdates &&
      gCurrentPlugins[index].download_status != kDownloadStatusAdding)
    SetDownloadStatus(gCurrentPlugins[index], kDownloadStatusNone);
  MouseOverPlugin(event.srcElement.parentElement, index);
}

function add_button_onmouseout(index) {
  if (gCurrentCategory != kCategoryUpdates &&
      gCurrentPlugins[index].download_status != kDownloadStatusAdding)
    SetDownloadStatus(gCurrentPlugins[index], kDownloadStatusNone);
  MouseOutPlugin(event.srcElement.parentElement, index);
}

function MouseOverPlugin(box, index) {
  var title = box.children.item(1);
  if (title.href) title.underline = true;

  box.children.item(0).src = "images/thumbnails_hover.png";
  box.children.item(3).children.item(2).src = "images/thumbnails_hover_mask.png";
  // Show the "Add" button.
  box.children.item(4).visible = true;

  var plugin = gCurrentPlugins[index];
  plugin_title.innerText = GetPluginTitle(plugin, gCurrentLanguage);
  plugin_description.innerText = GetPluginDescription(plugin, gCurrentLanguage);
  plugin_description.height = undefined;
  plugin_other_data.innerText = GetPluginOtherData(plugin);
  plugin.mouse_over = true;
}

function MouseOutPlugin(box, index) {
  box.children.item(1).underline = false;
  box.children.item(0).src = "";
  box.children.item(3).children.item(2).src = "images/thumbnails_default_mask.png";
  // Hide the "Add" button when it's in normal state.
  if (!gCurrentPlugins[index].download_status)
    box.children.item(4).visible = false;
  plugin_title.innerText = "";
  plugin_description.innerText = "";
  plugin_other_data.innerText = "";
  gCurrentPlugins[index].mouse_over = false;
}

function GetTotalPages() {
  if (!gCurrentPlugins || !gCurrentPlugins.length) {
    // Return 1 instead of 0 to avoid '1 of 0'.
    return 1;
  }
  return Math.ceil(gCurrentPageStartIndex / gPluginsPerPage) +
         Math.ceil((gCurrentPlugins.length - gCurrentPageStartIndex) /
                   gPluginsPerPage);
}

// Note for start: We need to keep the first plugin in the page unchanged
// when user resizes the window, so the first plugin will not always be at
// the whole page boundary.
function SelectPage(start) {
  plugins_div.removeAllElements();
  plugin_title.innerText = "";
  plugin_description.innerText = "";
  plugin_other_data.innerText = "";

  ClearThumbnailTasks();
  gCurrentPageStartIndex = start;
  var page = Math.ceil(start / gPluginsPerPage);
outer:
  for (var r = 0; r < gPluginRows; r++) {
    for (var c = 0; c < gPluginColumns; c++) {
      var i = start + c;
      if (i >= gCurrentPlugins.length)
        break outer;
      AddPluginBox(gCurrentPlugins[i], i, r, c);
    }
    start += gPluginColumns;
  }
  page_label.innerText = strings.PAGE_LABEL.replace("{{PAGE}}", page + 1)
                         .replace("{{TOTAL}}", GetTotalPages());
  previous_button.visible = next_button.visible = page_label.visible = true;
  view.setTimeout(RunThumbnailTasks, 500);
}

function UpdateCategories() {
  category_hover_img.visible = category_active_img.visible = false;
  var y = 0;
  for (var i = categories_div.children.count - 1; i >= 2; i--)
    categories_div.removeElement(categories_div.children.item(i));
  var language_categories = gPlugins[gCurrentLanguage];
  if (!language_categories)
    return;

  for (var i in kTopCategories) {
    var category = kTopCategories[i];
    if (category in language_categories) {
      AddCategoryButton(category, y);
      y += kCategoryButtonHeight;
    }
  }
  y += kCategoryGap;
  for (var i in kBottomCategories) {
    var category = kBottomCategories[i];
    if (category in language_categories) {
      AddCategoryButton(category, y);
      y += kCategoryButtonHeight;
    }
  }
  SelectCategory(kCategoryAll);
}

function ResetSearchBox() {
  search_box.value = strings.SEARCH_GADGETS;
  search_box.color = "#808080";
  search_box.killFocus();
}

var kSearchDelay = 500;
var gSearchTimer = 0;
var gInFocusHandler = false;

function search_box_onfocusout() {
  gInFocusHandler = true
  if (search_box.value == "")
    ResetSearchBox();
  gInFocusHandler = false;
}

function search_box_onfocusin() {
  gInFocusHandler = true;
  if (search_box.value == strings.SEARCH_GADGETS) {
    search_box.value = "";
    search_box.color = "#000000";
  }
  gInFocusHandler = false;
}

var gSearchPluginsTimer = 0;
function search_box_onchange() {
  if (gSearchTimer) cancelTimer(gSearchTimer);
  if (gInFocusHandler || search_box.value == strings.SEARCH_GADGETS)
    return;

  if (search_box.value == "") {
    SelectCategory(kCategoryAll);
    // Undo the actions in ResetSearchBox() called by SelectCategory().
    search_box.focus();
  } else {
    if (gSearchPluginsTimer)
      view.clearTimeout(gSearchPluginsTimer);
    gSearchPluginsTimer = setTimeout(function () {
      if (search_box.value && search_box.value != strings.SEARCH_GADGETS) {
        SelectCategory(null);
        gCurrentPlugins = SearchPlugins(search_box.value, gCurrentLanguage);
        SelectPage(0);
        gSearchPluginsTimer = 0;
      }
    }, kSearchDelay);
  }
}

