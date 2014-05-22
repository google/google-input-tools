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

var kMinX = -2000;
var kMaxX = 2000;
var kMinY = -2000;
var kMaxY = 2000;
var kMinWidth = 0;
var kMinHeight = 0;
var kMaxWidth = 2000;
var kMaxHeight = 2000;
var kSelectionBorderWidth = 6;
var kMainXML = "main.xml";
var kOptionsXML = "options.xml";
var kOptionsJS = "options.js";
var kGadgetGManifest = "gadget.gmanifest";

var kMinViewWidth = 720;
var kMinViewHeight = 400;

var g_shift_down = false;
var g_ctrl_down = false;
var g_selected_element = null;
var g_drag_offset_x = 0;
var g_drag_offset_y = 0;
var g_offset_angle = 0;
var g_dragging = false;
var g_size_width_dir = 0;
var g_size_height_dir = 0;
var g_current_tool = -1;
var g_gadget_file_manager = null;
var g_gadget_file = null;
var g_gadget_changed = false;
var g_current_file = null;
var g_current_file_time = 0;
var g_current_file_changed = false;
var g_extracted_files = { };
var g_extracted_files_time = { };
var g_global_file_manager = designerUtils.getGlobalFileManager();
var g_new_created = false;
var g_tool_button_down = false;
var g_changed_since_save_undo = false;

var kMaxUndoDepth = 20;
var g_undo_stack = [ ];
var g_undo_stack_pos = 0;
var g_undo_stack_pos_saved = 0;

var kGadgetFileFilterGGOnly = Strings.TYPE_ALL_GADGET + "|*.gg"
var kGadgetFileFilter =
    Strings.TYPE_ALL_GADGET + "|*.gg;" + kGadgetGManifest + "|" +
    Strings.TYPE_GG + "|*.gg|" +
    Strings.TYPE_GMANIFEST + "|" + kGadgetGManifest;

var kTools = [
  "div", "img", "button", "edit", "checkbox", "radio", "a", "label",
  "contentarea", "listbox", "combobox", "progressbar", "scrollbar"
];

var g_elements_view = null;
var g_properties_view = null;
var g_events_view = null;

var g_close_by_close_button = false;

// TODO: config.
var g_system_editors = {
  xml: "gedit",
  js: "gedit",
  gmanifest: "gedit",
  png: "gimp",
  gif: "gimp",
  jpeg: "gimp",
  jpg: "gimp"
};

function ResetGlobals() {
  ResetViewGlobals();
  g_shift_down = false;
  g_ctrl_down = false;
  g_drag_offset_x = 0;
  g_drag_offset_y = 0;
  g_offset_angle = 0;
  g_dragging = false;
  g_size_width_dir = 0;
  g_size_height_dir = 0;
  SelectTool(-1);
  SelectElement(null);
  e_designer_view.removeAllElements();
  e_properties_list.removeAllElements();
  e_events_list.removeAllElements();
  g_elements_view = null;
  g_properties_view = null;
  g_events_view = null;
  g_current_file_changed = false;
  g_tool_button_down = false;
  g_changed_since_save_undo = false;
}

function view_onopen() {
  InitMetadata();
  plugin.onAddCustomMenuItems = AddGlobalMenu;
  InitToolBar();
  NewGadget();
  e_selection.oncontextmenu = AddElementContextMenu;
  for (var i = 0; i < e_selection.children.count; i++)
    e_selection.children.item(i).oncontextmenu = AddElementContextMenu;
}

function view_onclose() {
  if (!g_close_by_close_button)
    CheckSaveGadget(false);
}

function view_onsizing() {
  if (event.width < kMinViewWidth) {
    event.width = kMinViewWidth;
  }
  if (event.height < kMinViewHeight) {
    event.height = kMinViewHeight;
  }
}

var g_view_onsize_timer = 0;
function view_onsize() {
  // The borders are updated not delayed.
  var v_width = view.width;
  var v_height = view.height;

  e_border_top_middle.width = v_width -
    e_border_top_left.offsetWidth - e_border_top_right.offsetWidth;

  e_border_bottom_middle.width = v_width -
    e_border_bottom_left.offsetWidth - e_border_bottom_right.offsetWidth;

  e_border_middle_left.height = v_height -
    e_border_top_left.offsetHeight - e_border_bottom_left.offsetHeight;

  e_border_middle_right.height = v_height -
    e_border_top_right.offsetHeight - e_border_bottom_right.offsetHeight;

  if (!g_view_onsize_timer) {
    // Use a timer to lower the priority of reaction of onsize.
    g_view_onsize_timer = view.setTimeout(function() {
      g_view_onsize_timer = 0;
      var net_width = view.width - e_border_middle_left.offsetWidth -
        e_border_middle_right.offsetWidth;
      var net_height = view.height - e_border_top_middle.offsetHeight -
        e_border_bottom_middle.offsetHeight;
      e_whole_ui.width = net_width;
      e_whole_ui.height = net_height;
      var main_height = net_height - e_toolbar.offsetHeight;
      e_main_ui.height = main_height;
      e_designer.width = e_tool_views.x = net_width - e_tool_views.offsetWidth;

      UpdateDesignerViewPos();

      var each_height = Math.floor(main_height / 3);
      with (e_tool_views.children) {
        item(1).height = item(3).height = each_height - 20;
        item(2).y = each_height;
        item(3).y = each_height + 20;
        var double_height = each_height * 2;
        item(5).height = main_height - double_height - 20;
        item(4).y = double_height;
        item(5).y = double_height + 20;
      }
    }, 0);
  }
}

function view_onmouseover() {
  CheckExternalFileChange();
}

function view_oncontextmenu() {
  AddGlobalMenu(event.menu);
  event.returnValue = false;
}

function view_onkeydown() {
  switch (event.keyCode) {
    case 16: // shift
      g_shift_down = true;
      break;
    case 17: // ctrl
      g_ctrl_down = true;
      break;
    case 'S'.charCodeAt(0):
      SaveCurrentFile();
      break;
    case 'Z'.charCodeAt(0):
      Undo();
      break;
    case 'Y'.charCodeAt(0):
      Redo();
      break;
  }
}

function view_onkeyup() {
  switch (event.keyCode) {
    case 16: // shift
      g_shift_down = false;
      break;
    case 17: // ctrl
      g_ctrl_down = false;
      break;
  }
}

function close_button_onclick() {
  if (CheckSaveGadget(true)) {
    g_close_by_close_button = true;
    plugin.RemoveMe(true);
  }
}

function tool_label_onsize(index) {
  var label = event.srcElement;
  var tool = label.parentElement;
  tool.width = Math.max(label.offsetWidth + 6, 30);
  if (index == 0) {
    tool.x = e_file_list.offsetX + e_file_list.offsetWidth + 10;
  } else {
    var prev_tool = e_toolbar.children.item(index - 1);
    tool.x = prev_tool.x + prev_tool.width;
  }
}

function file_list_onchange() {
  var item = e_file_list.selectedItem;
  if (item) {
    var filename = item.children.item(0).innerText;
    if (filename && filename != g_current_file)
      OpenFile(filename);
  }
}

function designer_view_onsize() {
  if (!g_dragging)
    UpdateDesignerViewPos();
}

function properties_list_onchange() {
  g_properties_view.OnSelectionChange();
}

function events_list_onchange() {
  g_events_view.OnSelectionChange();
}

function elements_list_onchange() {
  if (e_elements_list.selectedItem) {
    var id = g_elements_view.GetIdByNode(e_elements_list.selectedItem);
    SelectElement(g_elements_info[id]);
  } else {
    SelectElement(null);
  }
}

function elements_list_onkeydown() {
  if (event.keyCode == 46)  // delete
    RemoveSelectedElement();
}

function designer_panel_onmousedown() {
  if (event.button == 1) {
    var x = event.x - e_designer_view.offsetX;
    var y = event.y - e_designer_view.offsetY;
    if (g_current_tool != -1) {
      SelectElement(NewElement(kTools[g_current_tool], x, y));
      CurrentFileChanged();
      SaveUndoState();
      SelectTool(-1);
    } else {
      var element = FindElementByPos(e_designer_view, x, y);
      if (element) {
        g_drag_offset_x = x;
        g_drag_offset_y = y;
        SelectElement(GetElementInfo(element));
        e_designer_panel.cursor = "sizeall";
        g_dragging = true;
      } else {
        SelectElement(null);
      }
    }
  }
}

function designer_panel_onmousemove() {
  if (g_dragging && !(event.button & 1)) {
    e_designer_panel.cursor = "arrow";
    EndDragging();
  }
  if (g_selected_element && g_dragging) {
    DragMove(event.x - e_designer_view.offsetX,
             event.y - e_designer_view.offsetY);
  }
}

function designer_panel_onmouseup() {
  designer_panel_onmousemove();
  e_designer_panel.cursor = "arrow";
  EndDragging();
}

var g_save_undo_pending = 0;
function selection_onkeydown() {
  var size_move_func = g_shift_down ?
                       ResizeSelectedElement : MoveSelectedElement;
  var delta = null;
  switch (event.keyCode) {
    case 37: // left
    case 0x64: // numpad 4
      delta = size_move_func(-1, 0);
      break;
    case 38: // up
    case 0x68: // numpad 8
      delta = size_move_func(0, -1);
      break;
    case 39: // right
    case 0x66: // numpad 6
      delta = size_move_func(1, 0);
      break;
    case 40: // down
    case 0x62: // numpad 2
      delta = size_move_func(0, 1);
      break;
    case 46: // delete
      RemoveSelectedElement();
      break;
  }

  if (!g_dragging && delta && (delta.x != 0 || delta.y != 0)) {
    if (g_save_undo_pending == 0) {
      // Use a timer to combine multiple keyboard changes into one. 
      view.setInterval(function() {
        // If g_save_undo_pending is not set to 1 after 1 interval, do
        // actual save.
        if (g_save_undo_pending == 2) {
          if (!g_dragging) // Don't save if already in another dragging.
            SaveUndoState();
          view.clearInterval(event.cookie);
          g_save_undo_pending = 0;
        } else {
          g_save_undo_pending = 2;
        }
      }, 500);
    }
    g_save_undo_pending = 1;
  }
}

function selection_onmousedown() {
  if (event.button == 1) {
    var parent_pos = ElementCoordToParent(e_selection, event.x, event.y);
    g_drag_offset_x = parent_pos.x - e_designer_view.offsetX;
    g_drag_offset_y = parent_pos.y - e_designer_view.offsetY;
    var new_selected = FindElementByPos(e_designer_view,
                                        g_drag_offset_x, g_drag_offset_y);
    if (new_selected && new_selected != g_selected_element)
      SelectElement(GetElementInfo(new_selected));
    e_selection.cursor = "sizeall";
    g_dragging = true;
  }
}

function selection_onmousemove() {
  if (g_dragging && !(event.button & 1)) {
    e_selection.cursor = "arrow";
    EndDragging();
  }
  if (g_selected_element && g_dragging) {
    var parent_pos = ElementCoordToParent(e_selection, event.x, event.y);
    DragMove(parent_pos.x - e_designer_view.offsetX,
             parent_pos.y - e_designer_view.offsetY);
  }
}

function selection_onmouseup() {
  selection_onmousemove();
  e_selection.cursor = "arrow";
  EndDragging();
}

function selection_border_onmousedown() {
  if (event.button == 1) {
    var panel_pos = designerUtils.elementCoordToAncestor(event.srcElement,
                                                         e_designer_panel,
                                                         event.x, event.y);
    g_drag_offset_x = panel_pos.x - e_designer_view.offsetX;
    g_drag_offset_y = panel_pos.y - e_designer_view.offsetY;
    g_dragging = true;
  }
}

function selection_border_onmousemove() {
  if (g_dragging && !(event.button & 1))
    EndDragging();
  if (g_selected_element && g_dragging) {
    var panel_pos = designerUtils.elementCoordToAncestor(event.srcElement,
                                                         e_designer_panel,
                                                         event.x, event.y);
    DragMove(panel_pos.x - e_designer_view.offsetX,
             panel_pos.y - e_designer_view.offsetY);
  }
}

function selection_border_onmouseup() {
  selection_border_onmousemove();
  EndDragging();
}

function size_onmousedown(width_dir, height_dir) {
  if (event.button == 1 && g_selected_element) {
    var panel_pos = designerUtils.elementCoordToAncestor(event.srcElement,
                                                         e_designer_panel,
                                                         event.x, event.y);
    g_size_width_dir = width_dir;
    g_size_height_dir = height_dir;
    g_drag_offset_x = panel_pos.x - e_designer_view.offsetX;
    g_drag_offset_y = panel_pos.y - e_designer_view.offsetY;
    g_dragging = true;
  }
}

function size_onmousemove() {
  if (g_dragging && !(event.button & 1))
    EndDragging();
  if (g_selected_element && g_dragging) {
    var panel_pos = designerUtils.elementCoordToAncestor(event.srcElement,
                                                         e_designer_panel,
                                                         event.x, event.y);
    var x = panel_pos.x - e_designer_view.offsetX;
    var y = panel_pos.y - e_designer_view.offsetY;
    var delta = ViewDeltaToElement(g_selected_element,
                                   x - g_drag_offset_x, y - g_drag_offset_y);
    var size_delta = ResizeSelectedElement(delta.x * g_size_width_dir,
                                           delta.y * g_size_height_dir);
    // Dragging the top, left or topleft sizing point changes both size and
    // position.
    MoveSelectedElementInElementCoord(
        g_size_width_dir == -1 ? -size_delta.x : 0,
        g_size_height_dir == -1 ? -size_delta.y : 0);
    var real_delta = ElementDeltaToView(g_selected_element,
                                        size_delta.x * g_size_width_dir,
                                        size_delta.y * g_size_height_dir);
    g_drag_offset_x += real_delta.x;
    g_drag_offset_y += real_delta.y;
  }
}

function size_onmouseup() {
  size_onmousemove();
  EndDragging();
}

function pin_onmousedown() {
  if (event.button == 1) {
    g_drag_offset_x = event.x + e_pin.offsetX - e_designer_view.offsetX;
    g_drag_offset_y = event.y + e_pin.offsetY - e_designer_view.offsetY;
    g_dragging = true;
  }
}

function pin_onmousemove() {
  if (g_dragging && !(event.button & 1))
    EndDragging();
  if (g_selected_element && g_dragging) {
    var x = event.x + e_pin.offsetX - e_designer_view.offsetX;
    var y = event.y + e_pin.offsetY - e_designer_view.offsetY;
    var delta = ViewDeltaToElement(g_selected_element,
                                   x - g_drag_offset_x, y - g_drag_offset_y);
    if (delta.x != 0) {
      var new_pin_x = designerUtils.getOffsetPinX(g_selected_element) + delta.x;
      g_selected_element.pinX = new_pin_x;
      e_selection.pinX = new_pin_x + kSelectionBorderWidth;
      g_properties_view.SyncPropertyFromElement("pinX");
      CurrentFileChanged();
    }
    if (delta.y != 0) {
      var new_pin_y = designerUtils.getOffsetPinY(g_selected_element) + delta.y;
      g_selected_element.pinY = new_pin_y;
      e_selection.pinY = new_pin_y + kSelectionBorderWidth;
      g_properties_view.SyncPropertyFromElement("pinY");
      CurrentFileChanged();
    }
    if (delta.x != 0 || delta.y != 0) {
      MoveSelectedElementInElementCoord(delta.x, delta.y);
      g_drag_offset_x = x;
      g_drag_offset_y = y;
      UpdateStatusLine();
    }
  }
}

function pin_onmouseup() {
  pin_onmousemove();
  EndDragging();
}

function rotation_onmousedown() {
  if (event.button == 1 && g_selected_element) {
    var pos = ElementCoordToParent(e_selection,
                                   event.x + event.srcElement.offsetX,
                                   event.y + event.srcElement.offsetY);
    g_offset_angle = Math.atan2(e_pin.x - pos.x, pos.y - e_pin.y)
                         * 180 / Math.PI -
                     e_selection.rotation;
    g_dragging = true;
  }
}

function rotation_onmousemove() {
  if (g_dragging && !(event.button & 1))
    EndDragging();
  if (g_selected_element && g_dragging) {
    var pos = ElementCoordToParent(e_selection,
                                   event.x + event.srcElement.offsetX,
                                   event.y + event.srcElement.offsetY);
    var angle = Math.atan2(e_pin.x - pos.x, pos.y - e_pin.y) * 180 / Math.PI;
    angle -= g_offset_angle;
    angle = Math.round(angle);
    angle %= 360;
    if (angle < 0) angle += 360;
    var new_rotation = ViewRotationToElement(g_selected_element, angle);
    if (new_rotation != g_selected_element.rotation) {
      g_selected_element.rotation = new_rotation;
      e_selection.rotation = angle;
      g_properties_view.SyncPropertyFromElement("rotation");
      CurrentFileChanged();
      UpdateStatusLine();
    }
  }
}

function rotation_onmouseup() {
  rotation_onmousemove();
  EndDragging();
}

function UpdateTitle() {
  var title = "";
  if (g_gadget_file) {
    if (g_current_file) {
      if (g_current_file_changed)
        title = "*";
      title += g_current_file;
      title += " - ";
    }
    title += g_new_created ?
             strings.TITLE_NEW_GADGET : GetFileBaseName(g_gadget_file);
    title += " - ";
  }
  title += strings.GADGET_TITLE;
  e_title.innerText = title;
}

function UpdateDesignerViewPos() {
  // Note: this function can be called from onsize handler, so
  // e_designer.offsetWidth/offsetHeight can't be used.
  e_designer_view.x = Math.max(0, Math.round((e_designer.width -
                                              e_designer_view.width) / 2));
  // e_designer.height is 100%, so must use e_main_ui.height.
  e_designer_view.y = Math.max(0, Math.round((e_main_ui.height -
                                              e_designer_view.height) / 2));
  view.setTimeout(UpdateSelectionPos, 0);
}

function CurrentFileChanged() {
  if (!g_current_file_changed) {
    debug.info("***** Current gadget file changed");
    g_current_file_changed = true;
  }
  g_changed_since_save_undo = true;
  UpdateTitle();
}

function GadgetChanged() {
  if (!g_gadget_changed) {
    debug.info("***** Gadget changed");
    g_gadget_changed = true;
  }
}

function MoveSelectedElement(delta_x, delta_y) {
  var x_moved = 0;
  var y_moved = 0;
  if (!g_x_fixed) {
    var org_x = g_selected_element.offsetX;
    var new_x = Math.min(kMaxX, Math.max(kMinX, org_x + delta_x));
    g_selected_element.x = new_x;
    x_moved = new_x - org_x;
    if (x_moved != 0) {
      g_properties_view.SyncPropertyFromElement("x");
      CurrentFileChanged();
    }
  }
  if (!g_y_fixed) {
    var org_y = g_selected_element.offsetY;
    var new_y = Math.min(kMaxY, Math.max(kMinY, org_y + delta_y));
    g_selected_element.y = new_y;
    y_moved = new_y - org_y;
    if (y_moved != 0) {
      g_properties_view.SyncPropertyFromElement("y");
      CurrentFileChanged();
    }
  }

  var pos = ElementPosToView(g_selected_element);
  e_selection.x = e_pin.x = pos.x + e_designer_view.offsetX;
  e_selection.y = e_pin.y = pos.y + e_designer_view.offsetY;
  UpdateStatusLine();
  return { x: x_moved, y: y_moved };
}

function MoveSelectedElementInElementCoord(delta_x, delta_y) {
  var parent_delta = ElementDeltaToParent(g_selected_element, delta_x, delta_y);
  var moved = MoveSelectedElement(parent_delta.x, parent_delta.y);
  moved = ParentDeltaToElement(g_selected_element, moved.x, moved.y);
  UpdateStatusLine();
  return moved;
}

function ResizeSelectedElement(delta_x, delta_y) {
  var delta_width = 0;
  var delta_height = 0;
  if (!g_width_fixed) {
    var org_width = g_selected_element.offsetWidth;
    var new_width = Math.min(kMaxWidth,
                             Math.max(kMinWidth, org_width + delta_x));
    g_selected_element.width = new_width;
    e_selection.width = new_width + 2 * kSelectionBorderWidth;
    delta_width = new_width - org_width;
    if (delta_width != 0) {
      g_properties_view.SyncPropertyFromElement("width");
      CurrentFileChanged();
    }
  }
  if (!g_height_fixed) {
    var org_height = g_selected_element.offsetHeight;
    var new_height = Math.min(kMaxHeight,
                              Math.max(kMinHeight, org_height + delta_y));
    g_selected_element.height = new_height;
    e_selection.height = new_height + 2 * kSelectionBorderWidth;
    delta_height = new_height - org_height;
    if (delta_height != 0) {
      g_properties_view.SyncPropertyFromElement("height");
      CurrentFileChanged();
    }
  }
  UpdateStatusLine();
  return { x: delta_width, y: delta_height };
}

function EndDragging() {
  if (!g_dragging)
    return;
  g_dragging = false;
  if (g_properties_view)
    g_properties_view.SyncFromElement();
  e_selection.focus();
  if (g_selected_element == e_designer_view)
    view.setTimeout(UpdateDesignerViewPos, 0);
  SaveUndoState();
  UpdateStatusLine();
}

function DragMove(x, y) {
  if (g_selected_element != e_designer_view) {
    var delta = ViewDeltaToElement(g_selected_element.parentElement,
                                   x - g_drag_offset_x, y - g_drag_offset_y);
    MoveSelectedElement(delta.x, delta.y);
    g_drag_offset_x = x;
    g_drag_offset_y = y;
  }
}

function InitToolBar() {
  for (var i = 0; i < kTools.length; i++) {
    var tool_name = kTools[i];
    e_toolbar.appendElement("<div height='100%'>" +
        "<button width='100%' height='100%' stretchMiddle='true'" +
        " overImage='images/tool_button_over.png'" +
        " downImage='images/tool_button_down.png'" +
        " onmousemove='tool_button_onmousemove(" + i + ")'" +
        " onmousedown='tool_button_onmousedown(" + i + ")'" +
        " onmouseup='tool_button_onmouseup(" + i + ")'" +
        " ondblclick='tool_button_ondblclick(" + i + ")'/>" +
        "<img x='50%' pinX='50%' src='images/tool_" + tool_name + ".png'/>" +
        "<label size='8' y='25' x='50%' pinX='50%'" +
        " onsize='tool_label_onsize(" + i + ")'>" +
        "&TOOL_" + tool_name.toUpperCase() + ";</label>" +
        "</div>");
  }
}

function DisableTools() {
  with (e_toolbar.children) {
    for (var i = 0; i < count; i++) {
      var child = item(i);
      child.opacity = 150;
      child.enabled = false;
    }
  }
}

function EnableTools() {
  with (e_toolbar.children) {
    for (var i = 0; i < count; i++) {
      var child = item(i);
      child.opacity = 255;
      child.enabled = true;
    }
  }
}

function tool_button_onmousedown(index) {
  if (event.button == 1) {
    SelectTool(index);
    g_tool_button_down = true;
  }
}

function tool_button_onmousemove(index) {
  if (!g_tool_button_down)
    return;
  if (g_dragging && !(event.button & 1)) {
    e_designer_panel.cursor = "arrow";
    EndDragging();
    return;
  }

  var x_in_designer_view = event.x + event.srcElement.parentElement.offsetX -
                           e_designer_view.offsetX;
  var y_in_designer_panel = event.y - e_toolbar.offsetHeight;
  var y_in_designer_view = y_in_designer_panel - e_designer_view.offsetY;
  if (g_selected_element && g_dragging) {
    DragMove(x_in_designer_view, y_in_designer_view);
  } else if (y_in_designer_panel > 0) {
    // Create a new element and start dragging.
    SelectElement(NewElement(kTools[index], x_in_designer_view,
                             y_in_designer_view));
    CurrentFileChanged();
    SelectTool(-1);
    g_drag_offset_x = x_in_designer_view;
    g_drag_offset_y = y_in_designer_view;
    event.srcElement.cursor = "sizeall";
    g_dragging = true;
  }
}

function tool_button_onmouseup(index) {
  g_tool_button_down = false;
  tool_button_onmousemove();
  event.srcElement.cursor = "arrow";
  EndDragging();
}

function tool_button_ondblclick(index) {
  SelectElement(NewElement(kTools[index]));
  CurrentFileChanged();
  SaveUndoState();
  SelectTool(-1);
}

function FindElementByPos(element, x, y) {
  if (designerUtils.isPointIn(element, x, y)) {
    if (element.children &&
        (element.tagName != "combobox" || element.droplistVisible)) {
      var descendant = null;
      for (var i = element.children.count - 1; i >= 0; i--) {
        var child = element.children.item(i);
        var child_pos = ParentCoordToElement(child, x, y);
        var descendant = FindElementByPos(child, child_pos.x, child_pos.y);
        if (descendant)
          break;
      }
      return descendant ? descendant : element;
    } else {
      return element;
    }
  }
  return null;
}

function ElementCoordToParent(element, x, y) {
  return designerUtils.elementCoordToAncestor(element, element.parentElement,
                                              x, y);
}

function ParentCoordToElement(element, x, y) {
  return designerUtils.ancestorCoordToElement(element.parentElement, element,
                                              x, y);
}

function ElementDeltaToParent(element, delta_x, delta_y) {
  var pos0 = ElementCoordToParent(element, 0, 0);
  var pos = ElementCoordToParent(element, delta_x, delta_y);
  return { x: pos.x - pos0.x, y: pos.y - pos0.y };
}

function ParentDeltaToElement(element, delta_x, delta_y) {
  var pos0 = ParentCoordToElement(element, 0, 0);
  var pos = ParentCoordToElement(element, delta_x, delta_y);
  return { x: pos.x - pos0.x, y: pos.y - pos0.y };
}

// Here "View" means the e_designer_view.
// element must be an element under the e_designer_view.
function ElementCoordToView(element, x, y) {
  return designerUtils.elementCoordToAncestor(element, e_designer_view, x, y);
}

function ElementPosToView(element) {
  return ElementCoordToView(element,
                            designerUtils.getOffsetPinX(element),
                            designerUtils.getOffsetPinY(element));
}

function ViewCoordToElement(element, x, y) {
  return designerUtils.ancestorCoordToElement(e_designer_view, element, x, y);
}

function ViewDeltaToElement(element, delta_x, delta_y) {
  var path = [ ];
  while (element != e_designer_view) {
    path.push(element);
    element = element.parentElement;
  }
  var delta = { x: delta_x, y: delta_y };
  for (var i = path.length - 1; i >= 0; i--)
   delta = ParentDeltaToElement(path[i], delta.x, delta.y);
  return delta;
}

function ElementDeltaToView(element, delta_x, delta_y) {
  var delta = { x: delta_x, y: delta_y };
  while (element != e_designer_view) {
    delta = ElementDeltaToParent(element, delta.x, delta.y);
    element = element.parentElement;
  }
  return delta;
}

function ElementRotationToView(element) {
  var rotation = 0;
  while (element != e_designer_view) {
    rotation += element.rotation;
    element = element.parentElement;
  }
  rotation %= 360;
  if (rotation < 0) rotation += 360;
  return rotation;
}

function ViewRotationToElement(element, rotation) {
  element = element.parentElement;
  while (element != e_designer_view) {
    rotation -= element.rotation;
    element = element.parentElement;
  }
  rotation %= 360;
  if (rotation < 0) rotation += 360;
  return rotation;
}

function UpdateSelectionPos() {
  if (g_selected_element) {
    var pos = ElementPosToView(g_selected_element);
    e_selection.x = e_pin.x = e_designer_view.offsetX + pos.x;
    e_selection.y = e_pin.y = e_designer_view.offsetY + pos.y;
    e_selection.width = g_selected_element.offsetWidth +
                        2 * kSelectionBorderWidth;
    e_selection.height = g_selected_element.offsetHeight +
                         2 * kSelectionBorderWidth;
    e_selection.pinX = designerUtils.getOffsetPinX(g_selected_element) +
                       kSelectionBorderWidth;
    e_selection.pinY = designerUtils.getOffsetPinY(g_selected_element) +
                       kSelectionBorderWidth;
    e_selection.rotation = ElementRotationToView(g_selected_element);
    e_rotation1.visible = e_rotation2.visible = e_pin.visible =
        g_selected_element != e_designer_view;
    e_size_top_left.visible = !g_x_fixed && !g_y_fixed &&
                              !g_width_fixed & !g_height_fixed;
    e_size_top.visible = !g_y_fixed && !g_height_fixed;
    e_size_top_right.visible = !g_y_fixed && !g_height_fixed && !g_width_fixed;
    e_size_left.visible = !g_x_fixed && !g_width_fixed;
    e_size_right.visible = !g_width_fixed;
    e_size_bottom_left.visible = !g_x_fixed && !g_width_fixed &&
                                 !g_height_fixed;
    e_size_bottom.visible = !g_height_fixed;
    e_size_bottom_right.visible = !g_width_fixed && !g_height_fixed;
    e_selection.visible = true;
    UpdateStatusLine();
  }
}

function DisplayValue(value) {
  return value === "" || value === undefined ? "-" : value;
}

function UpdateStatusLine() {
  if (g_selected_element) {
    var status = DisplayValue(g_selected_element.width, "-") + "x" +
                 DisplayValue(g_selected_element.height, "-");
    if (g_selected_element != e_designer_view) {
      status = DisplayValue(g_selected_element.x, 0) + "," +
               DisplayValue(g_selected_element.y, 0) + "  " +
               status +
               "  pin " + DisplayValue(g_selected_element.pinX, 0) + "," +
               DisplayValue(g_selected_element.pinY, 0) + "  " +
               g_selected_element.rotation + "\u00b0";
    }
    e_status.innerText = status;
  } else {
    e_status.innerText = "";
  }
}

function UpdatePositionFixed() {
  if (g_selected_element == e_designer_view) {
    g_x_fixed = g_y_fixed = true;
    g_width_fixed = g_height_fixed = false;
  } else {
    // Don't allow mouse resizing if x/y/width/height specified percentage.
    var is_item = g_selected_element.tagName == "item";
    g_x_fixed = typeof g_selected_element.x == "string" || is_item;
    g_y_fixed = typeof g_selected_element.y == "string" || is_item;
    g_width_fixed = typeof g_selected_element.width == "string" || is_item;
    g_height_fixed = typeof g_selected_element.height == "string" ||
                     is_item || g_selected_element.tagName == "combobox";
  }
}

function SelectElement(element_info) {
  if (element_info) {
    var element = element_info.element;
    g_selected_element = element;
    // Will be set to true in UpdateSelectionPos.
    e_selection.visible = false;
    e_pin.visible = false;
    UpdatePositionFixed();
    g_properties_view = new Properties(
        e_properties_list, g_properties_meta[element_info.type], element_info,
        function() { // The on_property_change callback.
          // Setting one property may cause changes of other properties, so
          // sync all properties.
          g_properties_view.SyncFromElement();
          UpdatePositionFixed();
          view.setTimeout(UpdateSelectionPos, 0);
          view.setTimeout(UpdateSelectionPos, 100);
          CurrentFileChanged();
          SaveUndoState();
        });
    g_events_view = new Properties(
        e_events_list, g_events_meta[element_info.type], element_info,
        function() { // The on_property_change callback.
          // TODO:
          CurrentFileChanged();
          SaveUndoState();
        });
    ElementsViewSelectNode(element_info);
    view.setTimeout(UpdateSelectionPos, 0);
  } else {
    e_selection.visible = false;
    e_combobox_border.visible = false;
    e_pin.visible = false;
    g_selected_element = null;
    g_properties_view = null;
    g_events_view = null;
    e_properties_list.removeAllElements();
    e_events_list.removeAllElements();
    ElementsViewSelectNode(null);
    e_status.innerText = "";
    g_x_fixed = g_y_fixed = false;
    g_width_fixed = g_height_fixed = false;
  }
}

function SelectTool(index) {
  if (g_current_tool == index) {
    // Selecting a currently selected tool will cause the tool unselected.
    index = -1;
  }
  if (g_current_tool != -1) {
    var button = e_toolbar.children.item(g_current_tool).children.item(0);
    button.image = "";
    button.overImage = "images/tool_button_over.png";
  }
  g_current_tool = index;
  if (index != -1) {
    var button = e_toolbar.children.item(index).children.item(0);
    button.image = "images/tool_button_active.png";
    button.overImage = "images/tool_button_active_over.png";
    SelectElement(null);
  }
}

function AddGlobalMenu(menu) {
  AddEditMenu(menu);
  menu.AddItem("", 0, null);
  menu.AddItem(strings.MENU_NEW_GADGET, 0, NewGadget);
  menu.AddItem(strings.MENU_OPEN_GADGET, 0, OpenGadget);
  var enabled_gadget_open = g_gadget_file ? 0 : gddMenuItemFlagGrayed;
  var enabled_current_file = g_current_file ? 0 : gddMenuItemFlagGrayed;
  menu.AddItem(strings.MENU_SAVE_CURRENT_FILE, enabled_current_file,
               SaveCurrentFile);
  menu.AddItem(strings.MENU_SAVE_GADGET_AS, enabled_gadget_open, SaveGadgetAs);
  menu.AddItem(strings.MENU_CLOSE_GADGET, enabled_gadget_open, CloseGadget);
  menu.AddItem("", 0, null);
  AddAddMenu(menu);
  menu.AddItem(strings.MENU_EDIT_SOURCE, enabled_current_file, EditSource);
  menu.AddItem(strings.MENU_REFRESH, enabled_gadget_open, Refresh);
  menu.AddItem("", 0, null);
  menu.AddItem(strings.MENU_RUN_GADGET, enabled_gadget_open, RunGadget);
}

// Currently no "Edit" menu, but only Undo and Redo.
function AddEditMenu(menu) {
  menu.AddItem(strings.MENU_UNDO, CanUndo() ? 0 : gddMenuItemFlagGrayed, Undo);
  menu.AddItem(strings.MENU_REDO, CanRedo() ? 0 : gddMenuItemFlagGrayed, Redo);
}

// Add the submenu "Add >" into the menu.
function AddAddMenu(menu) {
  var enabled_gadget_open = g_gadget_file ? 0 : gddMenuItemFlagGrayed;
  var add_menu = menu.AddPopup(strings.MENU_ADD);
  menu.SetItemStyle(strings.MENU_ADD, enabled_gadget_open);
  add_menu.AddItem(Strings.MENU_ADD_FILES, enabled_gadget_open, AddFiles);
  add_menu.AddItem(
      Strings.MENU_ADD_OPTIONS_VIEW,
      g_gadget_file && !g_gadget_file_manager.exists(kOptionsXML) ?
          0 : gddMenuItemFlagGrayed,
      AddOptionsView);
  add_menu.AddItem(Strings.MENU_ADD_NEW_VIEW, enabled_gadget_open, AddNewView);
  add_menu.AddItem(Strings.MENU_ADD_NEW_FILE, enabled_gadget_open, AddNewFile);
}

function AddFileItemMenu() {
  var menu = event.menu;
  var filename = event.srcElement.children.item(0).innerText;
  var disabled = filename == kMainXML || filename == kGadgetGManifest ?
                 gddMenuItemFlagGrayed : 0;
  AddAddMenu(menu);
  menu.AddItem(Strings.MENU_REMOVE_FILE, disabled, function() {
    RemoveFile(filename);
  });
  menu.AddItem(Strings.MENU_RENAME_MOVE_FILE, disabled, function() {
    RenameMoveFile(filename);
  });
  event.returnValue = false;
}

function RefreshFileList() {
  e_file_list.removeAllElements();
  var files = g_gadget_file_manager.getAllFiles();
  files.sort();
  for (var i = 0; i < files.length; i++) {
    var file = files[i];
    e_file_list.appendString(file);
    if (file == g_current_file)
      e_file_list.selectedIndex = i;

    var item = e_file_list.children.item(i);
    item.oncontextmenu = AddFileItemMenu;
    var label = item.children.item(0);
    label.tooltip = file;
    label.size = 9;
    label.x = 1;
    label.width = "99%";
    label.trimming = "character-ellipsis";
  }
}

function NewGadget() {
  if (!CloseGadget())
    return;

  OpenGadgetFile(gadget.storage.extract("blank-gadget.gg"), kMainXML);
  if (g_gadget_file_manager) {
    var manifest = g_gadget_file_manager.read(kGadgetGManifest)
        .replace("{{UUID}}", designerUtils.generateUUID())
        .replace("{{AUTHOR}}", designerUtils.getUserName());
    g_gadget_file_manager.write(kGadgetGManifest, manifest, true);
  }
  g_new_created = true;
  UpdateTitle();
}

function OpenGadget() {
  if (!CloseGadget())
    return;
  var file = framework.BrowseForFile(kGadgetFileFilter);
  if (file)
    OpenGadgetFile(file, kMainXML);
}

function OpenGadgetFile(gadget_file, first_file) {
  g_gadget_file = gadget_file;
  g_gadget_file_manager = designerUtils.initGadgetFileManager(gadget_file);
  if (g_gadget_file_manager) {
    g_gadget_changed = false;
    g_new_created = false;
    e_main_ui.visible = true;
    e_file_list.enabled = true;
    EnableTools();
    OpenFile(first_file);
    RefreshFileList();
    return true;
  } else {
    alert(strings.ALERT_FAIL_OPEN_GADGET.replace("{{FILENAME}}", gadget_file));
    return false;
  }
}

function CheckSaveGadget(cancel_button) {
  if (CheckSaveCurrentFile(cancel_button)) {
    if (g_new_created && g_gadget_changed) {
      switch (confirm(strings.CONFIRM_SAVE_GADGET, cancel_button)) {
        case gddConfirmResponseYes:
          return SaveGadgetAsInternal();
        case gddConfirmResponseNo:
          return true;
        case gddConfirmResponseCancel:
          return false;
      }
    }
    return true;
  }
  return false;
}

function CheckSaveCurrentFile(cancel_button) {
  if (!g_current_file_changed)
    return true;
  switch (confirm(strings.CONFIRM_SAVE_FILE.replace("{{FILENAME}}",
                                                    g_current_file),
                  cancel_button)) {
    case gddConfirmResponseYes:
      return SaveCurrentFile();
    case gddConfirmResponseNo:
      return true;
    case gddConfirmResponseCancel:
      return false;
  }
}

function SaveGadgetAsInternal() {
  var default_name = g_new_created ? "NewGadget.gg" : g_gadget_file;
  var save_as = framework.BrowseForFile(kGadgetFileFilterGGOnly,
                                        strings.TITLE_SAVE_GADGET_AS,
                                        gddBrowseFileModeSaveAs,
                                        default_name);
  if (save_as) {
    if (g_global_file_manager.copy(g_gadget_file, save_as, true)) {
      OpenGadgetFile(save_as, g_current_file);
    } else {
      alert(string.ALERT_FAIL_SAVE_GADGET_AS.replace("{{FILENAME}}",
            save_as));
    }
    return true;
  }
  return false;
}

function SaveGadgetAs() {
  return SaveCurrentFile() ? SaveGadgetAsInternal() : false;
}

function CloseGadget() {
  if (!CheckSaveGadget(true))
    return false;

  designerUtils.removeGadget();
  DisableTools();
  e_file_list.enabled = false;
  e_main_ui.visible = false;
  ResetGlobals();
  g_gadget_file = null;
  g_gadget_file_manager = null;
  e_file_list.removeAllElements();
  g_extracted_files = { };
  g_extracted_files_time = { };
  g_undo_stack = [ ];
  g_undo_stack_pos = 0;
  g_undo_stack_pos_saved = 0;
  UpdateTitle();
  return true;
}

function RunGadget() {
  designerUtils.runGadget(g_gadget_file);
}

function GetFileExtension(filename) {
  var extpos = filename.lastIndexOf(".");
  return extpos > 0 ? filename.substring(extpos + 1) : "";
}

function GetFileBaseName(filename) {
  var slashpos = filename.lastIndexOf("/");
  return slashpos > 0 ? filename.substring(slashpos + 1) : filename;
}

function AddFiles() {
  if (!CheckSaveCurrentFile(true))
    return;
  var files = framework.BrowseForFiles("");
  // files is an array in GGLinux.
  if (files && files.length) {
    var prefix = prompt(strings.PROMPT_ADD_FILE_PREFIX, "");
    for (var i = 0; i < files.length; i++) {
      var file = files[i];
      var filename = file.substring(file.lastIndexOf("/") + 1);
      var dest = prefix + filename;
      if (ConfirmOverwrite(dest)) {
        var content = g_global_file_manager.readBinary(file);
        if (!content) {
          alert(strings.ALERT_FAIL_ADD_FILE.replace("{{FILENAME}}", file));
        } else if (!g_gadget_file_manager.writeBinary(dest, content, true)) {
          alert(strings.ALERT_FAIL_SAVE_FILE.replace("{{FILENAME}}", dest));
        } else {
          GadgetChanged();
        }
      }
    }
    Refresh();
  }
}

function Refresh() {
  RefreshFileList();
  if (g_current_file)
    OpenFile(g_current_file);
}

function ConfirmOverwrite(file) {
  return !g_gadget_file_manager.exists(file) ||
         confirm(strings.CONFIRM_OVERWRITE.replace("{{FILENAME}}", file));
}

function CheckExists(file) {
  if (g_gadget_file_manager.exists(file)) {
    alert(strings.ALERT_FILE_EXISTS.replace("{{FILENAME}}", file));
    return true;
  }
  return false;
}

function AddOptionsView() {
  if (CheckExists(kOptionsXML))
    return;
  if (!g_gadget_file_manager.write(
          kOptionsXML,
          '<?xml version="1.0" encoding="utf-8"?>\n' +
          '<view width="200" height="200"' +
          ' onopen="options_onopen()" onok="options_onok()">\n' +
          '  <script src="' + kOptionsJS + '"/>\n' +
          '</view>\n', false)) {
    alert(strings.ALERT_FAIL_SAVE_FILE.replace("{{FILENAME}}", kOptionsXML));
    return;
  }
  if (!CheckExists(kOptionsJS) &&
      !g_gadget_file_manager.write(
          kOptionsJS,
          "function options_onopen()\n{\n}\n" +
          "function options_onok()\n{\n}\n", false)) {
    alert(strings.ALERT_FAIL_SAVE_FILE.replace("{{FILENAME}}", kOptionsJS));
    // Ignore the situation and continue.
  }
  GadgetChanged();
  RefreshFileList();
  OpenFile(kOptionsXML);
}

function AddNewView() {
  var view_name = prompt(strings.PROMPT_NEW_VIEW_NAME, "");
  if (view_name) {
    if (GetFileExtension(view_name) != "xml")
      view_name += ".xml";
    if (CheckExists(view_name))
      return;
    if (g_gadget_file_manager.write(
            view_name,
            '<?xml version="1.0" encoding="utf-8"?>\n' +
            '<view width="200" height="200">' +
            '</view>\n', false)) {
      GadgetChanged();
      RefreshFileList();
      OpenFile(view_name);
    } else {
      alert(strings.ALERT_FAIL_SAVE_FILE.replace("{{FILENAME}}", view_name));
    }
  }
}

function AddNewFile() {
  var x = y;
  var file_name = prompt(strings.PROMPT_NEW_FILE_NAME, "");
  if (file_name) {
    if (CheckExists(file_name))
      return;
    if (g_gadget_file_manager.write(file_name, "", false)) {
      GadgetChanged();
      RefreshFileList();
      OpenFile(file_name);
    } else {
      alert(strings.ALERT_FAIL_SAVE_FILE.replace("{{FILENAME}}", file_name));
    }
  }
}

function RemoveFile(file_name) {
  if (!CheckSaveCurrentFile(true))
    return;
  if (confirm(strings.CONFIRM_REMOVE_FILE.replace("{{FILENAME}}", file_name))) {
    if (g_gadget_file_manager.remove(file_name)) {
      GadgetChanged();
      RefreshFileList();
      OpenFile(g_current_file && file_name == g_current_file ?
               kMainXML : g_current_file);
    } else {
      alert(strings.ALERT_FAIL_REMOVE_FILE.replace("{{FILENAME}}", file_name));
    }
  }
}

function RenameMoveFile(file_name) {
  var new_file_name = prompt(strings.PROMPT_RENAME_MOVE_FILE, file_name);
  if (new_file_name && new_file_name != file_name) {
    if (!ConfirmOverwrite(new_file_name))
      return;
    if (g_gadget_file_manager.copy(file_name, new_file_name, true) &&
        g_gadget_file_manager.remove(file_name)) {
      GadgetChanged();
      RefreshFileList();
      OpenFile(g_current_file && file_name == g_current_file ?
          kMainXML : g_current_file);
    } else {
      alert(strings.ALERT_FAIL_RENAME_MOVE_FILE
          .replace("{{FILENAME}}", file_name)
          .replace("{{NEWFILENAME}}", new_file_name));
    }
  }
}

function SystemOpenFile(filename) {
  var full_path;
  if (g_gadget_file_manager.isDirectlyAccessible(filename)) {
    full_path = g_gadget_file_manager.getFullPath(filename);
  } else {
    full_path = g_gadget_file_manager.extract(filename);
    if (full_path) {
      g_extracted_files[filename] = full_path;
      g_extracted_files_time[filename] =
          g_global_file_manager.getLastModifiedTime(full_path).getTime();
    }
  }

  if (full_path) {
    var extension = GetFileExtension(filename);
    var editor = g_system_editors[extension];
    if (editor)
      designerUtils.systemOpenFileWith(editor, full_path);
    else
      framework.openUrl(full_path);
  } else {
    alert(strings.ALERT_FAIL_OPEN_FILE.replace("{{FILENAME}}", filename));
  }
}

function EditSource() {
  if (CheckSaveCurrentFile(true))
    SystemOpenFile(g_current_file);
}

function SelectFile(filename) {
  var items = e_file_list.children;
  for (var i = 0; i < items.count; i++) {
    if (items.item(i).children(0).innerText == filename) {
      e_file_list.selectedIndex = i;
      break;
    }
  }
  UpdateTitle();
}

// Replace entities (except the predefined ones) to a XML string representing
// itself, e.g. replace "&MENU_FILE;" to "&amp;MENU_FILE;".
// This is important to let the original entity strings entered and
// displayed in the designer in place of the actual localized strings.
// Works well except some corner cases, but it's enough for a designer.
function ReplaceEntitiesToStrings(s) {
  // We don't allow ':' and non-ASCII alpha chars in string names.
  return s.replace(
      /&(?!(amp|lt|gt|apos|quot);)(?=[a-zA-Z_][a-zA-Z0-9_\-.]*;)/gm, "&amp;");
}

function ReplaceStringsToEntities(s) {
  return s.replace(/&amp;(?=[a-zA-Z_][a-zA-Z0-9_\-.]*;)/gm, "&");
}

function SaveCurrentFile() {
  if (!g_current_file)
    return false;
  var content = ReplaceStringsToEntities(ViewToXML());
  if (g_gadget_file_manager.write(g_current_file, content, true)) {
    if (!g_gadget_file_manager.isDirectlyAccessible(g_current_file)) {
      var extracted = g_extracted_files[g_current_file];
      if (extracted) {
        // Re-extract the file to let the system editor know the change.
        var full_path = g_gadget_file_manager.extract(g_current_file);
        if (full_path != extracted)
          g_global_file_manager.write(extracted, content, true);
        g_extracted_files_time[g_current_file] =
            g_global_file_manager.getLastModifiedTime(extracted).getTime();
      }
    }
    g_current_file_time =
        g_gadget_file_manager.getLastModifiedTime(g_current_file).getTime();
    g_current_file_changed = false;
    g_undo_stack_pos_saved = g_undo_stack_pos;
    GadgetChanged();
    UpdateTitle();
    return true;
  } else {
    alert(strings.ALERT_FAIL_SAVE_FILE.replace("{{FILENAME}}", filename));
    return false;
  }
}

function OpenFile(filename) {
  var extension = GetFileExtension(filename);
  if (extension == "xml") {
    var content = g_gadget_file_manager.read(filename);
    if (content) {
      content = ReplaceEntitiesToStrings(content);
      var doc = new DOMDocument();
      doc.loadXML(content);
      if (GadgetStrEquals(doc.documentElement.tagName, "view")) {
        if (!CheckSaveCurrentFile(true))
          return;
        ResetGlobals();
        g_undo_stack = [ content ];
        g_undo_stack_pos = g_undo_stack_pos_saved = 1;
        ParseViewXMLDocument(doc);
        g_current_file = filename;
        g_current_file_time =
            g_gadget_file_manager.getLastModifiedTime(filename).getTime();
        SelectFile(filename);
        e_selection.focus();
        return;
      } else {
        gadget.debug.warning("XML file is not a view file: " + filename);
      }
    } else {
      gadget.debug.warning("Failed to load file or file is empty: " + filename);
    }
  }
  SystemOpenFile(filename);
  // Still select the current opened file.
  SelectFile(g_current_file);
}

function CheckExternalFileChange() {
  // Copy changed extracted files back into the zip.
  for (var i in g_extracted_files) {
    var extracted = g_extracted_files[i];
    if (g_global_file_manager.getLastModifiedTime(extracted).getTime() >
        g_extracted_files_time[i]) {
      var content = g_global_file_manager.read(extracted);
      g_gadget_file_manager.write(i, content, true);
      gadget.debug.warning("Updating file: " + i);
    }
  }

  if (g_current_file) {
    var time =
        g_gadget_file_manager.getLastModifiedTime(g_current_file).getTime();
    if (time > g_current_file_time) {
      if (confirm(strings.CONFIRM_CHANGED
                  .replace("{{FILENAME}}", g_current_file))) {
        g_current_file_changed = false;
        OpenFile(g_current_file);
      }
    }
  }
}

function AddElementContextMenu() {
  if (!g_selected_element || g_selected_element == e_designer_view)
    return true;

  var menu = event.menu;
  var can_up_level = false;
  var can_down_level = false;
  var can_move_back = false;
  var can_move_front = false;
  var parent = g_selected_element.parentElement;
  var index = GetIndexInParent(g_selected_element);
  can_up_level = parent != e_designer_view;
  can_move_back = index > 0;
  can_move_front = index < parent.children.count - 1;
  // The element can be moved down a level if the next element can have
  // children.
  can_down_level = can_move_front && parent.children.item(index + 1).children;

  AddEditMenu(menu);
  menu.AddItem(strings.MENU_REMOVE_ELEMENT, 0, RemoveSelectedElement);
  menu.AddItem(strings.MENU_RENAME_ELEMENT, 0, RenameElement);
  menu.AddItem("", 0, null);
  menu.AddItem(strings.MENU_MOVE_BACK,
               can_move_back ? 0 : gddMenuItemFlagGrayed,
               MoveElementBack);
  menu.AddItem(strings.MENU_MOVE_FRONT,
               can_move_front ? 0 : gddMenuItemFlagGrayed,
               MoveElementFront);
  menu.AddItem(strings.MENU_MOVE_BOTTOM,
               can_move_back ? 0 : gddMenuItemFlagGrayed,
               MoveElementToBottom);
  menu.AddItem(strings.MENU_MOVE_TOP,
               can_move_front ? 0 : gddMenuItemFlagGrayed,
               MoveElementToTop);
  menu.AddItem(strings.MENU_MOVE_UP_LEVEL,
               can_up_level ? 0 : gddMenuItemFlagGrayed,
               MoveElementUpLevel);
  menu.AddItem(strings.MENU_MOVE_DOWN_LEVEL,
               can_down_level ? 0 : gddMenuItemFlagGrayed,
               MoveElementDownLevel);
  if (g_selected_element.tagName == "listbox" ||
      g_selected_element.tagName == "combobox") {
    menu.AddItem("", 0, null);
    menu.AddItem(strings.MENU_APPEND_STRING, 0, PromptAndAppendString);
  }
  event.returnValue = false;
}

function RemoveSelectedElement() {
  if (g_selected_element && g_selected_element != e_designer_view) {
    // DeleteElement will return the next element to be selected.
    SelectElement(DeleteElement(g_selected_element));
    CurrentFileChanged();
    SaveUndoState();
  }
}

function RenameElement() {
  g_properties_view.SelectProperty("name");
}

function MoveElementBack() {
  SelectElement(MoveElement(g_selected_element,
                            g_selected_element.parentElement,
                            GetPreviousSibling(g_selected_element)));
  CurrentFileChanged();
  SaveUndoState();
}

function MoveElementFront() {
  var parent = g_selected_element.parentElement;
  var index = GetIndexInParent(g_selected_element);
  SelectElement(MoveElement(g_selected_element, parent,
                            parent.children.item(index + 2)));
  CurrentFileChanged();
  SaveUndoState();
}

function MoveElementToBottom() {
  var parent = g_selected_element.parentElement;
  SelectElement(MoveElement(g_selected_element, parent,
                            parent.children.item(0)));
  CurrentFileChanged();
  SaveUndoState();
}

function MoveElementToTop() {
  SelectElement(MoveElement(g_selected_element,
                            g_selected_element.parentElement, null));
  CurrentFileChanged();
  SaveUndoState();
}

function MoveElementUpLevel() {
  var parent = g_selected_element.parentElement;
  if (parent.tagName == "item")
    parent = parent.parentElement;
  SelectElement(MoveElement(g_selected_element, parent.parentElement, parent));
  CurrentFileChanged();
  SaveUndoState();
}

function MoveElementDownLevel() {
  var next = GetNextSibling(g_selected_element);
  if (next && next.children) {
    SelectElement(MoveElement(g_selected_element, next,
                              next.children.item(0)));
    CurrentFileChanged();
    SaveUndoState();
  }
}

function PromptAndAppendString() {
  var s = view.prompt(strings.PROMPT_APPEND_STRING, "");
  if (s) {
    SelectElement(AppendString(g_selected_element, s));
    CurrentFileChanged();
    SaveUndoState();
  }
}

var kSupportedImageTypes = { jpg: null, jpeg: null, png: null, gif: null }; 
function ChooseColorImage(value, enable_color, enable_image) {
  var params = {
    value: value,
    enable_color: enable_color,
    enable_image: enable_image
  };

  if (enable_image) {
    params.images = [ ];
    var files = g_gadget_file_manager.getAllFiles();
    for (var i in files) {
      if (GetFileExtension(files[i]) in kSupportedImageTypes)
        params.images.push(files[i]);
    }
    params.images.sort();
  }
  designerUtils.showXMLOptionsDialog("choose_color_image.xml", params);
  return params.value;
}

function CanRedo() {
  return g_undo_stack_pos < g_undo_stack.length;
}

function CanUndo() {
  return g_undo_stack_pos > 1;
}

function SaveUndoState() {
  if (!g_changed_since_save_undo)
    return;
  g_changed_since_save_undo = false;
  if (CanRedo()) {
    g_undo_stack.splice(g_undo_stack_pos,
                        g_undo_stack.length - g_undo_stack_pos);
  }
  g_undo_stack.push(ViewToXML());
  if (g_undo_stack.length > kMaxUndoDepth)
    g_undo_stack.shift();
  g_undo_stack_pos = g_undo_stack.length;
  UpdateStatusLine();
}

function ReloadContent(content) {
  var doc = new DOMDocument();
  doc.loadXML(content);
  ResetGlobals();
  ParseViewXMLDocument(doc);
  e_selection.focus();
}

function Redo() {
  if (CanRedo()) {
    ReloadContent(g_undo_stack[g_undo_stack_pos++]);
    CheckSavedUndoPos();
  }
}

function Undo() {
  if (CanUndo()) {
    // The last undo state is saved after the last change, so don't restore
    // it but restore the last change before the last.
    --g_undo_stack_pos;
    ReloadContent(g_undo_stack[g_undo_stack_pos - 1]);
    CheckSavedUndoPos();
  }
}

function CheckSavedUndoPos() {
  if (g_undo_stack_pos == g_undo_stack_pos_saved) {
    g_current_file_changed = false;
    UpdateTitle();
  } else {
    CurrentFileChanged();
  }
}
