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

// Format of metadata:
// <element name>: {
//   inherits: "<inherited element name>",
//   properties: {
//     <property name>: null,
//   or
//     <property name>: [ <enumerated values> ],
//   or
//     <property name>: <value handler function> ],
//   ...
//   events: [ <list of names of supported events> ]
// ...

var kGadgetFileManagerPrefix = "gadget://";

function ChooseImage(value) {
  return ChooseColorImage(value, false, true);
}

function ChooseColorOrImage(value) {
  return ChooseColorImage(value, true, true);
}

// It's not feasible to detect default value of view's properties,
// so define them here.
var g_view_property_defaults = {
  caption: "",
  height: 0,
  width: 0,
  resizable: "zoom",
  showCaptionAlways: false
};

var g_metadata = {
  view: {
    properties: {
      caption: null,
      height: null,
      width: null,
      resizable: [ "false", "true", "zoom" ],
      resizeBorder: null,
      showCaptionAlways: null
    },
    events: [
      "oncancel", "onclick", "onclose", "ondblclick", "ondock", "onkeydown",
      "onkeypress", "onkeyup", "onminimize", "onmousedown", "onmousemove",
      "onmouseout", "onmouseover", "onmouseup", "onmousewheel", "onok",
      "onopen", "onoptionchanged", "onpopin", "onpopout", "onrclick",
      "onrdblclick", "onrestore", "onsize", "onsizing", "onthemechanged",
      "onundock"
    ]
  },

  basicElement_: {
    properties: {
      cursor: [
        "arrow", "ibeam", "wait", "cross", "uparrow", "size", "sizenwse",
        "sizenesw", "sizewe", "sizens", "sizeall", "no", "hand", "busy",
        "help"
      ],
      dropTarget: null,
      enabled: null,
      height: null,
      hitTest: [
        "httransparent", "htnowhere", "htclient", "htcaption", "htsysmenu",
        "htsize", "htmenu", "hthscroll", "htvscroll", "htminbutton",
        "htmaxbutton", "htleft", "htright", "httop", "httopleft",
        "httopright", "htbottom", "htbottomleft", "htbottomright", "htborder",
        "htobject", "htclose", "hthelp"
      ],
      mask: ChooseImage,
      name: null, // Should be specially treated.
      opacity: null,
      pinX: null,
      pinY: null,
      rotation: null,
      tooltip: null,
      width: null,
      height: null,
      visible: null,
      x: null,
      y: null
    },
    events: [
      "onclick", "ondblclick", "ondragdrop", "ondragout", "ondragover",
      "onfocusin", "onfocusout", "onkeydown", "onkeypress", "onkeyup",
      "onmousedown", "onmousemove", "onmouseout", "onmouseover", "onmouseup",
      "onmousewheel", "onrclick", "onrdblclick", "onsize"
    ]
  },

  textBase_: { // Not an actual element type.
    inherits: "basicElement_",
    properties: {
      align: [ "left", "center", "right", "justify" ],
      bold: null,
      color: ChooseColorOrImage,
      font: null,
      italic: null,
      size: null,
      strikeout: null,
      trimming: [
        "none", "character", "work", "character-ellipsis", "word-ellipsis",
        "path-ellipsis"
      ],
      underline: null,
      vAlign: [ "top", "middle", "bottom" ],
      wordWrap: null
    },
    events: [ ]
  },

  button: {
    inherits: "textBase_",
    properties: {
      caption: null,
      // defaultRendering: null,
      disabledImage: ChooseImage,
      downImage: ChooseImage,
      image: ChooseImage,
      overImage: ChooseImage
    },
    events: [ ]
  },

  checkbox: {
    inherits: "textBase_",
    properties: {
      caption: null,
      image: ChooseImage,
      checkedDisabledImage: ChooseImage,
      checkedDownImage: ChooseImage,
      checkedOverImage: ChooseImage,
      checkedImage: ChooseImage,
      // defaultRendering: null,
      disabledImage: ChooseImage,
      downImage: ChooseImage,
      overImage: ChooseImage,
      value: null
    },
    events: [ "onchange" ]
  },

  contentarea: {
    inherits: "basicElement_",
    properties: {
      maxContentItems: null
    },
    events: [ ]
  },

  div: {
    inherits: "basicElement_",
    properties: {
      autoscroll: null,
      background: ChooseColorOrImage
    },
    events: [ ]
  },

  edit: {
    inherits: "textBase_",
    properties: {
      background: ChooseColorOrImage,
      detectUrls: null,
      multiline: null,
      passwordChar: null,
      readonly: null,
      scrolling: null,
      value: null
    },
    events: [ "onchange" ]
  },

  img: {
    inherits: "basicElement_",
    properties: {
      colorMultiply: null,
      cropMaintainAspect: [ "false", "true", "photo" ],
      src: ChooseImage
    },
    events: [ ]
  },

  label: {
    inherits: "textBase_",
    properties: {
      innerText: null
    },
    events: [ ]
  },

  a: {
    inherits: "label",
    properties: {
      cursor: [
        "arrow", "ibeam", "wait", "cross", "uparrow", "size", "sizenwse",
        "sizenesw", "sizewe", "sizens", "sizeall", "no", "hand", "busy", "help"
      ],
      href: null,
      overColor: ChooseColorOrImage
    },
    events: [ ]
  },

  progressbar: {
    inherits: "basicElement_",
    properties: {
      // defaultRendering: null,
      emptyImage: ChooseImage,
      fullImage: ChooseImage,
      max: null,
      min: null,
      orientation: [ "horizontal", "vertical" ],
      thumbDisabledImage: ChooseImage,
      thumbDownImage: ChooseImage,
      thumbImage: ChooseImage,
      thumbOverImage: ChooseImage,
      value: null
    },
    events: [ "onchange" ]
  },

  scrollbar: {
    inherits: "basicElement_",
    properties: {
      background: ChooseImage,
      // defaultRendering: null,
      leftDownImage: ChooseImage,
      leftImage: ChooseImage,
      leftOverImage: ChooseImage,
      lineStep: null,
      max: null,
      min: null,
      orientation: [ "horizontal", "vertical" ],
      rightDownImage: ChooseImage,
      rightImage: ChooseImage,
      rightOverImage: ChooseImage,
      thumbDownImage: ChooseImage,
      thumbImage: ChooseImage,
      thumbOverImage: ChooseImage,
      value: null
    },
    events: [ "onchange" ]
  },

  listbox: {
    inherits: "div",
    properties: {
      itemHeight: null,
      itemOverColor: ChooseColorOrImage,
      itemSelectedColor: ChooseColorOrImage,
      itemSeparator: null,
      itemSeparatorColor: ChooseColorOrImage,
      itemWidth: null,
      multiSelect: null,
      selectedIndex: null
    },
    events: [ "onchange" ]
  },

  item: {
    inherits: "basicElement_",
    properties: {
      background: ChooseColorOrImage
    },
    events: [ ]
  },

  radio: {
    inherits: "checkbox",
    properties: { },
    events: [ ]
  },

  combobox: {
    inherits: "listbox",
    properties: {
      autoscroll: undefined,
      droplistVisible: null,
      itemSelectedColor: undefined,
      maxDroplistItems: null,
      multiSelect: undefined,
      type: [ "dropdown", "droplist" ],
      value: null
    },
    events: [ "ontextchange" ]
  }
};

// key: element type;
// value: {
//   <property name>: {
//     default_value: <default value>,
//     extra_info: <enumeration value array or value handler function>,
//     set_value: <function to put a property value to an element,
//                 prototype is like "void set_value(element, value)">
//     get_value: <function to get a property value from an element,
//                 prototype is like "value get_value(element)"> 
//   },
//   ...
// }
var g_properties_meta = { };
var g_events_meta = { };
// key: lowercase element type name, value: formal element type name. 
var g_type_lowercase_index = { };
// key: element type, sorted.
// value: {
//   <lowercase property name>: <formal property name>,
//   ... 
// }
var g_property_lowercase_index = { }; 

// Preprocess metadata.
function InitMetadata() {
  function HandleInherits(metadata) {
    if (metadata.inherits) {
      var inherited_metadata = g_metadata[metadata.inherits];
      HandleInherits(inherited_metadata);
      var inherited_properties = inherited_metadata.properties;
      for (var i in inherited_properties) {
        if (!(i in metadata.properties))
          metadata.properties[i] = inherited_properties[i];
      }
      Array.prototype.push.apply(metadata.events, inherited_metadata.events);
      delete metadata.inherits;
    }
  }

  function SetImage(value) {
    return value ? kGadgetFileManagerPrefix + value : "";
  }
  function GetImage(value) {
    return value ? value.substring(kGadgetFileManagerPrefix.length) : "";
  }
  function SetColorOrImage(value) {
    return value ? (value.indexOf('.') == -1 ? value : SetImage(value)) : "";
  }
  function GetColorOrImage(value) {
    return value ? (value.indexOf('.') == -1 ? value : GetImage(value)) : "";
  }

  function InitGetterSetter(property, name) {
    switch (property.extra_info) {
      case ChooseImage:
        properties[name].set_value = function(element, value) {
          element[name] = SetImage(value);
        };
        properties[name].get_value = function(element) {
          return GetImage(element[name]);
        };
        break;
      case ChooseColorOrImage:
        properties[name].set_value = function(element, value) {
          element[name] = SetColorOrImage(value);
        };
        properties[name].get_value = function(element) {
          return GetColorOrImage(element[name]);
        };
        break;
      default:
        properties[name].set_value = function(element, value) {
          element[name] = value;
        };
        properties[name].get_value = function(element) {
          return element[name];
        };
        break;
    }
  }

  for (var type in g_metadata) {
    var metadata = g_metadata[type];
    HandleInherits(metadata);
    if (type.charAt(type.length - 1) == '_')
      continue;

    g_type_lowercase_index[type.toLowerCase()] = type;
    metadata.events.sort();
    var lowercase_index = g_property_lowercase_index[type] = { };
    var events = g_events_meta[type] = { };
    for (var i in metadata.events) {
      var name = metadata.events[i];
      // Now treat events as normal string properties.
      events[name] = { default_value: "" };
      lowercase_index[name.toLowerCase()] = name;
    }
    var property_names = [ ];
    var src_properties = metadata.properties;
    for (var i in src_properties) {
      if (src_properties[i] !== undefined)
        property_names.push(i);
    }
    property_names.sort();
    var properties = g_properties_meta[type] = { };
    var default_values =
        type == "view" ? g_view_property_defaults :
        e_for_metadata.children.item("e_for_metadata_" + type);
    if (type == "item")
      default_values = default_values.children.item(0);
    for (var i in property_names) {
      var name = property_names[i];
      properties[name] = {
        default_value: (name == "name" ? "" : default_values[name]),
        extra_info: src_properties[name]
      };
      InitGetterSetter(properties[name], name);
      lowercase_index[name.toLowerCase()] = name;
    }
  }
  // g_metadata is no more used.
  g_metadata = null;
  e_for_metadata.parentElement.removeElement(e_for_metadata);
}
