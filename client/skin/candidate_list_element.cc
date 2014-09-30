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

#include "skin/candidate_list_element.h"

#include <string>
#include <vector>

#include "skin/candidate_element.h"
#include "skin/skin_consts.h"
#include "third_party/google_gadgets_for_linux/ggadget/canvas_interface.h"
#include "third_party/google_gadgets_for_linux/ggadget/color.h"
#include "third_party/google_gadgets_for_linux/ggadget/common.h"
#include "third_party/google_gadgets_for_linux/ggadget/div_element.h"
#include "third_party/google_gadgets_for_linux/ggadget/elements.h"
#include "third_party/google_gadgets_for_linux/ggadget/linear_element.h"
#include "third_party/google_gadgets_for_linux/ggadget/logger.h"
#include "third_party/google_gadgets_for_linux/ggadget/menu_interface.h"
#include "third_party/google_gadgets_for_linux/ggadget/scriptable_event.h"
#include "third_party/google_gadgets_for_linux/ggadget/scriptable_menu.h"
#include "third_party/google_gadgets_for_linux/ggadget/signals.h"
#include "third_party/google_gadgets_for_linux/ggadget/small_object.h"
#include "third_party/google_gadgets_for_linux/ggadget/variant.h"
#include "third_party/google_gadgets_for_linux/ggadget/view.h"

namespace {

const char* kOrientationTypes[] = {
    "horizontal", "vertical"
};

}  // namespace

namespace ime_goopy {
namespace skin {

class CandidateListElement::Impl : public ggadget::SmallObject<> {
 public:
  // Candidate element style.
  struct Style {
    ggadget::TextFormat default_format;
    ggadget::Variant selected_color;
    double menu_width;
    double menu_height;
    ggadget::Variant menu_icon;
    ggadget::Variant menu_over_icon;
    ggadget::Variant menu_down_icon;
    ggadget::Variant selected_menu_icon;
    ggadget::Variant selected_menu_over_icon;
    ggadget::Variant selected_menu_down_icon;
  };

  Impl(ggadget::View* view, CandidateListElement* owner);
  ~Impl();

  // Implmentation of owner's CalculateSize method.
  void CalculateSize();

  // Implementation of owner's RemoveAllCandidates method.
  void RemoveAllCandidates();

  // Implementation of owner's AppendCandidate method.
  CandidateElement* AppendCandidate(uint32_t id, const std::string& text);
  CandidateElement* AppendCandidateWithFormat(
      uint32_t id,
      const std::string& text,
      const ggadget::TextFormats& formats);

  // Called when on of the candidate is selected.
  void OnCandidateSelected(uint32_t id, bool commit);

  // Called when on of the candidate needs a pop-out menu.
  void OnCandidateMenuEvent(
      uint32_t id, ggadget::MenuInterface* menu_interface);

  // Find candidate in all candidates with id.
  // Return value: pointer to CandidateElement.
  // NULL if not found.
  CandidateElement* FindCandidateElementById(uint32_t id);

  // Convienient method to set default candidate style.
  void SetDefaultUIStyle();

  // Calculate the max height of candidate element under current UI style.
  double CalculateCandidateMaxHeight();

  // Customize specified candidate's UI style.
  void UpdateCandidateUIStyle(
      CandidateElement* candidate_element, const Style& candidate_style);

  // Update all candidates' UI/layout according to given UI and layout
  // relative variables.
  void UpdateCandidatesStyle();

  // Update all candidates' layout style.
  void UpdateCandidatesLayout();

  // Layout selection image according to selected_candidate_.
  void LayoutSelectionImage();

  // Overridden/overwritten from BasicElement to support auto property.
  ggadget::Variant GetWidth() const;
  void SetWidth(const ggadget::Variant& width);
  ggadget::Variant GetHeight() const;
  void SetHeight(const ggadget::Variant& height);

  std::string GetSelectionImageMargin() const;
  void SetSelectionImageMargin(const std::string& margin);

  std::string GetSelectionImageStretchBorder() const;
  void SetSelectionImageStretchBorder(const std::string& border);

  CandidateListElement* owner_;
  ggadget::Connection* on_theme_changed_connection_;

  // Margin for selection image comparing to candidate element.
  double selection_left_margin_;
  double selection_top_margin_;
  double selection_right_margin_;
  double selection_bottom_margin_;

  // Padding between candidate elements.
  double horizontal_padding_;
  double vertical_padding_;

  // Style of candidate element.
  Style candidate_style_;

  // Orientation how candidate layout.
  CandidateListElement::Orientation orientation_;

  // Current selected candidate.
  CandidateElement* selected_candidate_;

  // UI elements.
  ggadget::LinearElement* linear_element_;
  ggadget::DivElement* selection_image_element_;

  // Signal for candidate selected event.
  // the first parameter is id of target candidate.
  // the second parameter indicates if it is an commit select.
  ggadget::Signal2<void, uint32_t, bool> on_candidate_selected_event_;

  // Signal for candidate add context menu event.
  // the first parameter is id of target candidate.
  // the second parameter is the MenuInterface to be added items.
  ggadget::Signal2<void, uint32_t, ggadget::MenuInterface*>
      on_candidate_context_menu_event_;

  bool is_selection_image_on_top_;
  bool vertical_layout_candidate_aligned_;
  bool h_auto_sizing_;
  bool v_auto_sizing_;
  double min_width_;
  double min_height_;
  std::map<int /*Id*/, ggadget::TextFormats> candidate_formats_;

 private:
  DISALLOW_EVIL_CONSTRUCTORS(Impl);
};

CandidateListElement::Impl::Impl(
    ggadget::View* view, CandidateListElement* owner)
    : owner_(owner),
      on_theme_changed_connection_(NULL),
      selection_left_margin_(0),
      selection_top_margin_(0),
      selection_right_margin_(0),
      selection_bottom_margin_(0),
      horizontal_padding_(0),
      vertical_padding_(0),
      orientation_(ORIENTATION_HORIZONTAL),
      selected_candidate_(NULL),
      linear_element_(NULL),
      selection_image_element_(NULL),
      is_selection_image_on_top_(false),
      vertical_layout_candidate_aligned_(false),
      h_auto_sizing_(true),
      v_auto_sizing_(true),
      min_width_(0),
      min_height_(0) {
  // Set default UI value.
  SetDefaultUIStyle();

  // Construct selection image element.
  selection_image_element_ = new ggadget::DivElement(view, "");
  selection_image_element_->SetBackground(
      ggadget::Variant(GetDefaultImageFilePath(kCandidateSelectionImage)));
  selection_image_element_->SetBackgroundMode(
      ggadget::DivElement::BACKGROUND_MODE_STRETCH);

  // Construct linear element.
  linear_element_ = new ggadget::LinearElement(view, "");
  linear_element_->SetHorizontalAutoSizing(true);
  linear_element_->SetVerticalAutoSizing(true);
  linear_element_->SetOrientation(
      orientation_ == ORIENTATION_VERTICAL ?
      ggadget::LinearElement::ORIENTATION_VERTICAL :
      ggadget::LinearElement::ORIENTATION_HORIZONTAL);
  linear_element_->SetTextDirection(
      ggadget::BasicElement::TEXT_DIRECTION_INHERIT_FROM_PARENT);
  owner_->GetChildren()->AppendElement(linear_element_);

  on_theme_changed_connection_ = view->ConnectOnThemeChangedEvent(
      NewSlot(this, &Impl::UpdateCandidatesStyle));
}

CandidateListElement::Impl::~Impl() {
  delete selection_image_element_;
  selection_image_element_ = NULL;
  if (on_theme_changed_connection_) {
    on_theme_changed_connection_->Disconnect();
    on_theme_changed_connection_ = NULL;
  }
}

void CandidateListElement::Impl::CalculateSize() {
  double children_width = linear_element_->GetPixelWidth();
  children_width += std::max(selection_left_margin_, 0.0);
  children_width += std::max(selection_right_margin_, 0.0);
  double children_height = linear_element_->GetPixelHeight();
  children_height += std::max(selection_top_margin_, 0.0);
  children_height += std::max(selection_bottom_margin_, 0.0);

  // Set owner extents to children size if width/height is not specified.
  if (h_auto_sizing_)
    owner_->SetPixelWidth(children_width);
  min_width_ = children_width;
  if (v_auto_sizing_)
    owner_->SetPixelHeight(children_height);
  min_height_ = children_height;
}

void CandidateListElement::Impl::RemoveAllCandidates() {
  linear_element_->GetChildren()->RemoveAllElements();
  selected_candidate_ = NULL;
}

CandidateElement* CandidateListElement::Impl::AppendCandidate(
    uint32_t id, const std::string& text) {
  CandidateElement* candidate_element =
      new CandidateElement(owner_->GetView(), "");
  // Set candidate id and text.
  candidate_element->SetId(id);
  candidate_element->SetText(text);

  // Set candidate position and size.
  UpdateCandidateUIStyle(candidate_element, candidate_style_);
  candidate_element->SetVerticalAutoSizing(false);
  if (orientation_ == ORIENTATION_VERTICAL &&
      vertical_layout_candidate_aligned_) {
    candidate_element->SetVerticalAutoSizing(true);
    candidate_element->SetHorizontalAutoSizing(false);
    candidate_element->SetRelativeWidth(1.0);
  } else {
    candidate_element->SetHorizontalAutoSizing(true);
  }

  linear_element_->GetChildren()->AppendElement(candidate_element);
  // Hook candidate events.
  candidate_element->ConnectOnCandidateContextMenu(
      ggadget::NewSlot(this, &Impl::OnCandidateMenuEvent));
  candidate_element->ConnectOnCandidateSelected(
      ggadget::NewSlot(this, &Impl::OnCandidateSelected));

  candidate_element->SetRelativeHeight(1.0);
  return candidate_element;
}

CandidateElement* CandidateListElement::Impl::AppendCandidateWithFormat(
      uint32_t id,
      const std::string& text,
      const ggadget::TextFormats& formats) {
  CandidateElement* candidate_element = AppendCandidate(id, text);
  candidate_formats_[id] = formats;
  return candidate_element;
}

void CandidateListElement::Impl::OnCandidateSelected(uint32_t id, bool commit) {
  on_candidate_selected_event_(id, commit);
}

void CandidateListElement::Impl::OnCandidateMenuEvent(
    uint32_t id, ggadget::MenuInterface* menu_interface) {
  on_candidate_selected_event_(id, false);
  on_candidate_context_menu_event_(id, menu_interface);
}

CandidateElement* CandidateListElement::Impl::FindCandidateElementById(
    uint32_t id) {
  const size_t candidate_size = linear_element_->GetChildren()->GetCount();
  for (size_t i = 0; i < candidate_size; ++i) {
    CandidateElement* candidate = static_cast<CandidateElement*>(
        linear_element_->GetChildren()->GetItemByIndex(i));
    if (candidate->GetId() == id)
      return candidate;
  }
  return NULL;
}

void CandidateListElement::Impl::SetDefaultUIStyle() {
  // Set default candidate style.
  candidate_style_.default_format.set_font("sans-serif");
  candidate_style_.default_format.set_size(8);
  ggadget::Color foreground;
  ggadget::Color::FromString("#000000", &foreground, NULL);
  candidate_style_.default_format.set_foreground(foreground);
  candidate_style_.selected_color = ggadget::Variant("#808080");
  candidate_style_.menu_width = 10;
  candidate_style_.menu_height = 10;
  candidate_style_.menu_icon =
      ggadget::Variant(GetDefaultImageFilePath(kCandidateMenuIcon));
  candidate_style_.menu_down_icon =
      ggadget::Variant(GetDefaultImageFilePath(kCandidateMenuDownIcon));
  candidate_style_.menu_over_icon =
      ggadget::Variant(GetDefaultImageFilePath(kCandidateMenuOverIcon));
  candidate_style_.selected_menu_icon =
      ggadget::Variant(GetDefaultImageFilePath(kSelectedCandidateMenuIcon));
  candidate_style_.selected_menu_down_icon =
      ggadget::Variant(GetDefaultImageFilePath(kSelectedCandidateMenuDownIcon));
  candidate_style_.selected_menu_over_icon =
      ggadget::Variant(GetDefaultImageFilePath(kSelectedCandidateMenuOverIcon));

  // Set default orientation.
  orientation_ = ORIENTATION_HORIZONTAL;

  // Set default selection style.
  is_selection_image_on_top_ = false;
}

double CandidateListElement::Impl::CalculateCandidateMaxHeight() {
  CandidateElement test_candidate(owner_->GetView(), "");
  UpdateCandidateUIStyle(&test_candidate, candidate_style_);

  test_candidate.SetText(kSampleTextForMeasurement);
  static_cast<ggadget::BasicElement*>(&test_candidate)->CalculateSize();
  return test_candidate.GetPixelHeight();
}

void CandidateListElement::Impl::UpdateCandidateUIStyle(
    CandidateElement* candidate_element, const Style& candidate_style) {
  candidate_element->SetMenuWidth(candidate_style.menu_width);
  candidate_element->SetMenuHeight(candidate_style.menu_height);

  if (candidate_element == selected_candidate_) {
    ggadget::TextFormat select_default_format = candidate_style.default_format;
    select_default_format.SetFormat(ggadget::TextFormat::kForegroundName,
                                    candidate_style.selected_color);
    candidate_element->SetDefaultFormat(select_default_format);
    ggadget::TextFormats selected_format =
        candidate_formats_[candidate_element->GetId()];
    for (size_t i = 0; i < selected_format.size(); ++i) {
      if (selected_format[i].format.has_foreground()) {
        selected_format[i].format.SetFormat(
            ggadget::TextFormat::kForegroundName,
            candidate_style.selected_color);
      }
    }
    candidate_element->SetFormats(selected_format);
    candidate_element->SetMenuIcon(candidate_style.selected_menu_icon);
    candidate_element->SetMenuOverIcon(candidate_style.selected_menu_over_icon);
    candidate_element->SetMenuDownIcon(candidate_style.selected_menu_down_icon);
  } else {
    candidate_element->SetDefaultFormat(candidate_style.default_format);
    candidate_element->SetFormats(
        candidate_formats_[candidate_element->GetId()]);
    candidate_element->SetMenuIcon(candidate_style.menu_icon);
    candidate_element->SetMenuOverIcon(candidate_style.menu_over_icon);
    candidate_element->SetMenuDownIcon(candidate_style.menu_down_icon);
  }
}

void CandidateListElement::Impl::UpdateCandidatesStyle() {
  const size_t candidate_size = linear_element_->GetChildren()->GetCount();
  for (size_t i = 0; i < candidate_size; ++i) {
    CandidateElement* candidate_element =
        static_cast<CandidateElement*>(
            linear_element_->GetChildren()->GetItemByIndex(i));
    UpdateCandidateUIStyle(candidate_element, candidate_style_);
  }
}

void CandidateListElement::Impl::UpdateCandidatesLayout() {
  // Updates the layout of the candidates so that in vertical layout:
  // 1. when vertical_layout_candidate_aligned_ == true, the candidate element
  //    should fill the width of candidate list element.
  // 2. when rtl == true, the candidates will align to the right.
  const size_t children_count = linear_element_->GetChildren()->GetCount();
  bool rtl = owner_->IsTextRTL();
  for (size_t i = 0; i < children_count; ++i) {
    CandidateElement* candidate =
        static_cast<CandidateElement*>(
            linear_element_->GetChildren()->GetItemByIndex(i));
    if (orientation_ == ORIENTATION_VERTICAL &&
        vertical_layout_candidate_aligned_) {
      candidate->SetRelativeWidth(1.0);
      candidate->SetHorizontalAutoSizing(false);
    } else {
      candidate->SetHorizontalAutoSizing(true);
    }
    if (orientation_ == ORIENTATION_VERTICAL && rtl) {
      // Aligned to the right.
      candidate->SetRelativePinX(1.0);
      candidate->SetRelativeX(1.0);
    }
  }
}

void CandidateListElement::Impl::LayoutSelectionImage() {
  // Layout selection image.
  if (selected_candidate_) {
    double candidate_x_in_linear, candidate_y_in_linear;
    double candidate_x_in_list, candidate_y_in_list;
    selected_candidate_->SelfCoordToParentCoord(
        0, 0, &candidate_x_in_linear, &candidate_y_in_linear);
    linear_element_->SelfCoordToParentCoord(
        candidate_x_in_linear, candidate_y_in_linear,
        &candidate_x_in_list, &candidate_y_in_list);

    const ggadget::Rectangle candidate_rect(
        candidate_x_in_list,
        candidate_y_in_list,
        selected_candidate_->GetPixelWidth(),
        selected_candidate_->GetPixelHeight());
    const ggadget::Rectangle selection_image_rect(
        candidate_rect.x - selection_left_margin_,
        candidate_rect.y - selection_top_margin_,
        candidate_rect.w + selection_left_margin_ + selection_right_margin_,
        candidate_rect.h + selection_top_margin_ + selection_bottom_margin_);

    if (!selection_image_rect.Overlaps(candidate_rect) ||
        selection_image_rect.w <= 0 || selection_image_rect.h <= 0) {
      selection_image_element_->SetPixelX(candidate_rect.x);
      selection_image_element_->SetPixelY(candidate_rect.y);
      selection_image_element_->SetPixelWidth(candidate_rect.w);
      selection_image_element_->SetPixelHeight(candidate_rect.h);
    } else {
      selection_image_element_->SetPixelX(selection_image_rect.x);
      selection_image_element_->SetPixelY(selection_image_rect.y);
      selection_image_element_->SetPixelWidth(selection_image_rect.w);
      selection_image_element_->SetPixelHeight(selection_image_rect.h);
    }
  }
}

ggadget::Variant CandidateListElement::Impl::GetWidth() const {
  return h_auto_sizing_ ? ggadget::Variant("auto") : owner_->GetWidth();
}

void CandidateListElement::Impl::SetWidth(const ggadget::Variant& width) {
  if (width.type() == ggadget::Variant::TYPE_STRING &&
      ggadget::VariantValue<std::string>()(width) == "auto") {
    owner_->SetHorizontalAutoSizing(true);
  } else {
    h_auto_sizing_ = false;
    owner_->SetWidth(width);
  }
}

ggadget::Variant CandidateListElement::Impl::GetHeight() const {
  return v_auto_sizing_ ? ggadget::Variant("auto") : owner_->GetHeight();
}

void CandidateListElement::Impl::SetHeight(const ggadget::Variant& height) {
  if (height.type() == ggadget::Variant::TYPE_STRING &&
      ggadget::VariantValue<std::string>()(height) == "auto") {
    owner_->SetVerticalAutoSizing(true);
  } else {
    v_auto_sizing_ = false;
    owner_->SetHeight(height);
  }
}

std::string CandidateListElement::Impl::GetSelectionImageMargin() const {
  return ggadget::StringPrintf("%.0f,%.0f,%.0f,%.0f",
                               selection_left_margin_,
                               selection_top_margin_,
                               selection_right_margin_,
                               selection_bottom_margin_);
}

void CandidateListElement::Impl::SetSelectionImageMargin(
    const std::string& margin) {
  double left, top, right, bottom;
  if (!ggadget::StringToBorderSize(margin, &left, &top, &right, &bottom)) {
    owner_->SetSelectionImageMargin(0, 0, 0, 0);
  } else {
    owner_->SetSelectionImageMargin(left, top, right, bottom);
  }
}

std::string CandidateListElement::Impl::GetSelectionImageStretchBorder() const {
  double left, top, right, bottom;
  owner_->GetSelectionImageStretchBorder(&left, &top, &right, &bottom);
  return ggadget::StringPrintf("%.0f,%.0f,%.0f,%.0f",
                               left, top, right, bottom);
}

void CandidateListElement::Impl::SetSelectionImageStretchBorder(
    const std::string& border) {
  double left, top, right, bottom;
  if (!ggadget::StringToBorderSize(border,
                                   &left, &top,
                                   &right, &bottom)) {
    owner_->SetSelectionImageStretchBorder(0, 0, 0, 0);
  } else {
    owner_->SetSelectionImageStretchBorder(left, top, right, bottom);
  }
}

CandidateListElement::CandidateListElement(
    ggadget::View* view, const char* name)
    : ggadget::BasicElement(view, "candidatelist", name, true),
      impl_(new Impl(view, this)) {
}

CandidateListElement::~CandidateListElement() {
  delete impl_;
  impl_ = NULL;
}

CandidateListElement::Orientation CandidateListElement::GetOrientation() const {
  return impl_->orientation_;
}

void CandidateListElement::SetOrientation(
    CandidateListElement::Orientation orientation) {
  if (impl_->orientation_ != orientation) {
    impl_->orientation_ = orientation;
    impl_->linear_element_->SetOrientation(
        (orientation == ORIENTATION_VERTICAL) ?
        ggadget::LinearElement::ORIENTATION_VERTICAL :
        ggadget::LinearElement::ORIENTATION_HORIZONTAL);
    impl_->linear_element_->SetPadding(
        orientation == ORIENTATION_HORIZONTAL ?
        impl_->horizontal_padding_ : impl_->vertical_padding_);
    impl_->UpdateCandidatesLayout();
    QueueDraw();
  }
}

ggadget::Variant CandidateListElement::GetCandidateColor() const {
  return ggadget::Variant(
      impl_->candidate_style_.default_format.foreground().ToString());
}

void CandidateListElement::SetCandidateColor(const ggadget::Variant& color) {
  if (color.type() == ggadget::Variant::TYPE_STRING) {
    if (GetCandidateColor() != color) {
      impl_->candidate_style_.default_format.SetFormat(
          ggadget::TextFormat::kForegroundName,
          color);
      impl_->UpdateCandidatesStyle();
    }
  }
}

ggadget::Variant CandidateListElement::GetSelectedCandidateColor() const {
  return impl_->candidate_style_.selected_color;
}

void CandidateListElement::SetSelectedCandidateColor(
    const ggadget::Variant& color) {
  if (color.type() == ggadget::Variant::TYPE_STRING) {
    if (impl_->candidate_style_.selected_color != color) {
      impl_->candidate_style_.selected_color = color;
      impl_->UpdateCandidatesStyle();
    }
  }
}

std::string CandidateListElement::GetCandidateFont() const {
  return impl_->candidate_style_.default_format.font();
}

void CandidateListElement::SetCandidateFont(const char* font) {
  if (impl_->candidate_style_.default_format.font()!= font) {
    impl_->candidate_style_.default_format.set_font(font);
    impl_->UpdateCandidatesStyle();
  }
}

double CandidateListElement::GetCandidateSize() const {
  return impl_->candidate_style_.default_format.size();
}

void CandidateListElement::SetCandidateSize(double size) {
  if (GetCandidateSize() != size) {
    impl_->candidate_style_.default_format.set_size(size);
    impl_->UpdateCandidatesStyle();
  }
}

bool CandidateListElement::IsCandidateBold() const {
  return impl_->candidate_style_.default_format.bold();
}

void CandidateListElement::SetCandidateBold(bool bold) {
  if (IsCandidateBold() != bold) {
    impl_->candidate_style_.default_format.set_bold(bold);
    impl_->UpdateCandidatesStyle();
  }
}

bool CandidateListElement::IsCandidateItalic() const {
  return impl_->candidate_style_.default_format.italic();
}

void CandidateListElement::SetCandidateItalic(bool italic) {
  if (IsCandidateItalic() != italic) {
    impl_->candidate_style_.default_format.set_italic(italic);
    impl_->UpdateCandidatesStyle();
  }
}

bool CandidateListElement::IsCandidateStrikeout() const {
  return impl_->candidate_style_.default_format.strikeout();
}

void CandidateListElement::SetCandidateStrikeout(bool strikeout) {
  if (IsCandidateStrikeout() != strikeout) {
    impl_->candidate_style_.default_format.set_strikeout(strikeout);
    impl_->UpdateCandidatesStyle();
  }
}

bool CandidateListElement::IsCandidateUnderline() const {
  return impl_->candidate_style_.default_format.underline();
}

void CandidateListElement::SetCandidateUnderline(bool underline) {
  if (IsCandidateUnderline() != underline) {
    impl_->candidate_style_.default_format.set_underline(underline);
    impl_->UpdateCandidatesStyle();
  }
}

double CandidateListElement::GetCandidateMenuWidth() const {
  return impl_->candidate_style_.menu_width;
}

void CandidateListElement::SetCandidateMenuWidth(double width) {
  if (impl_->candidate_style_.menu_width != width) {
    impl_->candidate_style_.menu_width = width;
    impl_->UpdateCandidatesStyle();
  }
}

double CandidateListElement::GetCandidateMenuHeight() const {
  return impl_->candidate_style_.menu_height;
}

void CandidateListElement::SetCandidateMenuHeight(double height) {
  if (impl_->candidate_style_.menu_height != height) {
    impl_->candidate_style_.menu_height = height;
    impl_->UpdateCandidatesStyle();
  }
}

ggadget::Variant CandidateListElement::GetCandidateMenuIcon() const {
  return impl_->candidate_style_.menu_icon;
}

void CandidateListElement::SetCandidateMenuIcon(
    const ggadget::Variant& img) {
  if (impl_->candidate_style_.menu_icon != img) {
    impl_->candidate_style_.menu_icon = img;
    impl_->UpdateCandidatesStyle();
  }
}

ggadget::Variant CandidateListElement::GetCandidateMenuDownIcon() const {
  return impl_->candidate_style_.menu_down_icon;
}

void CandidateListElement::SetCandidateMenuDownIcon(
    const ggadget::Variant& img) {
  if (impl_->candidate_style_.menu_down_icon != img) {
    impl_->candidate_style_.menu_down_icon = img;
    impl_->UpdateCandidatesStyle();
  }
}

ggadget::Variant CandidateListElement::GetCandidateMenuOverIcon() const {
  return impl_->candidate_style_.menu_over_icon;
}

void CandidateListElement::SetCandidateMenuOverIcon(
    const ggadget::Variant& img) {
  if (impl_->candidate_style_.menu_over_icon != img) {
    impl_->candidate_style_.menu_over_icon = img;
    impl_->UpdateCandidatesStyle();
  }
}

ggadget::Variant CandidateListElement::GetSelectedCandidateMenuIcon() const {
  return impl_->candidate_style_.selected_menu_icon;
}

void CandidateListElement::SetSelectedCandidateMenuIcon(
    const ggadget::Variant& img) {
  if (impl_->candidate_style_.selected_menu_icon != img) {
    impl_->candidate_style_.selected_menu_icon = img;
    impl_->UpdateCandidatesStyle();
  }
}

ggadget::Variant
CandidateListElement::GetSelectedCandidateMenuDownIcon() const {
  return impl_->candidate_style_.selected_menu_down_icon;
}

void CandidateListElement::SetSelectedCandidateMenuDownIcon(
    const ggadget::Variant& img) {
  if (impl_->candidate_style_.selected_menu_down_icon != img) {
    impl_->candidate_style_.selected_menu_down_icon = img;
    impl_->UpdateCandidatesStyle();
  }
}

ggadget::Variant
CandidateListElement::GetSelectedCandidateMenuOverIcon() const {
  return impl_->candidate_style_.selected_menu_over_icon;
}

void CandidateListElement::SetSelectedCandidateMenuOverIcon(
    const ggadget::Variant& img) {
  if (impl_->candidate_style_.selected_menu_over_icon != img) {
    impl_->candidate_style_.selected_menu_over_icon = img;
    impl_->UpdateCandidatesStyle();
  }
}

ggadget::Variant CandidateListElement::GetSelectionImage() const {
  return impl_->selection_image_element_->GetBackground();
}

void CandidateListElement::SetSelectionImage(const ggadget::Variant& img) {
  impl_->selection_image_element_->SetBackground(img);
  QueueDraw();
}

bool CandidateListElement::IsSelectionImageOnTop() const {
  return impl_->is_selection_image_on_top_;
}

void CandidateListElement::SetSelectionImageOnTop(bool ontop) {
  if (impl_->is_selection_image_on_top_ != ontop) {
    impl_->is_selection_image_on_top_ = ontop;
    QueueDraw();
  }
}

void CandidateListElement::GetSelectionImageMargin(
    double* left, double* top, double* right, double* bottom) const {
  *left = impl_->selection_left_margin_;
  *top = impl_->selection_top_margin_;
  *right = impl_->selection_right_margin_;
  *bottom = impl_->selection_bottom_margin_;
}

void CandidateListElement::SetSelectionImageMargin(
    double left, double top, double right, double bottom) {
  if (left != impl_->selection_left_margin_ ||
      top != impl_->selection_top_margin_ ||
      right != impl_->selection_right_margin_ ||
      bottom != impl_->selection_bottom_margin_) {
    impl_->selection_left_margin_ = left;
    impl_->selection_top_margin_ = top;
    impl_->selection_right_margin_ = right;
    impl_->selection_bottom_margin_ = bottom;

    impl_->linear_element_->SetPixelX(
        std::max(impl_->selection_left_margin_, 0.0));
    impl_->linear_element_->SetPixelY(
        std::max(impl_->selection_top_margin_, 0.0));
    QueueDraw();
  }
}

void CandidateListElement::GetSelectionImageStretchBorder(
    double* left, double* top, double* right, double* bottom) const {
  impl_->selection_image_element_->GetBackgroundBorder(
      left, top, right, bottom);
}

void CandidateListElement::SetSelectionImageStretchBorder(
  double left, double top, double right, double bottom) {
  impl_->selection_image_element_->SetBackgroundBorder(
      left, top, right, bottom);
}

uint32_t CandidateListElement::GetSelectedCandidate() const {
  return impl_->selected_candidate_->GetId();
}

void CandidateListElement::SetSelectedCandidate(uint32_t id) {
  if (impl_->selected_candidate_ && impl_->selected_candidate_->GetId() == id)
    return;
  CandidateElement* candidate = impl_->FindCandidateElementById(id);
  if (!candidate)
    return;
  impl_->selected_candidate_ = candidate;
  impl_->UpdateCandidatesStyle();
  QueueDraw();
}

bool CandidateListElement::IsHorizontalAutoSizing() const {
  return impl_->h_auto_sizing_;
}

void CandidateListElement::SetHorizontalAutoSizing(bool auto_sizing) {
  if (impl_->h_auto_sizing_ != auto_sizing) {
    impl_->h_auto_sizing_ = auto_sizing;
    QueueDraw();
  }
}

bool CandidateListElement::IsVerticalAutoSizing() const {
  return impl_->v_auto_sizing_;
}

void CandidateListElement::SetVerticalAutoSizing(bool auto_sizing) {
  if (impl_->v_auto_sizing_ != auto_sizing) {
    impl_->v_auto_sizing_ = auto_sizing;
    QueueDraw();
  }
}

bool CandidateListElement::IsVerticalLayoutCandidateAligned() const {
  return impl_->vertical_layout_candidate_aligned_;
}

void CandidateListElement::SetVerticalLayoutCandidateAligned(bool aligned) {
  if (impl_->vertical_layout_candidate_aligned_ != aligned) {
    impl_->vertical_layout_candidate_aligned_ = aligned;
    impl_->UpdateCandidatesLayout();
    QueueDraw();
  }
}

double CandidateListElement::GetHorizontalPadding() const {
  return impl_->horizontal_padding_;
}

void CandidateListElement::SetHorizontalPadding(
    double padding) {
  if (impl_->horizontal_padding_ != padding) {
    impl_->horizontal_padding_ = padding;
    if (impl_->orientation_ == ORIENTATION_HORIZONTAL)
      impl_->linear_element_->SetPadding(impl_->horizontal_padding_);
    QueueDraw();
  }
}

double CandidateListElement::GetVerticalPadding() const {
  return impl_->vertical_padding_;
}

void CandidateListElement::SetVerticalPadding(double padding) {
  if (impl_->vertical_padding_ != padding) {
    impl_->vertical_padding_ = padding;
    if (impl_->orientation_ == ORIENTATION_VERTICAL)
      impl_->linear_element_->SetPadding(impl_->vertical_padding_);
    QueueDraw();
  }
}

CandidateElement* CandidateListElement::AppendCandidate(
    uint32_t id, const std::string& text) {
  return impl_->AppendCandidate(id, text);
}

CandidateElement* CandidateListElement::AppendCandidateWithFormat(
    uint32_t id,
    const std::string& text,
    const ggadget::TextFormats& formats) {
  return impl_->AppendCandidateWithFormat(id, text, formats);
}

void CandidateListElement::RemoveAllCandidates() {
  impl_->RemoveAllCandidates();
}

ggadget::Connection* CandidateListElement::ConnectOnCandidateSelected(
    ggadget::Slot2<void, uint32_t, bool>* handler) {
  return impl_->on_candidate_selected_event_.Connect(handler);
}

ggadget::Connection* CandidateListElement::ConnectOnShowCandidateContextMenu(
    ggadget::Slot2<void, uint32_t, ggadget::MenuInterface*>* handler) {
  return impl_->on_candidate_context_menu_event_.Connect(handler);
}

ggadget::BasicElement* CandidateListElement::CreateInstance(
    ggadget::View* view, const char* name) {
  return new CandidateListElement(view, name);
}

void CandidateListElement::DoClassRegister() {
  ggadget::BasicElement::DoClassRegister();
  // Register style properties.
  RegisterProperty(
      "candidateFont",
      ggadget::NewSlot(&CandidateListElement::GetCandidateFont),
      ggadget::NewSlot(&CandidateListElement::SetCandidateFont));
  RegisterProperty(
      "candidateSize",
      ggadget::NewSlot(&CandidateListElement::GetCandidateSize),
      ggadget::NewSlot(&CandidateListElement::SetCandidateSize));
  RegisterProperty(
      "candidateColor",
      ggadget::NewSlot(&CandidateListElement::GetCandidateColor),
      ggadget::NewSlot(&CandidateListElement::SetCandidateColor));
  RegisterProperty(
      "selectedCandidateColor",
      ggadget::NewSlot(&CandidateListElement::GetSelectedCandidateColor),
      ggadget::NewSlot(&CandidateListElement::SetSelectedCandidateColor));
  RegisterProperty(
      "candidateBold",
      ggadget::NewSlot(&CandidateListElement::IsCandidateBold),
      ggadget::NewSlot(&CandidateListElement::SetCandidateBold));
  RegisterProperty(
      "candidateItalic",
      ggadget::NewSlot(&CandidateListElement::IsCandidateItalic),
      ggadget::NewSlot(&CandidateListElement::SetCandidateItalic));
  RegisterProperty(
      "candidateUnderline",
      ggadget::NewSlot(&CandidateListElement::IsCandidateUnderline),
      ggadget::NewSlot(&CandidateListElement::SetCandidateUnderline));
  RegisterProperty(
      "candidateStrikeOut",
      ggadget::NewSlot(&CandidateListElement::IsCandidateStrikeout),
      ggadget::NewSlot(&CandidateListElement::SetCandidateStrikeout));
  RegisterProperty(
      "candidateMenuWidth",
      ggadget::NewSlot(&CandidateListElement::GetCandidateMenuWidth),
      ggadget::NewSlot(&CandidateListElement::SetCandidateMenuWidth));
  RegisterProperty(
      "candidateMenuHeight",
      ggadget::NewSlot(&CandidateListElement::GetCandidateMenuHeight),
      ggadget::NewSlot(&CandidateListElement::SetCandidateMenuHeight));
  RegisterProperty(
      "candidateMenuIcon",
      ggadget::NewSlot(&CandidateListElement::GetCandidateMenuIcon),
      ggadget::NewSlot(&CandidateListElement::SetCandidateMenuIcon));
  RegisterProperty(
      "candidateMenuDownIcon",
      ggadget::NewSlot(&CandidateListElement::GetCandidateMenuDownIcon),
      ggadget::NewSlot(&CandidateListElement::SetCandidateMenuDownIcon));
  RegisterProperty(
      "candidateMenuOverIcon",
      ggadget::NewSlot(&CandidateListElement::GetCandidateMenuOverIcon),
      ggadget::NewSlot(&CandidateListElement::SetCandidateMenuOverIcon));
  RegisterProperty(
      "selectedCandidateMenuIcon",
      ggadget::NewSlot(&CandidateListElement::GetSelectedCandidateMenuIcon),
      ggadget::NewSlot(&CandidateListElement::SetSelectedCandidateMenuIcon));
  RegisterProperty(
      "selectedCandidateMenuDownIcon",
      ggadget::NewSlot(
          &CandidateListElement::GetSelectedCandidateMenuDownIcon),
      ggadget::NewSlot(
          &CandidateListElement::SetSelectedCandidateMenuDownIcon));
  RegisterProperty(
      "selectedCandidateMenuOverIcon",
      ggadget::NewSlot(
          &CandidateListElement::GetSelectedCandidateMenuOverIcon),
      ggadget::NewSlot(
          &CandidateListElement::SetSelectedCandidateMenuOverIcon));

  // Register candidate list UI.
  RegisterProperty(
      "selectionImage",
      ggadget::NewSlot(&CandidateListElement::GetSelectionImage),
      ggadget::NewSlot(&CandidateListElement::SetSelectionImage));
  RegisterProperty(
      "selectionImageOnTop",
      ggadget::NewSlot(&CandidateListElement::IsSelectionImageOnTop),
      ggadget::NewSlot(&CandidateListElement::SetSelectionImageOnTop));
  RegisterProperty(
      "selectionImageMargin",
      ggadget::NewSlot(&Impl::GetSelectionImageMargin,
                       &CandidateListElement::impl_),
      ggadget::NewSlot(&Impl::SetSelectionImageMargin,
                       &CandidateListElement::impl_));
  RegisterProperty(
      "selectionImageStretchBorder",
      ggadget::NewSlot(&Impl::GetSelectionImageStretchBorder,
                       &CandidateListElement::impl_),
      ggadget::NewSlot(&Impl::SetSelectionImageStretchBorder,
                       &CandidateListElement::impl_));

  RegisterStringEnumProperty(
      "orientation",
      ggadget::NewSlot(&CandidateListElement::GetOrientation),
      ggadget::NewSlot(&CandidateListElement::SetOrientation),
      kOrientationTypes, arraysize(kOrientationTypes));

  RegisterMethod(
      "appendCandidate",
      ggadget::NewSlot(&CandidateListElement::AppendCandidate));
  RegisterMethod(
      "removeAllCandidates",
      ggadget::NewSlot(&CandidateListElement::RemoveAllCandidates));

  // Property for setting selected index by script.
  RegisterProperty(
      "selectedCandidate",
      ggadget::NewSlot(&CandidateListElement::GetSelectedCandidate),
      ggadget::NewSlot(&CandidateListElement::SetSelectedCandidate));

  // Overrides "width" and "height" properties to make sure they won't be
  // changed by script when "autoSizing" is true.
  // Note: we cannot override BasicElement::{Get|Set}{Width|Height} methods, as
  // they are not virtual.
  RegisterProperty(
      "width",
      ggadget::NewSlot(&Impl::GetWidth, &CandidateListElement::impl_),
      ggadget::NewSlot(&Impl::SetWidth, &CandidateListElement::impl_));
  RegisterProperty(
      "height",
      ggadget::NewSlot(&Impl::GetHeight, &CandidateListElement::impl_),
      ggadget::NewSlot(&Impl::SetHeight, &CandidateListElement::impl_));

  RegisterProperty(
      "verticalLayoutCandidateAligned",
      ggadget::NewSlot(
          &CandidateListElement::IsVerticalLayoutCandidateAligned),
      ggadget::NewSlot(
          &CandidateListElement::SetVerticalLayoutCandidateAligned));

  RegisterProperty(
      "horizontalPadding",
      ggadget::NewSlot(&CandidateListElement::GetHorizontalPadding),
      ggadget::NewSlot(&CandidateListElement::SetHorizontalPadding));
  RegisterProperty(
      "verticalPadding",
      ggadget::NewSlot(&CandidateListElement::GetVerticalPadding),
      ggadget::NewSlot(&CandidateListElement::SetVerticalPadding));
}

void CandidateListElement::CalculateSize() {
  BasicElement::CalculateSize();
  impl_->CalculateSize();
}

void CandidateListElement::BeforeChildrenLayout() {
  impl_->UpdateCandidatesLayout();
  double margin_width = std::max(impl_->selection_left_margin_, 0.0) +
                        std::max(impl_->selection_right_margin_, 0.0);
  double margin_height = std::max(impl_->selection_top_margin_, 0.0) +
                         std::max(impl_->selection_bottom_margin_, 0.0);
  // If size is not auto, update the linear size with respect to candidate list
  // element size.
  if (!impl_->h_auto_sizing_)
    impl_->linear_element_->SetPixelWidth(GetPixelWidth() - margin_width);
  if (!impl_->v_auto_sizing_)
    impl_->linear_element_->SetPixelHeight(GetPixelHeight() - margin_height);
}

void CandidateListElement::Layout() {
  impl_->LayoutSelectionImage();
}

void CandidateListElement::DoDraw(ggadget::CanvasInterface* canvas) {
  // Draw down selection image if should be.
  if (impl_->linear_element_->GetChildren()->GetCount() != 0 &&
      impl_->selected_candidate_ && !impl_->is_selection_image_on_top_) {
    // Draw selection image with border.
    canvas->PushState();
    canvas->TranslateCoordinates(impl_->selection_image_element_->GetPixelX(),
                                 impl_->selection_image_element_->GetPixelY());
    impl_->selection_image_element_->Draw(canvas);
    canvas->PopState();
  }

  // Draw candidates.
  DrawChildren(canvas);

  // Draw up selection image if should be.
  if (impl_->linear_element_->GetChildren()->GetCount() != 0 &&
      impl_->selected_candidate_ && impl_->is_selection_image_on_top_) {
    // Draw selection image with border.
    canvas->PushState();
    canvas->TranslateCoordinates(impl_->selection_image_element_->GetPixelX(),
                                 impl_->selection_image_element_->GetPixelY());
    impl_->selection_image_element_->Draw(canvas);
    canvas->PopState();
  }
}

double CandidateListElement::GetMinWidth() const {
  return std::max(impl_->min_width_, BasicElement::GetMinWidth());
}

double CandidateListElement::GetMinHeight() const {
  return std::max(impl_->min_height_, BasicElement::GetMinHeight());
}

}  // namespace skin
}  // namespace ime_goopy
