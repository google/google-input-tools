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

var g_elements_info = { };
var g_element_id_seq = 0;
var g_element_name_seqs = { };
var g_used_element_names = { };
var g_view_info = null;

function Trim(string) {
  return string.replace(/^\s+/gm, "").replace(/\s+$/gm, "");
}

function ResetViewGlobals() {
  g_elements_info = { };
  g_element_id_seq = 0;
  g_element_name_seqs = { };
  g_used_element_names = { };
  e_designer_view.removeAllElements();
  g_view_info = {
    element: e_designer_view,
    id: "e_designer_view",
    type: "view",
    extra_properties: { },
    events: { },
    before_comments: [ ],
    inner_comments: [ ],
    after_comments: [ ],
    scripts: [ ]
  };
  g_elements_info[g_view_info.id] = g_view_info;
}

function GadgetStrEquals(str1, str2) {
  return str1.toLowerCase() == str2.toLowerCase();
}

function GetElementInfo(element) {
  return element ? g_elements_info[element.name] : null;
}

function GetElementInfoParent(element_info) {
  return element_info == g_view_info ?
         null : GetElementInfo(element_info.element.parentElement);
}

function NewElementId() {
  return "id" + (g_element_id_seq++);
}

function GetElementProperty(element_info, name) {
  if (name == "name")
    return element_info.name;
  if (name.substring(0, 2) == "on")
    return element_info.events[name];
  var metadata = g_properties_meta[element_info.type][name];
  if (!metadata)
    return element_info.extra_properties[name];
  var value = metadata.get_value(element_info.element);
  // The e_designer_view is an element, so it may not support all properties
  // of view. These properties are stored in extra_properties.
  if (element_info.type == "view" && value === undefined) {
    value = element_info.extra_properties[name];
    if (value === undefined)
      value = g_view_property_defaults[name];
  }
  return value;
}

function GetElementTitle(element_info) {
  var title = element_info.type;
  if (element_info.name)
    title += " (" + element_info.name + ")";
  return title;
}

function SetElementName(element_info, name) {
  element_info.name = name;
  g_elements_view.SetNodeTitle(element_info.id,
                               GetElementTitle(element_info));
}

function InsertElementsViewNode(element_info, before_element) {
  var element = element_info.element;
  var parent_info = GetElementInfoParent(element_info);
  var level = parent_info ? g_elements_view.GetLevel(parent_info.id) + 1 : 0;
  g_elements_view.InsertNode(level, element_info.id,
                             GetElementTitle(element_info),
                             element.children && element.children.count,
                             before_element ? before_element.name : null);
}

function SetElementProperty(element_info, name, value) {
  var type = element_info.type;
  if (name == "name") {
    // "name" properties are specially handled.
    SetElementName(element_info, value);
    g_used_element_names[value] = true;
  } else if (name.substring(0, 2) == "on") {
    element_info.events[name] = value;
  } else {
    try {
      var meta = g_properties_meta[type][name];
      if (meta) {
        meta.set_value(element_info.element, value);
      } else {
        // This property is not supported by current designer.
        // Still try to let it be effective.
        element_info.element[name] = value;
        // The following will do only if the above statement hasn't raised
        // an exception.
        gadget.debug.warning("Property " + name + " for " + type +
                             " is not supported by designer.");
        element_info.extra_properties[name] = value;
      }
    } catch (e) {
      if (meta && type == "view") {
        // The e_designer_view is an element, so it may not support all
        // properties of the view.
        element_info.extra_properties[name] = value;
      } else {
        gadget.debug.error("Failed to set property: '" + name +
                           "' to '" + value + "' for " + type);
      }
    }
  }
}

function SetElementProperties(xml_element, element_info) {
  var property_lowercase_index =
      g_property_lowercase_index[element_info.type];
  var attrs = xml_element.attributes;
  var attr_count = attrs.length;
  for (var i = 0; i < attr_count; i++) {
    var attr = attrs.item(i);
    var name = attr.name;
    var formal_name = property_lowercase_index[name.toLowerCase()];
    if (formal_name)
      name = formal_name;
    SetElementProperty(element_info, name, attr.value);
  }
}

function ParseElement(xml_element, parent_info, insert_before) {
  var element_info = null;
  var type = xml_element.tagName;
  // listitem is deprecated by item.
  if (GadgetStrEquals(type, "listitem"))
    type = "item";
  type = g_type_lowercase_index[type.toLowerCase()];
  if (type && g_properties_meta[type]) {
    var id = NewElementId();
    var element = parent_info.element.insertElement(
        "<" + type + " name='" + id + "'/>", insert_before);
    if (element) {
      designerUtils.setDesignerMode(element);
      element_info = {
        element: element,
        id: id,
        type: type,
        extra_properties: { },
        events: { },
        before_comments: [ ],
        inner_comments: [ ]
      };
      g_elements_info[id] = element_info;

      SetElementProperties(xml_element, element_info);
      if (element.children) {
        ParseChildren(xml_element, element_info);
      } else {
        var text = Trim(xml_element.text);
        if (text) {
          try {
            element_info.element.innerText = text;
          } catch (e) {
            gadget.debug.error("Failed to set innerText for " + type);
          }
        }
      }
    } else {
      gadget.debug.error("Failed to create element: " + type);
    }
  }
  return element_info; 
}

function ParseScriptElement(xml_element) {
  var attrs = xml_element.attributes;
  var attr_count = attrs.length;
  for (var i = 0; i < attr_count; i++) {
    var attr = attrs.item(i);
    if (GadgetStrEquals(attr.name, "src")) {
      g_view_info.scripts.push({ src: attr.value });
      return;
    }
  }
  // No "src" attribute, try inline script.
  for (var child = xml_element.firstChild; child; child = child.nextSibling) {
    // Though only comment node is allowed to be inline scripts, here also
    // tolerate other types and they will be converted to valid scripts.
    if (Trim(child.text)) {
      // Only test if the trimmed text is not empty, but still save the not
      // trimmed text.
      g_view_info.scripts.push({ inline: child.text });
    }
  }
}

function ParseChildren(xml_element, parent_info) {
  var comments = [ ];
  for (var child = xml_element.firstChild; child; child = child.nextSibling) {
    switch (child.nodeType) {
      case 1: // ELEMENT_NODE
        if (GadgetStrEquals(child.tagName, "script")) {
          ParseScriptElement(child);
        } else {
          var child_info = ParseElement(child, parent_info, null);
          if (child_info) {
            child_info.before_comments = comments;
            comments = [ ];
          } else {
            // Convert unknown elements into comments.
            comments.push(child.xml);
          }
        }
        break;
      case 3: // TEXT_NODE
      case 8: // COMMENT_NODE
        // Ignore blank nodes. Non-blank extra text nodes will be converted
        // to comments.
        if (Trim(child.nodeValue)) {
          // Only test if the trimmed text is not empty, but still save the not
          // trimmed text.
          comments.push(child.nodeValue);
        }
        break;
      default:
        // Don't delete it, but preserve it as comment.
        comments.push(child.xml);
        break;
    }
  }
  // These comments is not before any child.
  parent_info.inner_comments = comments;
}

function ParseViewXMLDocument(doc) {
  g_elements_view = new Tree(e_elements_list, "elvw_", ExpandElement,
                             AddElementContextMenu);
  var view_element = doc.documentElement;
  if (!view_element)
    return false;
  InsertElementsViewNode(g_view_info, null);
  SetElementProperties(view_element, g_view_info);
  ParseChildren(view_element, g_view_info);
  g_elements_view.SetNodeHasChildren(
      g_view_info.id, e_designer_view.children.count > 0);

  var comments = g_view_info.before_comments;
  for (var node = doc.firstChild; node; node = node.nextSibling) {
    if (node.nodeType == 8)  // COMMENT_NODE
      comments.push(node.nodeValue);
    else if (node == view_element)
      comments = g_view_info.after_comments;
  }

  if (e_designer_view.children.count)
    SelectElement(GetElementInfo(e_designer_view.children.item(0)));
  else
    SelectElement(g_view_info);
  return true;
}

function ExpandElement(id) {
  var element_info = g_elements_info[id];
  var insert_before = GetNextElementInTree(element_info.element);
  var children = element_info.element.children;
  if (children) {
    var count = children.count;
    for (var i = 0; i < count; i++) {
      var child = children.item(i);
      InsertElementsViewNode(GetElementInfo(child), insert_before);
    }
  }
}

function ElementsViewSelectNode(element_info) {
  if (!g_elements_view)
    return;
  if (!element_info) {
    g_elements_view.SelectNode(null);
  } else {
    var path = [ ];
    var id = element_info.id;
    while (element_info != g_view_info &&
           !g_elements_view.GetNode(element_info.id)) {
      element_info = GetElementInfoParent(element_info);
      path.push(element_info.id);
    }
    for (var i = path.length - 1; i >= 0; i--)
      g_elements_view.ExpandFolder(path[i]);
    g_elements_view.SelectNode(id);
  }
}

function GetNewElementName(type) {
  if (!g_element_name_seqs[type])
    g_element_name_seqs[type] = 0;
  while (true) {
    var name = type + (++g_element_name_seqs[type]);
    if (!g_used_element_names[name]) {
      g_used_element_names[name] = true;
      return name;
    }
  }
}

// Note: These default values are just for a good initial state of elements.
// They are different from the API level default values.
// Don't repeat API level default values here to keep consistence.
kElementInitXML = {
  div: "<div name='&NAME;' width='32' height='32' background='#00CCEE'/>",
  img: "<img name='&NAME;' src='icon_large.png'/>",
  button: "<button name='&NAME;' width='64' height='22'" +
  		    " image='stock_images/button_up.png'" +
  		    " downImage='stock_images/button_down.png'" +
  		    " overImage='stock_images/button_over.png'/>",
  edit: "<edit name='&NAME;' width='64' height='16' value='Text'/>",
  checkbox: "<checkbox name='&NAME;' caption='&NAME;'" +
            " image='stock_images/checkbox_up.png'" +
            " downImage='stock_images/checkbox_down.png'" +
            " overImage='stock_images/checkbox_over.png'" +
            " checkedImage='stock_images/checkbox_checked_up.png'" +
            " checkedDownImage='stock_images/checkbox_checked_down.png'" +
            " checkedOverImage='stock_images/checkbox_checked_over.png'/>",
  radio: "<radio name='&NAME;' caption='&NAME;'" +
         " image='stock_images/radiobutton.png'" +
         " checkedImage='stock_images/radiobutton_checked.png'/>",
  a: "<a name='&NAME;' href='www.google.com'/>",
  label: "<label name='&NAME;'>&NAME;</label>",
  contentarea: "<contentarea name='&NAME;' width='100' height='100'/>",
  listbox: "<listbox name='&NAME;' width='100' height='80' autoscroll='true'" +
           " itemWidth='100%' itemHeight='20'>" +
           " <item><label>Label #0</label></item>" +
           " <item><label>Label #1</label></item>" +
           "</listbox>",
  combobox: "<combobox name='&NAME;' width='100' height='100'" +
            " itemWidth='100%' itemHeight='20' selectedIndex='0'>" +
            " <item><label>Label #0</label></item>" +
            " <item><label>Label #1</label></item>" +
            "</combobox>",
  progressbar: "<progressbar name='&NAME;' width='192' height='16'" +
               " emptyImage='stock_images/progressbar_empty.png'" +
               " fullImage='stock_images/progressbar_full.png'/>",
  scrollbar: "<scrollbar name='&NAME;' width='16' height='100'/>"
};

function NewElement(type, x, y) {
  g_elements_view.SetNodeHasChildren(g_view_info.id, true);
  g_elements_view.ExpandFolder(g_view_info.id);
  var name = GetNewElementName(type);
  var init_xml = kElementInitXML[type].replace(/&NAME;/gm, name);
  var domdoc = new DOMDocument();
  domdoc.loadXML(init_xml);
  var element_info = ParseElement(domdoc.documentElement, g_view_info, null);
  if (element_info) {
    InsertElementsViewNode(element_info, null);
    if (x != undefined && y != undefined) {
      element_info.element.x = x;
      element_info.element.y = y;
    }
  }
  return element_info;
}

function DeleteElement(element) {
  var id = element.name;
  g_elements_view.DeleteNode(id);
  var next = GetNextSibling(element);
  if (!next)
    next = GetPreviousSibling(element);
  if (!next) {
    next = element.parentElement;
    g_elements_view.SetNodeHasChildren(next.name, false);
  }
  // Don't delete from g_used_element_names, because the name may be also
  // used by another live element.
  delete g_elements_info[id];
  element.parentElement.removeElement(g_selected_element);
  return GetElementInfo(next);
}

function CommentsToDOM(domdoc, parent, comments) {
  for (var i in comments) {
    // Replace "--"s in comments to "- -" to avoid exception
    // raised by some dom parsers.
    var comment = comments[i].replace(/(-(?=-)|-$)/gm, "- ");
    parent.appendChild(domdoc.createComment(comment));
  }
}

function ElementToDOM(domdoc, parent_xml_element, element_info) {
  var element = element_info.element;
  var type = element_info.type;
  var xml_element = domdoc.createElement(type);
  var metainfo = g_properties_meta[type];
  for (var name in metainfo) {
    var value = GetElementProperty(element_info, name);
    if (value !== undefined &&
        value != metainfo[name].default_value) {
      if (name == "innerText")
        xml_element.appendChild(domdoc.createTextNode(value));
      else
        xml_element.setAttribute(name, value);
    }
  }
  for (var name in element_info.events) {
    var value = element_info.events[name];
    if (value)
      xml_element.setAttribute(name, value);
  }
  for (var name in element_info.extra_properties) {
    // The name should not in metadata. Check it just for safety.
    if (!(name in metainfo)) {
      var value = element_info.extra_properties[name];
      xml_element.setAttribute(name, value);
    }
  }
  CommentsToDOM(domdoc, parent_xml_element, element_info.before_comments); 
  parent_xml_element.appendChild(xml_element);

  var children = element.children;
  if (children) {
    for (var i = 0; i < children.count; i++)
      ElementToDOM(domdoc, xml_element, GetElementInfo(children.item(i)));
  }

  CommentsToDOM(domdoc, parent_xml_element, element_info.inner_comments); 
  return xml_element;
}

function ViewToDOMDoc() {
  var domdoc = new DOMDocument();
  ElementToDOM(domdoc, domdoc, g_view_info);
  CommentsToDOM(domdoc, domdoc, g_view_info.after_comments); 

  var docele = domdoc.documentElement;
  for (var i in g_view_info.scripts) {
    var src = g_view_info.scripts[i].src;
    if (src) {
      var script_ele = domdoc.createElement("script");
      script_ele.setAttribute("src", src);
      docele.appendChild(script_ele);
    } else {
      var inline = g_view_info.scripts[i].inline;
      if (inline) {
        var script_ele = domdoc.createElement("script");
        var inline_script = domdoc.createComment(inline);
        script_ele.appendChild(inline_script);
        docele.appendChild(script_ele);
      }
    }
  }
  return domdoc;
}

function ElementToXML(element_info) {
  var domdoc = new DOMDocument();
  return ElementToDOM(domdoc, domdoc, element_info).xml;
}

function ViewToXML() {
  return ViewToDOMDoc().xml;
}

function GetIndexInParent(element) {
  if (!element.parentElement)
    return 0;
  var children = element.parentElement.children;
  for (var i = 0; i < children.count; i++) {
    if (children.item(i) == element)
      return i;
  }
  // Should not occur.
  return -1;
}

function GetPreviousSibling(element) {
  if (!element.parentElement)
    return null;
  var index = GetIndexInParent(element);
  if (index <= 0)
    return null;
  return element.parentElement.children.item(index - 1);
}

function GetNextSibling(element) {
  if (!element.parentElement)
    return null;
  var index = GetIndexInParent(element);
  if (index < 0 || index >= element.parentElement.children.count)
    return null;
  return element.parentElement.children.item(index + 1);
}

function GetNextElementInTree(element) {
  while (element != e_designer_view) {
    var next = GetNextSibling(element);
    if (next)
      return next;
    element = element.parentElement;
  }
  return null;
}

function MoveElement(element, parent, before_element) {
  // Keep the element position unchanged within the view.
  var old_pos = designerUtils.elementCoordToAncestor(
      element.parentElement, e_designer_view, element.offsetX, element.offsetY);
  var new_pos = designerUtils.ancestorCoordToElement(
      e_designer_view, parent, old_pos.x, old_pos.y);
  if (typeof element.x != "string")
    element.x = new_pos.x;
  if (typeof element.y != "string")
    element.y = new_pos.y;

  var old_info = GetElementInfo(element);
  var parent_info = GetElementInfo(parent);
  g_elements_view.SetNodeHasChildren(parent_info.id, true);
  g_elements_view.ExpandFolder(parent_info.id);
  var xml = ElementToXML(GetElementInfo(element));
  var expand = false;
  if ((parent.tagName == "combobox" || parent.tagName == "listbox")) {
    if (element.tagName != "item") {
      xml = "<item>" + xml + "</item>";
      expand = true;
    }
  } else if (element.tagName == "item") {
    xml = xml.replace(/^\s*<item/m, "$1<div")
             .replace(/<\/item\s*>\s*$/m, "</div>$1");
    expand = true;
  }
  DeleteElement(element);
  var domdoc = new DOMDocument();
  domdoc.loadXML(xml);
  var new_info = ParseElement(domdoc.documentElement, parent_info,
                              before_element);
  new_info.before_comments = old_info.before_comments;
  var before_element_in_tree = before_element ?
                               before_element : GetNextElementInTree(parent);
  InsertElementsViewNode(new_info, before_element_in_tree);
  if (expand)
    g_elements_view.ExpandFolder(new_info.id);
  return new_info;
}

function AppendString(list, string) {
  g_elements_view.ExpandFolder(list.name);
  var domdoc = new DOMDocument();
  domdoc.loadXML("<item><label/></item>");
  var item_info = ParseElement(domdoc.documentElement, GetElementInfo(list),
                               null);
  item_info.element.children.item(0).innerText = string;
  InsertElementsViewNode(item_info, GetNextSibling(list));
  g_elements_view.ExpandFolder(item_info.id);
  return item_info;
}
