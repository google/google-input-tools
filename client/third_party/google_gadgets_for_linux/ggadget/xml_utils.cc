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

#include <cstring>
#include <cmath>

#include "xml_utils.h"
#include "basic_element.h"
#include "object_element.h"     // Special process for object element.
#include "elements.h"
#include "file_manager_interface.h"
#include "gadget_consts.h"
#include "logger.h"
#include "scriptable_interface.h"
#include "script_context_interface.h"
#include "unicode_utils.h"
#include "view.h"
#include "xml_dom_interface.h"
#include "xml_parser_interface.h"

namespace ggadget {

static void SetScriptableProperty(ScriptableInterface *scriptable,
                                  ScriptContextInterface *script_context,
                                  const char *filename, int row, int column,
                                  const char *name, const char *value,
                                  const char *tag_name) {
  Variant prototype;
  ScriptableInterface::PropertyType prop_type =
      scriptable->GetPropertyInfo(name, &prototype);
  if (prop_type != ScriptableInterface::PROPERTY_NORMAL &&
      prop_type != ScriptableInterface::PROPERTY_DYNAMIC) {
    LOG("%s:%d:%d Can't set property %s for %s", filename, row, column,
        name, tag_name);
    return;
  }

  Variant str_value_variant(value);
  Variant property_value;
  switch (prototype.type()) {
    case Variant::TYPE_BOOL: {
      bool b;
      if (str_value_variant.ConvertToBool(&b)) {
        property_value = Variant(b);
      } else {
        LOG("%s:%d:%d: Invalid bool '%s' for property %s of %s",
            filename, row, column, value, name, tag_name);
        return;
      }
      break;
    }
    case Variant::TYPE_INT64: {
      int64_t i;
      if (str_value_variant.ConvertToInt64(&i)) {
        property_value = Variant(i);
      } else {
        LOG("%s:%d:%d: Invalid Integer '%s' for property %s of %s",
            filename, row, column, value, name, tag_name);
        return;
      }
      break;
    }
    case Variant::TYPE_DOUBLE: {
      double d;
      if (str_value_variant.ConvertToDouble(&d)) {
        property_value = Variant(d);
      } else {
        LOG("%s:%d:%d: Invalid double '%s' for property %s of %s",
            filename, row, column, value, name, tag_name);
        return;
      }
      break;
    }
    case Variant::TYPE_STRING:
      property_value = str_value_variant;
      break;

    case Variant::TYPE_VARIANT: {
      int64_t i;
      double d;
      bool b;
      if (!*value) {
        property_value = str_value_variant;
      } else if (strchr(value, '.') == NULL &&
                 str_value_variant.ConvertToInt64(&i)) {
        property_value = Variant(i);
      } else if (str_value_variant.ConvertToDouble(&d)) {
        property_value = Variant(d);
      } else if (str_value_variant.ConvertToBool(&b)) {
        property_value = Variant(b);
      } else {
        property_value = str_value_variant;
      }
      break;
    }
    case Variant::TYPE_SLOT: {
      if (script_context) {
        property_value = Variant(script_context->Compile(value, filename, row));
        break;
      } else {
        LOG("%s:%d:%d: Can't set script '%s' for property %s of %s: "
            "ScriptContext is not available.",
            filename, row, column, value, name, tag_name);
        return;
      }
    }

    default:
      LOG("%s:%d:%d: Unsupported type %s when setting property %s for %s",
          filename, row, column, prototype.Print().c_str(), name, tag_name);
      return;
  }

  if (!scriptable->SetProperty(name, property_value))
    LOG("%s:%d:%d: Can't set readonly property %s for %s",
        filename, row, column, name, tag_name);
}

void SetupScriptableProperties(ScriptableInterface *scriptable,
                               ScriptContextInterface *script_context,
                               const DOMElementInterface *xml_element,
                               const char *filename) {
  bool is_object = false;
  std::string tag_name = xml_element->GetTagName();
  // TODO(zkfan): support ObjectElement
#if defined(OS_POSIX) && !defined(GGL_FOR_GOOPY)
  if (scriptable->IsInstanceOf(ObjectElement::CLASS_ID)) {
    is_object = true;
    // The classId attribute must be set before any other attributes.
    std::string class_id = GetAttributeGadgetCase(xml_element, kClassIdAttr);
    if (class_id.size()) {
      down_cast<ObjectElement *>(scriptable)->SetObjectClassId(class_id);
    } else {
      LOG("%s:%d:%d: No classid is specified for the object element",
          filename, xml_element->GetRow(), xml_element->GetColumn());
    }
  }
#endif
  const DOMNamedNodeMapInterface *attributes = xml_element->GetAttributes();

  ASSERT(attributes);
  attributes->Ref();

  size_t length = attributes->GetLength();
  for (size_t i = 0; i < length; i++) {
    const DOMAttrInterface *attr =
        down_cast<const DOMAttrInterface *>(attributes->GetItem(i));
    std::string name = attr->GetName();
    if (GadgetStrCmp(kInnerTextProperty, name.c_str()) == 0) {
      LOG("%s:%d:%d: %s is not allowed in XML as an attribute: ",
          filename, attr->GetRow(), attr->GetColumn(), kInnerTextProperty);
      continue;
    }

    if (name.find('.') == name.npos &&
        GadgetStrCmp(kNameAttr, name.c_str()) != 0 &&
        (!is_object || GadgetStrCmp(kClassIdAttr, name.c_str()) != 0)) {
      SetScriptableProperty(scriptable, script_context, filename,
                            attr->GetRow(), attr->GetColumn(),
                            name.c_str(), attr->GetValue().c_str(),
                            tag_name.c_str());
    }
  }
  // "innerText" property is set in InsertElementFromDOM().

  // Deal with inner object properties after all normal properties, because
  // some object is only available when some other properties are set.
  // For example, div's scroll bar is only available when autoscroll='true'.
  for (size_t i = 0; i < length; i++) {
    const DOMAttrInterface *attr =
        down_cast<const DOMAttrInterface *>(attributes->GetItem(i));
    std::string name = attr->GetName();
    std::string object_name, property_name;
    if (SplitString(name, ".", &object_name, &property_name)) {
      ResultVariant object_v = scriptable->GetProperty(object_name.c_str());
      if (object_v.v().type() == Variant::TYPE_SCRIPTABLE) {
        ScriptableInterface *object =
            VariantValue<ScriptableInterface *>()(object_v.v());
        if (object) {
          SetScriptableProperty(object, script_context, filename,
                                attr->GetRow(), attr->GetColumn(),
                                property_name.c_str(), attr->GetValue().c_str(),
                                (tag_name + "." + object_name).c_str());
          continue;
        }
      }

      LOG("%s:%d:%d Can't set property %s for %s", filename,
          attr->GetRow(), attr->GetColumn(), name.c_str(), tag_name.c_str());
    }
  }

  attributes->Unref();
}

BasicElement *InsertElementFromDOM(Elements *elements,
                                   ScriptContextInterface *script_context,
                                   const DOMElementInterface *xml_element,
                                   const BasicElement *before,
                                   const char *filename) {
  std::string tag_name = xml_element->GetTagName();
  if (GadgetStrCmp(tag_name.c_str(), kScriptTag) == 0)
    return NULL;

  std::string name = GetAttributeGadgetCase(xml_element, kNameAttr);
  BasicElement *element = elements->InsertElement(tag_name.c_str(), before,
                                                  name.c_str());
  if (!element) {
    LOG("%s:%d:%d: Failed to create element %s", filename,
        xml_element->GetRow(), xml_element->GetColumn(), tag_name.c_str());
    return element;
  }

  SetupScriptableProperties(element, script_context, xml_element, filename);
  Elements *children = element->GetChildren();
  std::string text;
  for (const DOMNodeInterface *child = xml_element->GetFirstChild();
       child; child = child->GetNextSibling()) {
    DOMNodeInterface::NodeType type = child->GetNodeType();
    if (type == DOMNodeInterface::ELEMENT_NODE) {
      const DOMElementInterface *child_element =
          down_cast<const DOMElementInterface*>(child);
      std::string child_tag = child_element->GetTagName();
      // TODO(zkfan): support ObjectElement
#if defined(OS_POSIX) && !defined(GGL_FOR_GOOPY)
      // Special process for the child element (i.e. param element) of object
      // element. This is for the compatability with GDWin.
      // We set each param as a property of the real object wrapped in the
      // object element.
      if (element->IsInstanceOf(ObjectElement::CLASS_ID) &&
          GadgetStrCmp(child_tag.c_str(), kParamTag) == 0) {
        BasicElement *object = down_cast<ObjectElement*>(element)->GetObject();
        if (object) {
          std::string param_name = GetAttributeGadgetCase(child_element,
                                                          kNameAttr);
          std::string param_value = GetAttributeGadgetCase(child_element,
                                                           kValueAttr);
          if (param_name.empty() || param_value.empty()) {
            LOG("%s:%d:%d: No name or value specified for param",
                filename, child_element->GetRow(), child_element->GetColumn());
          } else {
            SetScriptableProperty(object, script_context, filename,
                                  child_element->GetRow(),
                                  child_element->GetColumn(),
                                  param_name.c_str(), param_value.c_str(),
                                  kParamTag);
          }
        } else {
          // DLOG instead of LOG, because this must be caused by missing
          // classId, which has been LOG'ed.
          DLOG("%s:%d:%d: No object has been created for the object element",
               filename, xml_element->GetRow(), xml_element->GetColumn());
        }
      } else if (children) {
#else
      if (children) {
#endif
        if (!InsertElementFromDOM(children, script_context,
                                  child_element,
                                  NULL, filename)) {
          // Treat unknown tag as text format tag.
          text += child->GetXML();
        }
      } else {
        text += child->GetXML();
      }
    } else if (type == DOMNodeInterface::TEXT_NODE ||
               type == DOMNodeInterface::CDATA_SECTION_NODE) {
      text += down_cast<const DOMTextInterface *>(child)->GetTextContent();
    }
  }
  // Set the "innerText" property.
  // Trimming is required for compatibility.
  text = TrimString(text);
  if (!text.empty()) {
    SetScriptableProperty(element, script_context, filename,
                          xml_element->GetRow(), xml_element->GetColumn(),
                          kInnerTextProperty, text.c_str(), tag_name.c_str());
  }
  return element;
}

std::string GetAttributeGadgetCase(const DOMElementInterface *element,
                                   const char *name) {
  ASSERT(element);
  ASSERT(name);
  if (!element || !name || !*name)
    return std::string();

#ifdef GADGET_CASE_SENSITIVE
  return element->GetAttribute(name);
#else
  const DOMNamedNodeMapInterface *attrs = element->GetAttributes();
  std::string result;
  if (attrs) {
    attrs->Ref();
    size_t size = attrs->GetLength();
    for (size_t i = 0; i < size; i++) {
      const DOMAttrInterface *attr =
          down_cast<const DOMAttrInterface *>(attrs->GetItem(i));
      if (GadgetStrCmp(attr->GetName().c_str(), name) == 0) {
        result = attr->GetValue();
        break;
      }
    }
    attrs->Unref();
  }
  return result;
#endif
}

} // namespace ggadget
