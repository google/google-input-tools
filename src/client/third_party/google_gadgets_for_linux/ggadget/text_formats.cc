/*
  Copyright 2011 Google Inc.

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

#include "text_formats.h"

#include <algorithm>
#include <list>

#include "gadget_consts.h"
#include "scoped_ptr.h"
#include "xml_dom_interface.h"
#include "xml_parser_interface.h"
#include "xml_utils.h"

namespace {

// Class to represent boundaries of TextFormatRange objects.
//
// A TextFormatRange object with range in (start, end) is represented by/ 2
// TextFormatBoundary objects: (pos=start, type=START) and
// (pos=end, type=END).
struct TextFormatBoundary {
  enum Type {
    START,
    END,
  };

  TextFormatBoundary(int pos,
                     int index,
                     Type type,
                     const ggadget::TextFormat *format)
      : pos_(pos),
        index_(index),
        type_(type),
        format_(format) {
  }

  bool operator < (const TextFormatBoundary &rhs) const {
    if (pos_ != rhs.pos_) {
      return pos_ < rhs.pos_;
    }
    if (type_ != rhs.type_) {
      return type_ == END;
    }
    return index_ < rhs.index_;
  }

  int pos_;
  int index_;  // original index of format_
  const ggadget::TextFormat *format_;
  Type type_;
};

// List of <merged format, original format>
typedef std::list<std::pair<ggadget::TextFormat, const ggadget::TextFormat*> >
    MergedFormatList;

} // namespace

namespace ggadget {

#define DECLARE_FORMAT(id, type, name, capital_name, default_value) \
const std::string TextFormat::k##capital_name##Name = #name;
#include "text_formats_decl.h"
#undef DECLARE_FORMAT

TextFormat::TextFormat()
  : flag_(0),
    default_format_(NULL) {
#define DECLARE_FORMAT(id, type, name, capital_name, default_value) \
  name##_ = default_value;
#include "text_formats_decl.h"
#undef DECLARE_FORMAT
}

inline bool VariantTobool(const Variant& v) {
  bool value = false;
  v.ConvertToBool(&value);
  return value;
}

inline double VariantTodouble(const Variant& v) {
  double value = 0;
  v.ConvertToDouble(&value);
  return value;
}

inline std::string VariantTostring(const Variant& v) {
  std::string value;
  v.ConvertToString(&value);
  return value;
}

inline ScriptType VariantToScriptType(const Variant& v) {
  int value = 0;
  v.ConvertToInt(&value);
  return static_cast<ScriptType>(value);
}

inline Color VariantToColor(const Variant& v) {
  std::string value;
  v.ConvertToString(&value);
  Color color;
  double alpha = 0;
  Color::FromString(value.c_str(), &color, &alpha);
  return color;
}

template<typename type>
inline Variant ToVariant(type value) {
  return Variant(value);
}

inline Variant ToVariant(Color value) {
  return Variant(value.ToString());
}

inline Variant ToVariant(ScriptType value) {
  return Variant(static_cast<int>(value));
}

void TextFormat::SetFormat(const FormatEntry* formats, int count) {
  for (int i = 0; i < count; ++i)
    SetFormat(formats[i].format_name, formats[i].value);
}

void TextFormat::SetFormat(const std::string& format_name,
                           const Variant& value) {
#define DECLARE_FORMAT(id, type, name, capital_name, default_value) \
  if (k##capital_name##Name == format_name)                         \
    set_##name(VariantTo##type(value));
#include "text_formats_decl.h"
#undef DECLARE_FORMAT
}

Variant TextFormat::GetFormat(const std::string& format_name) const {
#define DECLARE_FORMAT(id, type, name, capital_name, default_value) \
  if (k##capital_name##Name == format_name)                         \
    return ToVariant(name());
#include "text_formats_decl.h"
#undef DECLARE_FORMAT
  return Variant();
}

void TextFormat::set_default_format(const TextFormat* default_format) {
  default_format_ = default_format;
  ASSERT(!default_format || !default_format_->default_format_);
}

void TextFormat::MergeFormat(const TextFormat& new_format) {
#define DECLARE_FORMAT(id, type, name, capital_name, default_value) \
  if (new_format.has_##name()) set_##name(new_format.name());
#include "text_formats_decl.h"
#undef DECLARE_FORMAT
}

void TextFormat::MergeIfNotHave(const TextFormat& format) {
#define DECLARE_FORMAT(id, type, name, capital_name, default_value) \
  if (format.has_##name() && !has_##name())                         \
    set_##name(format.name());
#include "text_formats_decl.h"
#undef DECLARE_FORMAT
}

void ParseXMLNode(const DOMElementInterface* xml_node,
                  const TextFormat* default_format,
                  std::string* text,
                  TextFormats* formats) {
  TextFormat current_format;
  std::string tag_name = xml_node->GetTagName();
  bool format_changed = true;
  if (tag_name == "font" || tag_name == "span") {
    const DOMNamedNodeMapInterface* attributes = xml_node->GetAttributes();
    if (attributes) {
      for (size_t i = 0; i < attributes->GetLength(); i++) {
        const DOMAttrInterface *attr =
            down_cast<const DOMAttrInterface *>(attributes->GetItem(i));
        std::string attr_name = attr->GetName();
        Variant str_value_variant(attr->GetValue());
        if (attr_name == "face" || attr_name == "font") {
          current_format.set_font(attr->GetValue());
        } else if (attr_name == "color" ||
                   attr_name == "foreground" ||
                   attr_name == "fgcolor") {
          Color color;
          Color::FromString(attr->GetValue().c_str(), &color, NULL);
          current_format.set_foreground(color);
        } else if (attr_name == "bgcolor" || attr_name == "background") {
          Color color;
          Color::FromString(attr->GetValue().c_str(), &color, NULL);
          current_format.set_background(color);
        } else if (attr_name == "underline_color") {
          Color color;
          Color::FromString(attr->GetValue().c_str(), &color, NULL);
          current_format.set_underline_color(color);
        } else if (attr_name == "strikeout_color") {
          Color color;
          Color::FromString(attr->GetValue().c_str(), &color, NULL);
          current_format.set_strikeout_color(color);
        } else if (attr_name == "size") {
          double value = 0;
          str_value_variant.ConvertToDouble(&value);
          current_format.set_size(value);
        } else if (attr_name == "underline") {
          current_format.set_underline(attr->GetValue() == "single");
        } else if (attr_name == "strikethrough" && attr_name == "strikeout") {
          bool value = true;
          str_value_variant.ConvertToBool(&value);
          current_format.set_strikeout(value);
        } else if (attr_name == "style") {
          current_format.set_italic(attr->GetValue() == "italic");
        } else if (attr_name == "weight") {
          current_format.set_bold(attr->GetValue() == "bold");
        } else if (attr_name == "rise") {
          double rise = 0;
          str_value_variant.ConvertToDouble(&rise);
          current_format.set_rise(rise);
        } else {
          format_changed = false;
        }
      }
    }
  } else if (tag_name == "b") {
    current_format.set_bold(true);
  } else if (tag_name == "i") {
    current_format.set_italic(true);
  } else if (tag_name == "sub") {
    current_format.set_script_type(SUBSCRIPT);
  } else if (tag_name == "sup") {
    current_format.set_script_type(SUPERSCRIPT);
  } else if (tag_name == "ins" || tag_name == "u") {
    current_format.set_underline(true);
  } else if (tag_name == "del" || tag_name == "s") {
    current_format.set_strikeout(true);
  } else {
    format_changed = false;
  }
  size_t start = 0;
  UTF16String text_utf16;
  TextFormatRange* format_range = NULL;
  int format_index = formats->size();
  if (format_changed) {
    formats->resize(formats->size() + 1);
    (*formats)[format_index].format = current_format;
    (*formats)[format_index].format.set_default_format(default_format);
    ConvertStringUTF8ToUTF16(*text, &text_utf16);
    (*formats)[format_index].range.start = text_utf16.length();
    start = text->length();
  }

  for (const DOMNodeInterface *child = xml_node->GetFirstChild();
       child; child = child->GetNextSibling()) {
    DOMNodeInterface::NodeType type = child->GetNodeType();
    if (type == DOMNodeInterface::ELEMENT_NODE) {
      ParseXMLNode(down_cast<const DOMElementInterface*>(child),
                   default_format, text, formats);
    } else if (type == DOMNodeInterface::TEXT_NODE ||
               type == DOMNodeInterface::CDATA_SECTION_NODE) {
      text->append(
          down_cast<const DOMTextInterface*>(child)->GetTextContent());
    }
  }
  if (format_changed && text->length() > start) {
    ConvertStringUTF8ToUTF16(text->c_str() + start,
                             text->length() - start,
                             &text_utf16);
    (*formats)[format_index].range.end =
        (*formats)[format_index].range.start + text_utf16.length();
  } else if (format_changed) {
    formats->erase(formats->begin() + format_index);
  }
}

bool ParseMarkUpText(const std::string& mark_up_text,
                     const TextFormat* base_format,
                     std::string* text,
                     TextFormats* formats) {
  std::string xml_string = StringPrintf("<t>%s</t>", mark_up_text.c_str());
  XMLParserInterface* parser = GetXMLParser();
  if (!parser)
    return false;
  DOMDocumentInterface* xmldoc = GetXMLParser()->CreateDOMDocument();
  xmldoc->SetPreserveWhiteSpace(true);
  xmldoc->Ref();
  if (!parser->ParseContentIntoDOM(xml_string, NULL, xml_string.c_str(), NULL,
                                   NULL, NULL, xmldoc, NULL, NULL)) {
    xmldoc->Unref();
    return false;
  }
  DOMNodeInterface* xml_node = xmldoc->GetFirstChild();
  if (!xml_node) {
    xmldoc->Unref();
    return false;
  }
  ParseXMLNode(down_cast<const DOMElementInterface*>(xml_node),
               base_format, text, formats);
  xmldoc->Unref();
  return true;
}

TextFormats NormalizeTextFormats(const TextFormats& formats) {
  int first_start = formats.empty() ? 0 : formats[0].range.start;
  std::vector<TextFormatBoundary> boundaries;
  boundaries.reserve(formats.size() * 2 + 2);

  for (size_t i = 0; i < formats.size(); i++) {
    const TextFormatRange& tfr = formats[i];
    if (tfr.range.Length() <= 0)
      continue;
    TextFormatBoundary start(
        tfr.range.start,
        i + 1, // in case we are adding an empty format range starting from 0
        TextFormatBoundary::START,
        &tfr.format);
    TextFormatBoundary end(
        tfr.range.end,
        i + 1,
        TextFormatBoundary::END,
        &tfr.format);
    boundaries.push_back(start);
    boundaries.push_back(end);
    first_start = std::min(first_start, tfr.range.start);
  }

  // If there is no format starting at 0, add an empty one for convinience
  if (first_start > 0) {
    TextFormat empty_format;
    TextFormatBoundary start(0,
                             0,
                             TextFormatBoundary::START,
                             &empty_format);
    TextFormatBoundary end(first_start,
                           0,
                           TextFormatBoundary::END,
                           &empty_format);
    boundaries.push_back(start);
    boundaries.push_back(end);
  }

  std::sort(boundaries.begin(), boundaries.end());

  MergedFormatList merged_formats;
  std::vector<MergedFormatList::iterator> index_format_mapping(
      formats.size() + 1, merged_formats.end());
  TextFormats normalized_formats;
  TextFormat current_format;
  int last_boundary_pos = -1;
  TextFormatRange normalized_format_range;

  // Add an empty format as a base format for convinience
  merged_formats.push_back(
      std::make_pair(current_format, reinterpret_cast<TextFormat*>(NULL)));
  // Keep tracking the last appended item
  MergedFormatList::iterator back_iter = merged_formats.begin();

  for (size_t i = 0; i < boundaries.size(); ++i) {
    const TextFormatBoundary& boundary = boundaries[i];

    if (i > 0 && last_boundary_pos != boundary.pos_) {
      TextFormatRange new_format;
      new_format.range.start = last_boundary_pos;
      new_format.range.end = boundary.pos_;
      new_format.format = current_format;
      normalized_formats.push_back(new_format);
    }

    switch (boundary.type_) {
    case TextFormatBoundary::START:
      current_format.MergeFormat(*boundary.format_);
      merged_formats.push_back(
          std::make_pair(current_format, boundary.format_));
      ++back_iter;
      index_format_mapping[boundary.index_] = back_iter;
      break;
    case TextFormatBoundary::END:
      MergedFormatList::iterator iter = index_format_mapping[boundary.index_];
      iter = merged_formats.erase(iter);
      MergedFormatList::iterator prev_iter = iter;
      prev_iter--;
      for (; iter != merged_formats.end(); ++iter, ++prev_iter) {
        iter->first = prev_iter->first;
        iter->first.MergeFormat(*iter->second);
      }
      back_iter = prev_iter; // In case we removed the last element
      current_format = back_iter->first;
      break;
    }
    last_boundary_pos = boundary.pos_;
  }
  return normalized_formats;
}

}  // namespace ggadget
