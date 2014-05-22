/*
  Copyright 2014 Google Inc.

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
#include "skin/composition_element.h"

#include <algorithm>
#include <vector>

#include "skin/skin_consts.h"
#include "third_party/google_gadgets_for_linux/ggadget/string_utils.h"
#include "third_party/google_gadgets_for_linux/ggadget/color.h"
#include "third_party/google_gadgets_for_linux/ggadget/text_formats.h"
#include "third_party/google_gadgets_for_linux/ggadget/text_frame.h"
#include "third_party/google_gadgets_for_linux/ggadget/variant.h"
#include "third_party/google_gadgets_for_linux/ggadget/view.h"

namespace ime_goopy {
namespace skin {
using ggadget::TextFormat;
using ggadget::Variant;

namespace {
static const double kClausePadding = 2.0;
static const double kSegmentationLabelPadding = 0;
static const double kDefaultCaretWidth = 1.0;
static const ggadget::Color kCaretColor(58/255.0, 184/255.0, 251/255.0);
static const char kTextAttrSperator = '_';
static const char *kClauseStatus[] = {
  "active", "inactive", "converted", "highlight",
};
static const char kSegmentationLabel[] = "segmentationlabel";

static const ggadget::FormatEntry kActiveTextFormat[] = {
  {TextFormat::kForegroundName, Variant("black")},
  {TextFormat::kSizeName, Variant(12.0)},
};

static const ggadget::FormatEntry kInactiveTextFormat[] = {
  {TextFormat::kForegroundName, Variant("dimgray")},
  {TextFormat::kSizeName, Variant(12.0)},
};

static const ggadget::FormatEntry kConvertedTextFormat[] = {
  {TextFormat::kForegroundName, Variant("black")},
  {TextFormat::kSizeName, Variant(10.0)},
};

static const ggadget::FormatEntry kHightlightTextFormat[] = {
  {TextFormat::kForegroundName, Variant("blue")},
  {TextFormat::kSizeName, Variant(12.0)},
};

static const std::string kTextAttrName[] = {
  TextFormat::kBoldName,
  TextFormat::kItalicName,
  TextFormat::kStrikeoutName,
  TextFormat::kUnderlineName,
  TextFormat::kForegroundName,
  TextFormat::kFontName,
  TextFormat::kSizeName,
};

struct FormatBoundary {
  int format_index;
  int code_point;
  bool end;
  bool overridable;
};

bool FormatBoundaryLeftTo(const FormatBoundary& a, const FormatBoundary& b) {
  return a.code_point < b.code_point ||
         (a.code_point == b.code_point && a.end && !b.end);
}

// Merges two sets of format ranges.
// If a text range is both covered by |formats| and |overridable_formats|, the
// format will be determined by |formats|. And if a text range is only covered
// by |overridable_formats|, the format will be determined by
// |overridable_formats|.
void MergeTextFormats(const ggadget::TextFormats& formats,
                      const ggadget::TextFormats& overridable_formats,
                      ggadget::TextFormats* out_formats) {
  out_formats->clear();
  std::vector<FormatBoundary> boundaries;
  for (size_t i = 0; i < formats.size(); ++i) {
    if (formats[i].range.Length() == 0)
      continue;
    FormatBoundary bound = {0};
    bound.format_index = i;
    bound.code_point = formats[i].range.start;
    boundaries.push_back(bound);
    bound.code_point = formats[i].range.end;
    bound.end = true;
    boundaries.push_back(bound);
  }
  for (size_t i = 0; i < overridable_formats.size(); ++i) {
    if (overridable_formats[i].range.Length() == 0)
      continue;
    FormatBoundary bound = {0};
    bound.format_index = i;
    bound.overridable = true;
    bound.code_point = overridable_formats[i].range.start;
    boundaries.push_back(bound);
    bound.code_point = overridable_formats[i].range.end;
    bound.end = true;
    boundaries.push_back(bound);
  }
  std::stable_sort(boundaries.begin(), boundaries.end(), FormatBoundaryLeftTo);
  int last_boundary = 0;
  TextFormat current_format;
  TextFormat current_overridable_format;
  std::vector<TextFormat> format_stack;
  std::vector<TextFormat> overridable_format_stack;
  for (size_t i = 0; i < boundaries.size(); ++i) {
    if (boundaries[i].code_point > last_boundary) {
      ggadget::TextFormatRange format_range;
      format_range.range.start = last_boundary;
      format_range.range.end = boundaries[i].code_point;
      format_range.format = current_format;
      format_range.format.MergeIfNotHave(current_overridable_format);
      out_formats->push_back(format_range);
    }
    last_boundary = boundaries[i].code_point;
    if (boundaries[i].end) {
      if (boundaries[i].overridable) {
        current_overridable_format = overridable_format_stack.back();
        overridable_format_stack.pop_back();
      } else {
        current_format = format_stack.back();
        format_stack.pop_back();
      }
    } else {
      if (boundaries[i].overridable) {
        overridable_format_stack.push_back(current_overridable_format);
        current_overridable_format.MergeFormat(
            overridable_formats[boundaries[i].format_index].format);
      } else {
        format_stack.push_back(current_format);
        current_format.MergeFormat(
            formats[boundaries[i].format_index].format);
      }
    }
  }
}

}  // namespace

class CompositionElement::Impl : public ggadget::SmallObject<> {
 public:
  Impl(CompositionElement *owner, ggadget::View *view)
      : owner_(owner),
        on_theme_changed_connection_(NULL),
        caret_color_(kCaretColor),
        caret_pos_(0),
        element_height_(0),
        element_width_(0),
        text_frame_(owner, view),
        update_element_size_(false),
        h_auto_sizing_(true),
        v_auto_sizing_(true) {
    clause_text_formats_[CompositionElement::ACTIVE].SetFormat(
        kActiveTextFormat, arraysize(kActiveTextFormat));
    clause_text_formats_[CompositionElement::INACTIVE].SetFormat(
        kInactiveTextFormat, arraysize(kInactiveTextFormat));
    clause_text_formats_[CompositionElement::CONVERTED].SetFormat(
        kConvertedTextFormat, arraysize(kConvertedTextFormat));
    clause_text_formats_[CompositionElement::HIGHLIGHT].SetFormat(
        kHightlightTextFormat, arraysize(kHightlightTextFormat));

    on_theme_changed_connection_ = view->ConnectOnThemeChangedEvent(
        NewSlot(this, &Impl::OnThemeChanged));
  }

  ~Impl() {
    if (on_theme_changed_connection_) {
      on_theme_changed_connection_->Disconnect();
      on_theme_changed_connection_ = NULL;
    }
    ClearText();
  }

  Variant GetWidth() const {
    return h_auto_sizing_ ? Variant("auto") : owner_->GetWidth();
  }

  void SetWidth(const Variant &width) {
    if (width.type() == Variant::TYPE_STRING &&
      ggadget::VariantValue<std::string>()(width) == "auto") {
      owner_->SetHorizontalAutoSizing(true);
    } else {
      h_auto_sizing_ = false;
      owner_->SetWidth(width);
    }
  }

  Variant GetHeight() const {
    return v_auto_sizing_ ? Variant("auto") : owner_->GetHeight();
  }

  void SetHeight(const Variant &height) {
    if (height.type() == Variant::TYPE_STRING &&
      ggadget::VariantValue<std::string>()(height) == "auto") {
      owner_->SetVerticalAutoSizing(true);
    } else {
      v_auto_sizing_ = false;
      owner_->SetHeight(height);
    }
  }

  void SetClauseTextAttribute(ClauseStatus clause_status,
                              TextAttribute text_attr,
                              const Variant &value) {
    TextFormat *text_format = &clause_text_formats_[clause_status];
    SetTextAttributeValue(text_attr, value, text_format);
    update_element_size_= true;
    owner_->UpdateUI();
  }

  Variant GetClauseTextAttribute(
      ClauseStatus clause_status,
      TextAttribute text_attr) const {
    return GetTextAttributeValue(clause_text_formats_[clause_status],
                                 text_attr);
  }

  void SetTextAttributeValue(TextAttribute text_attr,
                             const Variant &value,
                             TextFormat* text_format) {
    if (text_format == NULL) return;
    text_format->SetFormat(kTextAttrName[text_attr], value);
  }

  Variant GetTextAttributeValue(const TextFormat &text_format,
                                TextAttribute text_attr) const {
    return text_format.GetFormat(kTextAttrName[text_attr]);
  }

  void ClearText() {
    text_frame_.SetText("");
    update_element_size_= true;
  }

  // 'name' shoulde be 'kClauseStatus'_'kTextAttrName' or
  // 'kSegmentationLabel'_'KTextAttrName'
  Variant GetTextAttributeProperty(const char *name) {
    if (!name || !*name)
      return Variant();
    std::string attr_name(name);
    int pos = attr_name.find(kTextAttrSperator);
    if (pos == attr_name.npos)
      return Variant();
    std::string lower_status = ggadget::ToLower(attr_name.substr(0, pos));
    std::string lower_attr = ggadget::ToLower(attr_name.substr(pos + 1));
    CompositionElement::TextAttribute text_attr;
    if (!GetTextAttribute(lower_attr, &text_attr))
      return Variant();

    CompositionElement::ClauseStatus clause_status;
    if (GetClauseStatus(lower_status, &clause_status))
      return GetClauseTextAttribute(clause_status, text_attr);
    return Variant();
  }

  bool SetTextAttributeProperty(const char *name,
                                const Variant &value) {
    if (!name || !*name)
      return false;
    std::string attr_name(name);
    int pos = attr_name.find(kTextAttrSperator);
    if (pos == attr_name.npos)
      return false;
    std::string lower_status = ggadget::ToLower(attr_name.substr(0, pos));
    std::string lower_attr = ggadget::ToLower(attr_name.substr(pos + 1));
    CompositionElement::TextAttribute text_attr;
    if (!GetTextAttribute(lower_attr, &text_attr))
      return false;

    CompositionElement::ClauseStatus clause_status;
    if (GetClauseStatus(lower_status, &clause_status)) {
      SetClauseTextAttribute(clause_status, text_attr, value);
      return true;
    }
    return false;
  }

  bool GetClauseStatus(const std::string &name,
                       CompositionElement::ClauseStatus *clause_status) {
    if (clause_status == NULL)
      return false;
    for (size_t i = 0; i < arraysize(kClauseStatus); i++) {
      if (name == kClauseStatus[i]) {
        *clause_status = static_cast<CompositionElement::ClauseStatus>(i);
        return true;
      }
    }
    return false;
  }

  bool GetTextAttribute(const std::string &name,
                        CompositionElement::TextAttribute *text_attr) {
    if (text_attr == NULL)
      return false;
    // Get specific text attribute.
    for (size_t i = 0; i < arraysize(kTextAttrName); i++) {
      if (name == kTextAttrName[i]) {
        *text_attr = static_cast<CompositionElement::TextAttribute>(i);
        return true;
      }
    }
    return false;
  }

  void OnThemeChanged() {
    update_element_size_= true;
    owner_->UpdateUI();
  }

  CompositionElement *owner_;
  ggadget::Connection *on_theme_changed_connection_;

  ggadget::TextFrame text_frame_;
  ggadget::Color caret_color_;
  int caret_pos_;
  double element_height_;
  double element_width_;

  TextFormat clause_text_formats_[arraysize(kClauseStatus)];
  std::string text_;
  // Formats comes with composition text, provided by the ime component.
  ggadget::TextFormats formats_;
  // Formats corresponding the composition states. These formats are specified
  // skin and is overridable by |formats_|. We will first use |formats|, if some
  // text ranges are not covered by formats, we will use
  // |formats_from_composition_states_|
  ggadget::TextFormats formats_from_composition_states_;
  bool h_auto_sizing_ : 1;
  bool v_auto_sizing_ : 1;
  bool update_element_size_: 1;

 private:
  DISALLOW_EVIL_CONSTRUCTORS(Impl);
};

CompositionElement::CompositionElement(ggadget::View *view, const char *name)
    : ggadget::BasicElement(view, "composition", name, false),
      impl_(new Impl(this, view)) {
  SetDynamicPropertyHandler(
      ggadget::NewSlot(impl_, &Impl::GetTextAttributeProperty),
      ggadget::NewSlot(impl_, &Impl::SetTextAttributeProperty));
}

void CompositionElement::DoClassRegister() {
  ggadget::BasicElement::DoClassRegister();
  RegisterProperty("caretpos",
      ggadget::NewSlot(&CompositionElement::GetCaretPosition),
      ggadget::NewSlot(&CompositionElement::SetCaretPosition));
  RegisterProperty("caretcolor",
      ggadget::NewSlot(&CompositionElement::GetCaretColor),
      ggadget::NewSlot(&CompositionElement::SetCaretColor));
  RegisterProperty("width",
      ggadget::NewSlot(&Impl::GetWidth, &CompositionElement::impl_),
      ggadget::NewSlot(&Impl::SetWidth, &CompositionElement::impl_));
  RegisterProperty("height",
      ggadget::NewSlot(&Impl::GetHeight, &CompositionElement::impl_),
      ggadget::NewSlot(&Impl::SetHeight, &CompositionElement::impl_));
}

CompositionElement::~CompositionElement() {
  delete impl_;
  impl_ = NULL;
}

void CompositionElement::SetCompositionText(const std::string &text) {
  impl_->text_ = text;
  impl_->formats_.clear();
  impl_->formats_from_composition_states_.clear();
}

void CompositionElement::SetCompositionStatus(
    int start, int end, CompositionElement::ClauseStatus status) {
  ggadget::TextFormatRange format;
  format.format = impl_->clause_text_formats_[status];
  format.range.start = start;
  format.range.end = end;
  impl_->formats_from_composition_states_.push_back(format);
}

void CompositionElement::SetCompositionFormats(
    const ggadget::TextFormats &formats) {
  impl_->formats_ = formats;
}

void CompositionElement::DoDraw(ggadget::CanvasInterface *canvas) {
  double draw_text_x = 0;
  double draw_text_y = 0;
  double draw_text_height = GetPixelHeight();

  double width = 0;
  double height = 0;
  impl_->text_frame_.GetSimpleExtents(&width, &height);
  impl_->text_frame_.Draw(canvas, 0, 0, width, height);
  impl_->text_frame_.DrawCaret(canvas, impl_->caret_pos_, impl_->caret_color_);
}

void CompositionElement::SetClauseTextAttribute(
    ClauseStatus clause_status,
    TextAttribute text_attr,
    const Variant &value) {
  impl_->SetClauseTextAttribute(clause_status, text_attr, value);
}

Variant CompositionElement::GetClauseTextAttribute(
    ClauseStatus clause_status,
    TextAttribute text_attr) const {
  return impl_->GetClauseTextAttribute(clause_status, text_attr);
}

void CompositionElement::SetCaretPosition(int caret_pos) {
  if (impl_->caret_pos_ != caret_pos) {
    impl_->caret_pos_ = caret_pos;
    UpdateUI();
  }
}

int CompositionElement::GetCaretPosition() const {
  return impl_->caret_pos_;
}

void CompositionElement::SetCaretColor(const std::string &color) {
  ggadget::Color new_color;
  ggadget::Color::FromString(color.c_str(), &new_color, NULL);
  if (new_color != impl_->caret_color_) {
    impl_->caret_color_ = new_color;
    UpdateUI();
  }
}

std::string CompositionElement::GetCaretColor() const {
  return impl_->caret_color_.ToString();
}

bool CompositionElement::IsHorizontalAutoSizing() const {
  return impl_->h_auto_sizing_;
}

void CompositionElement::SetHorizontalAutoSizing(bool auto_sizing) {
  if (impl_->h_auto_sizing_ != auto_sizing) {
    impl_->h_auto_sizing_ = auto_sizing;
    UpdateUI();
  }
}

bool CompositionElement::IsVerticalAutoSizing() const {
  return impl_->v_auto_sizing_;
}

void CompositionElement::SetVerticalAutoSizing(bool auto_sizing) {
  if (impl_->v_auto_sizing_ != auto_sizing) {
    impl_->v_auto_sizing_ = auto_sizing;
    UpdateUI();
  }
}

void CompositionElement::Clear() {
  impl_->ClearText();
}

void CompositionElement::CalculateSize() {
  BasicElement::CalculateSize();
  if (impl_->update_element_size_) {
    double width = 0;
    double height = 0;
    impl_->text_frame_.SetRTL(IsTextRTL());
    impl_->text_frame_.GetSimpleExtents(&width, &height);
    if (impl_->v_auto_sizing_) {
      if (impl_->update_element_size_)
        impl_->element_height_ = height;
      SetPixelHeight(std::max(impl_->element_height_, GetMinHeight()));
    } else if (!HeightIsRelative()) {
      impl_->element_height_ = 0;
    }
    if (impl_->h_auto_sizing_) {
      if (impl_->update_element_size_)
        impl_->element_width_ = width;
      SetPixelWidth(std::max(impl_->element_width_, GetMinWidth()));
    } else if (!WidthIsRelative()) {
      impl_->element_width_ = 0;
    }
    impl_->update_element_size_ = false;
  }
}

double CompositionElement::GetMinWidth() const {
  return std::max(impl_->element_width_, ggadget::BasicElement::GetMinWidth());
}

double CompositionElement::GetMinHeight() const {
  return std::max(impl_->element_height_,
                  ggadget::BasicElement::GetMinHeight());
}

void CompositionElement::UpdateUI() {
  ggadget::TextFormats formats;
  MergeTextFormats(
      impl_->formats_, impl_->formats_from_composition_states_, &formats);
  impl_->update_element_size_ = impl_->text_frame_.SetTextWithFormats(
      impl_->text_, formats);
  QueueDraw();
}

ggadget::BasicElement *CompositionElement::CreateInstance(ggadget::View *view,
                                                          const char *name) {
  return new CompositionElement(view, name);
}

}  // namespace skin
}  // namespace ime_goopy
