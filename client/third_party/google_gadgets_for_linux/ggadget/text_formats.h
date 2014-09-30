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

#ifndef GGADGET_TEXT_FORMATS_H__
#define GGADGET_TEXT_FORMATS_H__

#include <string>
#include <vector>

#include <ggadget/color.h>
#include <ggadget/font_interface.h>
#include <ggadget/gadget_consts.h>
#include <ggadget/variant.h>

namespace ggadget {

// Using std::string to avoid std::string in macro arguments.
using std::string;

struct FormatEntry {
  std::string format_name;
  ggadget::Variant value;
};

enum ScriptType {
  SUPERSCRIPT,
  NORMAL,
  SUBSCRIPT,
};

// A struct that contains a set of text format.
// For each format attribute, for example, 'font', you can access the attribute
// using TextFormat::font(), TextFormat::set_font(), TextFormat::has_font().
// The struct also have a link to a default format, if some attribute is not set
// in format, the method TextFormat::[attribute_name] will return the value of
// the attribute in default format.
struct TextFormat {
 public:
  TextFormat();
  // Sets the default format for the format objects, we will get the attributes
  // that is not set in this object from the default_format.
  // Notice that |default_format| should not have default format, to avoid loop.
  void set_default_format(const TextFormat* default_format);
  // Merges the attributes in |new_format| to the format object, attributes that
  // is both in the |new_format| and the format object will be overwritten.
  void MergeFormat(const TextFormat& new_format);
  // Merges the attributes in |new_format| to the format object, attributes that
  // is both in the |new_format| and the format object will NOT be overwritten.
  void MergeIfNotHave(const TextFormat& format);
  // Set format from a format entry array.
  void SetFormat(const FormatEntry* entries, int count);
  // Set format attribute indicated by |format_name| with |value|.
  void SetFormat(const std::string& format_name, const Variant& value);
  // Get the value of format attribute named |format_name|.
  Variant GetFormat(const std::string& format_name) const;

#define DECLARE_FORMAT(id, type, name, capital_name, default_value)    \
 public:                                                               \
  type name() const {                                                  \
    if (has_##name())                                                  \
      return name##_;                                                  \
    else if (default_format_)                                          \
      return default_format_->name();                                  \
    else                                                               \
      return default_value;                                            \
  }                                                                    \
  void set_##name(const type& value) {                                 \
    flag_ |= (1 << k##capital_name##ID);                               \
    name##_ = value;                                                   \
  }                                                                    \
  bool has_##name() const {                                            \
    return flag_ & (1 << k##capital_name##ID);                         \
  }                                                                    \
  static const int k##capital_name##ID = id;                           \
  static const std::string k##capital_name##Name;                      \
                                                                       \
 private:                                                              \
  type name##_;

#include <ggadget/text_formats_decl.h>

#undef DECLARE_FORMAT

 private:
  int flag_;
  const TextFormat* default_format_;
};

// A range of text. For convenience of Windows and Mac, the range is count in
// UTF-16 code points.
struct Range {
  int start;
  int end;
  int Length() const {
    return end - start;
  }
};

struct TextFormatRange {
  TextFormat format;
  Range range;
};

typedef std::vector<TextFormatRange> TextFormats;

// Parse a piece of mark up text into raw text and format ranges.
// The mark up text supported here is similar with the pango mark up language
// and html font tag, with little difference. The tag and corresponding
// attributes are defined as follow:
// tag <font> or <span>: indicates the formats of surrounded text.
// its attribues:
//   "face", "font": the name of the font family.
//   "size": the size(in points) of the font.
//   "style": The slant style - should be 'normal' or 'italic'.
//   "weight": The font weight - should be 'normal' or 'bold'.
//   "color", "fgcolor", "foreground": the color of the text. In html format.
//   "bgcolor", "background": the color of the background.
//   "underline_color", "strikeout_color": the color of underline or strikeout.
//   "underline": the underline style - should be 'none' or 'single'.
//   "strikethrough": 'true' or 'false' whether to strike through the text.
//   "rise": the verticval displacement from the baseline. In points.
//   "underline_color": the color of underline.
//   "strikeout_color": the color of strikeout.
// We also support some convinience tags as follow:
//   <b>: make the text bold
//   <i>: make the text italic
//   <s>, <del>: strikethrough the text
//   <u>, <ins>: underline the text
//   <sub>: subscript the text
//   <sup>: superscript the text
// For example, mark_up_text can be:
// "This <font color='#FF0000'>is </font> <i>mark-up</i> text"
// The text with no format tag will be set to base_format.
// Returns true if text successfully parsed.

bool ParseMarkUpText(const std::string& mark_up_text,
                     const TextFormat* base_format,
                     std::string* text,
                     TextFormats* formats);

// Normalize TextFormats
//
// The ranges of normalized TextFormats do not overlap with each
// other. The if there are more than one text formats in the same
// range in the original formats, they will be merged in the
// normalized one.
//
// For example:
//                 |---------A---------|
//       |----------B--------|--------C--------|
// |------------------------D----------------------------|
//
// Will be normalized to:
// |- D -|-- D+B --|- D+B+A -|- D+A+C -|- D+C -|--- D ---|
TextFormats NormalizeTextFormats(const TextFormats& formats);
}  // namespace ggadget

#endif  // GGADGET_TEXT_FORMATS_H__
