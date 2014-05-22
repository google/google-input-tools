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

function Tree(listbox, namespace, on_expand, menu_handler) {
  this.listbox_ = listbox;
  this.namespace_ = namespace;
  this.on_expand_ = on_expand;
  this.menu_handler_ = menu_handler;
  listbox.removeAllElements();
}

Tree.prototype = {
  kIndentPixels: 15,

  GetLevelIndent: function(level) {
    return this.kIndentPixels * level + 8;
  },

  GetLevel: function(id) {
    var node = this.GetNode(id);
    return node ? (node.children.item(0).offsetX - 8) / this.kIndentPixels : -1;
  },

  GetLevelByIndex: function(index) {
    var node = this.listbox_.children.item(index);
    return node ? (node.children.item(0).offsetX - 8) / this.kIndentPixels : -1;
  },

  GetNodeName: function(id) {
    return this.namespace_ + id;
  },

  GetIdFromNodeName: function(node_name) {
    return node_name.substring(this.namespace_.length);
  },

  GetNode: function(id) {
    return this.listbox_.children.item(this.GetNodeName(id));
  },

  GetNodeIndex: function(id) {
    var children = this.listbox_.children;
    var count = children.count;
    var node_name = this.GetNodeName(id);
    for (var i = 0; i < count; i++) {
      if (children.item(i).name == node_name)
        return i;
    }
    return -1;
  },

  GetIdByNode: function(node) {
    return this.GetIdFromNodeName(node.name);
  },

  InsertNode: function(level, id, title, has_children, before_id) {
    var indent = this.GetLevelIndent(level);
    var before_node = before_id ? this.GetNode(before_id) : null;
    var item = this.listbox_.insertElement(
      "<item name='" + this.GetNodeName(id) + "'/>", before_node);
    var button = item.appendElement(
      "<button image='images/folder.png' overImage='images/folder_over.png'" +
      " downImage='images/folder_over.png' pinX='50%' pinY='50%' x='" + indent +
      "' y='50%' visible='false'/>");
    var label = item.appendElement("<label x='" + (indent + 8) + "' size='9'" +
      " vAlign='middle' height='100%'/>");

    var this_ = this;
    button.visible = has_children;
    button.onclick = function() {
      this_.ToggleFolder(id);
    };
    if (this.menu_handler_) {
      item.oncontextmenu = function(menu) {
        this_.SelectNode(id);
        return this_.menu_handler_(menu);
      };
    }
    label.innerText = title;
    return item;
  },

  ToggleFolder: function(id) {
    var node = this.GetNode(id);
    if (node) {
      this.listbox_.selectedItem = node;
      var button = node.children.item(0);
      if (button.visible) {
        if (button.rotation == 0)
          this.ExpandFolder(id);
        else
          this.CollapseFolder(id);
      }
    }
  },

  ExpandFolder: function(id) {
    var node = this.GetNode(id);
    if (node) {
      var button = node.children.item(0);
      if (button.visible && button.rotation == 0) {
        if (this.on_expand_) {
          // The caller is in charge of adding the children nodes.
          this.on_expand_(id);
        }
        button.rotation = 90;
      }
    }
  },

  CollapseFolder: function(id) {
    var node = this.GetNode(id);
    if (node) {
      var button = node.children.item(0);
      if (button.visible && button.rotation == 90) {
        var level = this.GetLevel(id);
        var index = this.GetNodeIndex(id) + 1;
        while (true) {
          if (this.GetLevelByIndex(index) <= level)
            break;
          this.listbox_.removeElement(this.listbox_.children.item(index));
        }
        button.rotation = 0;
      }
    }
  },

  SetNodeTitle: function(id, title) {
    var node = this.GetNode(id);
    if (node)
      node.children.item(1).innerText = title;
  },

  SetNodeHasChildren: function(id, has_children) {
    var node = this.GetNode(id);
    if (node) {
      if (!has_children)
        this.CollapseFolder(id);
      node.children.item(0).visible = has_children;
    }
  },

  SelectNode: function(id) {
    this.listbox_.selectedItem = id ? this.GetNode(id) : null;
  },

  DeleteNode: function(id) {
    this.CollapseFolder(id);
    this.listbox_.removeElement(this.GetNode(id));
  }
};
