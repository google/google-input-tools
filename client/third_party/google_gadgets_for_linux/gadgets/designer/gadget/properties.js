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

function Properties(listbox, metadata, element_info, on_property_change) {
  var this_ = this;
  this.listbox_ = listbox;
  listbox.removeAllElements();
  this.metadata_ = metadata;
  this.element_info_ = element_info;
  this.element_ = element_info.element;
  this.on_property_change_ = on_property_change;
  this.get_value_funcs_ = [ ];
  this.set_value_funcs_ = [ ];
  this.setting_value_ = [ ];
  this.name_to_index_ = { };
  var i = 0;
  for (var name in metadata) {
    var item = listbox.appendElement("<item>" +
      "<div background='#808080' width='40%' height='100%' opacity='50'/>" +
      "<label width='40%' height='100%' size='9'" +
      " trimming='character-ellipsis'/></item>");
    item.children.item(1).tooltip = name;
    item.children.item(1).innerText = name;
    this.name_to_index_[name] = i;
    CreateEditControl(i, item, name);
    i++;
  }
  this.SyncFromElement();

  function CreateEditControl(index, item, name) {
    var default_value = metadata[name].default_value;
    var metainfo = metadata[name].extra_info;
    // The edit control of the property item, may be a combobox or an edit.
    var edit_control;

    if (metainfo instanceof Array) {
      // Enumerated values.
      edit_control = AddCombobox(metainfo);
      this_.get_value_funcs_[index] = function() {
        return metainfo[edit_control.selectedIndex];
      };
      this_.set_value_funcs_[index] = function(value) {
        var value = GetElementProperty(this_.element_info_, name);
        for (var i in metainfo) {
          if (value == metainfo[i]) {
            edit_control.selectedIndex = i;
            return;
          }
        }
        edit_control.selectedIndex = -1;
      };
    } else if (typeof default_value == "boolean") {
      edit_control = AddCombobox([ false, true ]);
      this_.get_value_funcs_[index] = function() {
        return edit_control.selectedIndex == 1;
      };
      this_.set_value_funcs_[index] = function(value) {
        edit_control.selectedIndex = value ? 1 : 0;
      };
    } else {
      // Normal property editor.
      edit_control = item.appendElement(
        "<edit width='60%' x='40%' height='100%' size='9'/>");
      edit_control.onkeypress = function() {
        if (event.keyCode == 10 || event.keyCode == 13)
          SyncOut();
      };
      edit_control.onfocusout = function() {
        SyncOut();
      };
      this_.get_value_funcs_[index] = function() {
        return edit_control.value;
      };
      this_.set_value_funcs_[index] = function(value) {
        edit_control.value = value;
      };
      
      if (typeof metainfo == "function") {
        var button = item.appendElement(
          "<button x='100%' pinX='14' width='14' height='17' " +
          "defaultRendering='true' caption='...' size='7' align='center'/>");
        button.onclick = function() {
          edit_control.value = metainfo(edit_control.value);
          SyncOut();
        };
      }
    }

    edit_control.onfocusin = function() {
      this_.listbox_.selectedIndex = index;
    };

    function AddCombobox(values) {
      var combobox = item.appendElement(
          "<combobox width='60%' x='40%' height='144' type='droplist'" +
          " itemWidth='100%' itemHeight='16' background='#FFFFFF'/>");
      for (var i in values) {
        combobox.appendElement("<item><label x='2' size='9'>" + values[i] +
                               "</label></item>");
      }
      combobox.onchange = function() {
        if (!this_.setting_value_[index])
          SyncOut();
      };
      combobox.onsize = function() {
        this_.UpdateComboboxBorder(combobox);
      };
      return combobox;
    }

    function SyncOut() {
      var value = this_.get_value_funcs_[index]();
      var old_value = GetElementProperty(this_.element_info_, name);
      if (value != old_value) {
        SetElementProperty(this_.element_info_, name, value);
        if (this_.on_property_change_) {
          // Delay the callback after the layout.
          view.setTimeout(this_.on_property_change_, 0);
        }
      }
    }
  }
}

Properties.prototype = {
  SyncFromElement: function() {
    var i = 0;
    for (var name in this.metadata_) {
      var value = GetElementProperty(this.element_info_, name);
      this.setting_value_[i] = true;
      this.set_value_funcs_[i](value);
      this.setting_value_[i] = false;
      i++;
    }
  },

  SyncPropertyFromElement: function(name) {
    var value = GetElementProperty(this.element_info_, name);
    var index = this.name_to_index_[name];
    if (index !== undefined) {
      this.setting_value_[index] = true;
      this.set_value_funcs_[index](value);
      this.setting_value_[index] = false;
    }
  },

  OnSelectionChange: function() {
    var item = this.listbox_.selectedItem;
    if (item) {
      var edit_control = item.children.item(2);
      edit_control.focus();
    }
  },

  SelectProperty: function(name) {
    var index = this.name_to_index_[name];
    if (index !== undefined) {
      if (this.listbox_.selectedIndex != index) {
        this.listbox_.selectedIndex = index;
      } else {
        this.OnSelectionChange();
      }
    }
  },

  UpdateComboboxBorder: function(combobox) {
    if (combobox.offsetHeight > 20) {
      var pos = designerUtils.elementCoordToAncestor(combobox, e_tool_views,
                                                     0, 0);
      e_combobox_border.x = pos.x - 1;
      e_combobox_border.y = pos.y + 18;
      e_combobox_border.width = combobox.offsetWidth + 2;
      e_combobox_border.height = combobox.offsetHeight - 17;
      e_combobox_border.visible = true;
    } else {
      e_combobox_border.visible = false;
    }
  }

};

