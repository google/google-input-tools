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

#include "components/ui/skin_ui_component.h"

#include "base/logging.h"
#include "common/app_const.h"
#include "common/google_search_utils.h"
#include "components/common/constants.h"
#include "components/common/file_utils.h"
#include "components/ui/composing_window_position.h"
#include "components/ui/cursor_trapper.h"
#include "components/ui/skin_ui_component_utils.h"
#include "components/ui/ui_types.h"
#include "locale/locale_utils.h"

#pragma push_macro("LOG")
#pragma push_macro("DLOG")
#include "skin/candidate_list_element.h"
#include "skin/composition_element.h"
#include "skin/skin.h"
#include "skin/skin_consts.h"
#include "skin/skin_host_win.h"
#include "skin/skin_library_initializer.h"
#include "third_party/google_gadgets_for_linux/ggadget/button_element.h"
#include "third_party/google_gadgets_for_linux/ggadget/math_utils.h"
#include "third_party/google_gadgets_for_linux/ggadget/menu_interface.h"
#include "third_party/google_gadgets_for_linux/ggadget/label_element.h"
#include "third_party/google_gadgets_for_linux/ggadget/scriptable_binary_data.h"
#include "third_party/google_gadgets_for_linux/ggadget/slot.h"
#include "third_party/google_gadgets_for_linux/ggadget/string_utils.h"
#include "third_party/google_gadgets_for_linux/ggadget/text_formats.h"
#include "third_party/google_gadgets_for_linux/ggadget/variant.h"
#include "third_party/google_gadgets_for_linux/ggadget/view.h"
#include "third_party/google_gadgets_for_linux/ggadget/view_host_interface.h"
#pragma pop_macro("DLOG")
#pragma pop_macro("LOG")

namespace ime_goopy {
namespace components {
namespace {

static const char kSettingsVerticalCandidateView[] =
    "SkinUI/VerticalCandidateList";
static const char kSettingsTrackCaret[] = "SkinUI/TrackCaret";
static const char kSettingsComposingViewX[] = "SkinUI/ComposingViewX";
static const char kSettingsComposingViewY[] = "SkinUI/ComposingViewY";
static const char kSettingsRecentInputMethods[] = "SkinUI/RecentInputMethods";
static const char kSettingsSemiTransparencyToolbar[] =
    "SkinUI/SemiTransparencyToolbar";
static const char kSettingsIsToolbarMini[] = "SkinUI/MiniToolbar";
static const char kSettingsIsToolbarCollapse[] = "SkinUI/ToolbarCollapsed";
static const char kSettingsIsToolbarFloating[] = "SkinUI/FloatingToolbar";
static const char kSettingsIsFullScreenAppNoFloatingToolbar[] =
    "SkinUI/NoFloatingToolbarInFullscreenApp";
static const char kSettingsFirstRunShowed[] = "SkinUI/FirstRunShowed";

static const char kComponentName[] = "Goopy Skin UI Component";
static const char kComponentStringID[] = "com.google.input_tools.skinui";

static const char kSegmentLabels[] = "0123456789";

static const char kResourcePackPathPattern[] = "/ui_component_[LANG].pak";

ggadget::Color IPCColorToGgagdgetColor(const ipc::proto::Color& color) {
  return ggadget::Color(color.red(), color.green(), color.blue());
}

void TextAttributeToTextFormats(const ipc::proto::Text& text,
                                ggadget::TextFormats* formats) {
  formats->clear();
  for (int i = 0; i < text.attribute_size(); ++i) {
    ggadget::TextFormatRange format;
    switch (text.attribute(i).type()) {
      case ipc::proto::Attribute::FONT_FAMILY:
        format.format.set_font(text.attribute(i).string_value());
        break;
      case ipc::proto::Attribute::FONT_SIZE:
        format.format.set_size(text.attribute(i).float_value());
        break;
      case ipc::proto::Attribute::FONT_SCALE:
        format.format.set_scale(text.attribute(i).float_value());
        break;
      case ipc::proto::Attribute::FONT_WEIGHT:
        format.format.set_bold(text.attribute(i).font_weight() ==
                               ipc::proto::Attribute::FWT_BOLD);
        break;
      case ipc::proto::Attribute::FONT_STYLE:
        format.format.set_italic(text.attribute(i).font_style() ==
                                 ipc::proto::Attribute::FS_ITALIC);
        break;
      case ipc::proto::Attribute::UNDERLINE:
        if (text.attribute(i).underline_style() !=
            ipc::proto::Attribute::US_NONE) {
          format.format.set_italic(true);
          if (text.attribute(i).has_color_value()) {
            format.format.set_underline_color(
                IPCColorToGgagdgetColor(text.attribute(i).color_value()));
          }
        }
        break;
      case ipc::proto::Attribute::STRIKETHROUGH:
        format.format.set_strikeout(true);
        if (text.attribute(i).has_color_value()) {
          format.format.set_strikeout_color(
              IPCColorToGgagdgetColor(text.attribute(i).color_value()));
        }
        break;
      case ipc::proto::Attribute::TEXT_DIRECTION:
        format.format.set_text_rtl(
            text.attribute(i).text_direction() ==
            ipc::proto::RTL);
        break;
      case ipc::proto::Attribute::FOREGROUND:
        format.format.set_foreground(
            IPCColorToGgagdgetColor(text.attribute(i).color_value()));
        break;
      default:
        continue;
    }
    format.range.start = text.attribute(i).start();
    format.range.end = text.attribute(i).end();
    formats->push_back(format);
  }
}

void AddCandidateIndex(const std::string& keytext,
                       std::string* text,
                       ggadget::TextFormats* formats) {
  ggadget::UTF16String keytext_utf16;
  ggadget::ConvertStringUTF8ToUTF16(keytext, &keytext_utf16);
  int keytext_length = keytext_utf16.length();
  for (size_t i = 0; i < formats->size(); ++i) {
    formats->at(i).range.start += keytext_length;
    formats->at(i).range.end += keytext_length;
  }
  *text = keytext + *text;
}

}  // namespace


SkinUIComponent::SkinUIComponent()
  : cursor_trapper_(new CursorTrapper),
    composing_view_position_(new ComposingWindowPosition),
    should_show_composition_(false),
    should_show_candidate_list_(false),
    should_show_toolbar_(false),
    is_candidate_list_shown_(false),
    vertical_candidate_list_(true),
    track_caret_(true),
    floating_toolbar_(true),
    mini_toolbar_(false),
    semi_tranparent_toolbar_(false),
    toolbar_collapsed_(false),
    composing_view_x_(0),
    composing_view_y_(0),
    // Set the initial position of toolbar over the bottom right corner of
    // screen but within the range of int32.
    toolbar_x_(kint16max),
    toolbar_y_(kint16max) {
  settings_ = new ipc::SettingsClient(this, this);
}

SkinUIComponent::~SkinUIComponent() {
}

std::string SkinUIComponent::GetComponentName() {
  return kComponentName;
}

std::string SkinUIComponent::GetComponentStringID() {
  return kComponentStringID;
}

ime_goopy::skin::CandidateListElement*
    SkinUIComponent::GetCandidateListElement() {
  if (!skin_.get())
    return NULL;
  return skin_->GetElementByNameAndType<ime_goopy::skin::CandidateListElement>(
      skin::Skin::COMPOSING_VIEW, ime_goopy::skin::kCandidateListElement);
}

ime_goopy::skin::CompositionElement* SkinUIComponent::GetCompositionElement() {
  if (!skin_.get())
    return NULL;
  return skin_->GetElementByNameAndType<ime_goopy::skin::CompositionElement>(
      skin::Skin::COMPOSING_VIEW, ime_goopy::skin::kCompositionElement);
}

void SkinUIComponent::SetComposition(
    const ipc::proto::Composition* composition) {
  DCHECK(skin_.get());
  if (!skin_.get()) return;
  ime_goopy::skin::CompositionElement* composition_element =
      GetCompositionElement();
  if (!composition_element) return;
  composition_element->Clear();
  if (!composition ||
      !composition->has_text() ||
      composition->text().text().empty()) {
    UpdateComposingWindowVisibility();
    composition_.Clear();
    return;
  }
  if (composition != &composition_)
    composition_.CopyFrom(*composition);
  std::string text = composition_.text().text();
  int caret_position = composition_.selection().end();
  int clause_before_caret = 0;
  ggadget::TextFormats formats;
  TextAttributeToTextFormats(composition_.text(), &formats);
  composition_element->SetCompositionText(text);
  composition_element->SetCompositionFormats(formats);
  for (int i = 0; i < composition_.text().attribute_size(); ++i) {
    const ipc::proto::Attribute& attr = composition_.text().attribute(i);
    if (attr.type() == ipc::proto::Attribute::COMPOSITION_STATE) {
      skin::CompositionElement::ClauseStatus status =
          skin::CompositionElement::INACTIVE;
      switch (attr.composition_state()) {
        case ipc::proto::Attribute::CS_TARGET_NOT_CONVERTED:
          status = skin::CompositionElement::ACTIVE;
          break;
        case ipc::proto::Attribute::CS_CONVERTED:
          status = skin::CompositionElement::CONVERTED;
          break;
        case ipc::proto::Attribute::CS_INPUT:
          if (attr.has_color_value())
            status = skin::CompositionElement::HIGHLIGHT;
          else
            status = skin::CompositionElement::INACTIVE;
          break;
        default:
          break;
      }
      composition_element->SetCompositionStatus(attr.start(), attr.end(),
                                                status);
    }
  }
  composition_element->SetCaretPosition(caret_position);
  // TODO(synch): figure out a way to support segmentation labels.
  // composition_element->SetShowSegmentationLabel(::GetKeyState(VK_CONTROL));
  composition_element->UpdateUI();
  UpdateComposingWindowVisibility();
}

void SkinUIComponent::SetCandidateList(
    const ipc::proto::CandidateList* candidate_list) {
  DCHECK(skin_.get());
  if (!skin_.get()) return;
  ime_goopy::skin::CandidateListElement* element = GetCandidateListElement();
  if (!element)
    return;
  element->RemoveAllCandidates();
  if (!candidate_list ||
      (candidate_list->candidate_size() == 0 &&
       !candidate_list->has_footnote())) {
    candidate_list_.Clear();
    if (candidate_list)
      candidate_list_.set_id(candidate_list->id());
    UpdateComposingWindowVisibility();
    return;
  }
  // TODO (synch): refactor candidate list element to fully support specifying
  //    formats for candidates.
  if (candidate_list != &candidate_list_)
    candidate_list_ = *candidate_list;
  std::string default_color_str;
  element->GetCandidateColor().ConvertToString(&default_color_str);
  ggadget::Color default_color;
  ggadget::Color::FromString(default_color_str.c_str(), &default_color, NULL);
  for (int i = 0; i < candidate_list->candidate_size(); ++i) {
    const ipc::proto::Text* ipc_text = &(candidate_list->candidate(i).text());
    std::string text = ipc_text->text();
    if (text.empty()) {
      ipc_text = &(candidate_list->candidate(i).actual_text());
      text = ipc_text->text();
    }
    ggadget::TextFormats formats;
    TextAttributeToTextFormats(*ipc_text, &formats);
    if (i < candidate_list->selection_key_size()) {
      std::string key_text;
      int key = candidate_list->selection_key(i);
      ggadget::ConvertStringUTF16ToUTF8(
          reinterpret_cast<ggadget::UTF16Char*>(&key), 1, &key_text);
      AddCandidateIndex(key_text + ". ", &text, &formats);
    }
    element->AppendCandidateWithFormat(i, text, formats);
  }
  element->SetVisible(candidate_list_.visible());
  if (candidate_list->candidate_size()) {
    GetCandidateListElement()->SetSelectedCandidate(
        candidate_list->selected_candidate());
  }
  std::string help_tips;
  if (candidate_list->has_footnote())
    help_tips = candidate_list->footnote().text();
  skin_->SetHelpMessage(skin::Skin::COMPOSING_VIEW, help_tips.c_str());
  ggadget::ButtonElement* page_up =
      skin_->GetElementByNameAndType<ggadget::ButtonElement>(
          skin::Skin::COMPOSING_VIEW,
          skin::kCandidateListPageUpButton);
  if (page_up) {
    page_up->SetEnabled(
        candidate_list->has_page_start() && candidate_list->page_start() != 0);
  }
  ggadget::ButtonElement* page_down =
      skin_->GetElementByNameAndType<ggadget::ButtonElement>(
          skin::Skin::COMPOSING_VIEW,
          skin::kCandidateListPageDownButton);
  if (page_down) {
    page_down->SetEnabled(
        candidate_list->has_total_candidates() &&
        candidate_list->total_candidates() > candidate_list->page_start() +
                                             candidate_list->candidate_size());
  }
  UpdateComposingWindowVisibility();
  if (is_candidate_list_shown_)
    cursor_trapper_->Restore();
}

void SkinUIComponent::UpdateComposingViewPosition() {
  DCHECK(skin_.get());
  if (!skin_.get()) return;
  ggadget::ViewHostInterface* view_host =
      skin_->GetComposingView()->GetViewHost();
  if (track_caret_) {
    int w = 0;
    int h = 0;
    view_host->GetWindowSize(&w, &h);
    composing_view_position_->SetViewSize(w, h);
    Point<int> pos = composing_view_position_->GetPosition();
    view_host->SetWindowPosition(pos.x, pos.y);
  } else {
    view_host->SetWindowPosition(composing_view_x_, composing_view_y_);
  }
}

void SkinUIComponent::SetSelectedCandidate(int candidate_list_id,
                                           int candidate_index) {
  DCHECK(skin_.get());
  if (!skin_.get()) return;
  if (candidate_list_id == candidate_list_.id()) {
    DCHECK_GE(candidate_index, 0);
    DCHECK_LT(candidate_index, candidate_list_.candidate_size());
    if (candidate_list_.candidate_size() > candidate_index) {
      GetCandidateListElement()->SetSelectedCandidate(candidate_index);
      candidate_list_.set_selected_candidate(candidate_index);
    }
  }
}

void SkinUIComponent::CandidateSelectCallback(uint32 candidate_index,
                                              bool commit) {
  DCHECK_LT(candidate_index, candidate_list_.candidate_size());
  SelectCandidate(
      candidate_list_.owner(),
      candidate_list_.id(),
      candidate_index,
      commit);
}

void SkinUIComponent::ConstructCandidateMenu(
    uint32 candidate_index,
    ggadget::MenuInterface* menu) {
  DCHECK_LT(candidate_index, candidate_list_.candidate_size());
  if (candidate_index >= candidate_list_.candidate_size())
    return;
  const ipc::proto::Candidate& candidate =
      candidate_list_.candidate(candidate_index);
  if (!candidate.has_commands())
    return;
  SkinUIComponentUtils::CommandListToMenuInterface(
      this,
      GetIcid(),
      true,                     // True for a candidate menu.
      candidate_list_.id(),
      candidate_index,
      candidate.commands(),
      menu);
}

void SkinUIComponent::MenuCallback(const char* menu_text,
                                   const CommandInfo& command_info) {
  if (!command_info.is_candidate_command) {
    DoCommand(command_info.owner, command_info.icid, command_info.command_id);
  } else {
    DoCandidateCommand(
        command_info.owner,
        command_info.icid,
        command_info.candidate_list_id,
        command_info.candidate_index,
        command_info.command_id);
  }
}

void SkinUIComponent::SetActiveInputMethod(
    const ipc::proto::ComponentInfo& component) {
  DCHECK(skin_.get());
  if (!skin_.get()) return;
  if (component.language_size() == 0) return;
  bool rtl = LocaleUtils::IsRTLLanguage(component.language(0));
  SetRightToLeftLayout(rtl);
  tool_bar_->SetActiveInputMethod(component);
}

void SkinUIComponent::UpdateComposingWindowVisibility() {
  DCHECK(skin_.get());
  if (!skin_.get()) return;
  ime_goopy::skin::CandidateListElement* element = GetCandidateListElement();
  if (ShouldShowComposingView()) {
    skin_->ShowComposingView();
    if (!is_candidate_list_shown_ &&
        should_show_candidate_list_ &&
        candidate_list_.visible() &&
        (candidate_list_.candidate_size() || candidate_list_.has_footnote())) {
      CandidateListShown(candidate_list_.owner(), candidate_list_.id());
      is_candidate_list_shown_ = true;
    }
  } else {
    skin_->CloseComposingView();
    if (is_candidate_list_shown_) {
      CandidateListHidden(candidate_list_.owner(), candidate_list_.id());
      is_candidate_list_shown_ = false;
    }
    composing_view_position_->Reset();
  }
}

void SkinUIComponent::SetCompositionVisibility(bool show) {
  should_show_composition_ = show;
  UpdateComposingWindowVisibility();
}

void SkinUIComponent::SetCandidateListVisibility(bool show) {
  should_show_candidate_list_ = show;
  UpdateComposingWindowVisibility();
}

void SkinUIComponent::SetToolbarVisibility(bool show) {
  DCHECK(skin_.get());
  if (!skin_.get()) return;
  should_show_toolbar_ = show;
  tool_bar_->SetVisible(show);
  tool_bar_->UpdateToolbarView();
  bool first_run_showed = false;
  settings_->GetBooleanValue(kSettingsFirstRunShowed, &first_run_showed);
  if (!first_run_showed) {
    settings_->SetBooleanValue(kSettingsFirstRunShowed, true);
    ShowFirstRun();
  }
}

void SkinUIComponent::SetCommandList(const CommandLists& command_lists) {
  DCHECK(skin_.get());
  if (!skin_.get()) return;
  tool_bar_->SetCommandLists(command_lists);
}

void SkinUIComponent::SetInputMethods(const ComponentInfos& components) {
  if (!components.size())  // This only happens when ipc console is quiting.
    return;
  DCHECK(skin_.get());
  if (!skin_.get()) return;
  tool_bar_->SetInputMethodList(components);
}

void SkinUIComponent::SetInputCaret(const ipc::proto::InputCaret& caret) {
  DCHECK(skin_.get());
  if (!skin_.get()) return;
  composing_view_position_->SetCaretRect(caret);
  UpdateComposingViewPosition();
}

void SkinUIComponent::ChangeCandidateListVisibility(int id, bool visible) {
  DCHECK(skin_.get());
  if (!skin_.get()) return;
  // TODO(synch): implement this function when we support cascaded candidate
  // list.
  if (id == candidate_list_.id()) {
    candidate_list_.set_visible(visible);
    ime_goopy::skin::CandidateListElement* element = GetCandidateListElement();
    if (!element)
      return;
    element->SetVisible(visible);
    UpdateComposingWindowVisibility();
  }
}

void SkinUIComponent::InitializeComposingView() {
  skin::CandidateListElement* candidate_list = GetCandidateListElement();
  DCHECK(candidate_list);
  candidate_list->ConnectOnShowCandidateContextMenu(
      ggadget::NewSlot(this, &SkinUIComponent::ConstructCandidateMenu));
  candidate_list->ConnectOnCandidateSelected(
      ggadget::NewSlot(this, &SkinUIComponent::CandidateSelectCallback));

  ggadget::BasicElement* page_up = skin_->GetElementByName(
      skin::Skin::COMPOSING_VIEW, skin::kCandidateListPageUpButton);
  if (page_up) {
    page_up->ConnectOnClickEvent(
        NewSlot(this, &SkinUIComponent::PageUpButtonCallback, page_up));
  }

  ggadget::BasicElement* page_down = skin_->GetElementByName(
      skin::Skin::COMPOSING_VIEW, skin::kCandidateListPageDownButton);
  if (page_down) {
    page_down->ConnectOnClickEvent(
        NewSlot(this, &SkinUIComponent::PageDownButtonCallback, page_down));
  }

  ggadget::BasicElement* google_search = skin_->GetElementByName(
      skin::Skin::COMPOSING_VIEW, skin::kGoogleSearchButton);
  if (google_search) {
    google_search->ConnectOnClickEvent(
        ggadget::NewSlot(this, &SkinUIComponent::SearchButtonCallback));
  }
  ggadget::View* composing_view = skin_->GetComposingView();
  DCHECK(composing_view);
  if (!composing_view) return;
  ggadget::ViewHostInterface* view_host = composing_view->GetViewHost();
  DCHECK(view_host);
  if (!view_host) return;
  view_host->ConnectOnEndMoveDrag(
      ggadget::NewSlot(this, &SkinUIComponent::ComposingViewDragEndCallback));
  view_host->ConnectOnShowContextMenu(
      ggadget::NewSlot(this, &SkinUIComponent::OnShowContextMenu));
  view_host->SetFocusable(false);
}

void SkinUIComponent::InitializeToolbarView() {
  tool_bar_.reset(new ToolbarManager(this, skin_.get()));
  if (!tool_bar_->Initialize()) {
    NOTREACHED() << "Toolbar Initial failed";
    tool_bar_.reset(NULL);
    skin_.reset(NULL);
    return;
  }
  ggadget::ViewHostInterface* view_host = skin_->GetMainView()->GetViewHost();
  view_host->SetFocusable(false);
  view_host->ConnectOnShowContextMenu(
      ggadget::NewSlot(this, &SkinUIComponent::OnShowContextMenu));
  tool_bar_->SetVisible(should_show_toolbar_);
  tool_bar_->UpdateToolbarView();
}

void SkinUIComponent::PageUpButtonCallback(ggadget::BasicElement* element) {
  cursor_trapper_->Save(element);
  CandidateListPageUp(candidate_list_.owner(), candidate_list_.id());
}

void SkinUIComponent::PageDownButtonCallback(ggadget::BasicElement* element) {
  cursor_trapper_->Save(element);
  CandidateListPageDown(candidate_list_.owner(), candidate_list_.id());
}

void SkinUIComponent::ComposingViewDragEndCallback(int x, int y) {
  if (!track_caret_) {
    composing_view_x_ = x;
    composing_view_y_ = y;
    settings_->SetIntegerValue(kSettingsComposingViewX, x);
    settings_->SetIntegerValue(kSettingsComposingViewY, y);
  }
}

void SkinUIComponent::SearchButtonCallback() {
  if (candidate_list_.candidate_size() &&
      candidate_list_.selected_candidate() < candidate_list_.candidate_size()) {
    const ipc::proto::Candidate& candidate =
        candidate_list_.candidate(candidate_list_.selected_candidate());
    std::string text;
    if (candidate.has_actual_text() &&
        candidate.actual_text().has_text() &&
        !candidate.actual_text().text().empty()) {
      text = candidate.actual_text().text();
    } else if (candidate.has_text() &&
               candidate.text().has_text() &&
               !candidate.text().text().empty()) {
      text = candidate.text().text();
    }
    if (!text.empty()) {
      GoogleSearchUtils::Search(text);
      return;
    }
  }
  std::string url = GoogleSearchUtils::GoogleHomepageUrl();
  skin_host_->OpenURL(skin_.get(), url.c_str());
}

void SkinUIComponent::OnRegistered() {
  // TODO(synch): use different locale for different language.
  //              and fix the skin path.
  skin::SkinLibraryInitializer::Initialize();
  skin_host_.reset(new ime_goopy::skin::SkinHostWin);
  bool rtl = LocaleUtils::IsRTLLanguage(LocaleUtils::GetUserUILanguage());
  std::string skin_path = FileUtils::GetDataPathForComponent(kComponentStringID) +
      "/default.gskin";
  skin_.reset(skin_host_->LoadSkin(
      skin_path.c_str(),
      "",
      NULL,
      0,
      false,
      vertical_candidate_list_,
      rtl));

  // In rtl system, toolbar should be placed at botton left by default.
  if (rtl)
    toolbar_x_ = 0;

  DCHECK(skin_.get());
  if (!skin_.get()) return;
  skin_->ConnectOnShowImeMenu(
      ggadget::NewSlot(this, &SkinUIComponent::ConstructIMEMenu));
  InitializeComposingView();
  InitializeToolbarView();
  InitializeSettings();
  return;
}

void SkinUIComponent::OnDeregistered() {
  skin_.reset();
  skin_host_.reset();
  skin::SkinLibraryInitializer::Finalize();
}

void SkinUIComponent::SelectInputMethod(uint32 input_method_id) {
  UIComponentBase::SelectInputMethod(input_method_id);
}

void SkinUIComponent::ExecuteCommand(int owner,
                                     int icid,
                                     const std::string& id) {
  DoCommand(owner, icid, id);
}

void SkinUIComponent::OnValueChanged(const std::string& key,
                                     const ipc::proto::VariableArray& array) {
  if (key == kSettingsVerticalCandidateView) {
    SetVerticalCandidateLayout(SkinUIComponentUtils::GetBoolean(array));
  } else if (key == kSettingsTrackCaret) {
    track_caret_ = SkinUIComponentUtils::GetBoolean(array);
  } else if (key == kSettingsComposingViewX) {
    composing_view_x_ = SkinUIComponentUtils::GetInteger(array);
    DCHECK_GE(composing_view_x_, kint16min);
    DCHECK_LE(composing_view_x_, kint16max);
  } else if (key == kSettingsComposingViewY) {
    composing_view_y_ = SkinUIComponentUtils::GetInteger(array);
    DCHECK_GE(composing_view_y_, kint16min);
    DCHECK_LE(composing_view_y_, kint16max);
  } else if (key == kSettingsSemiTransparencyToolbar) {
    semi_tranparent_toolbar_ = SkinUIComponentUtils::GetBoolean(array);
  } else if (key == kSettingsIsToolbarMini) {
    mini_toolbar_ = SkinUIComponentUtils::GetBoolean(array);
  } else if (key == kSettingsIsToolbarCollapse) {
    toolbar_collapsed_ = SkinUIComponentUtils::GetBoolean(array);
  } else if (key == kSettingsToolbarPanelX) {
    toolbar_x_ = SkinUIComponentUtils::GetInteger(array);
    DCHECK_GE(toolbar_x_, kint16min);
    DCHECK_LE(toolbar_x_, kint16max);
  } else if (key == kSettingsToolbarPanelY) {
    toolbar_y_ = SkinUIComponentUtils::GetInteger(array);
    DCHECK_GE(toolbar_y_, kint16min);
    DCHECK_LE(toolbar_y_, kint16max);
  } else if (key == kSettingsIsToolbarFloating) {
    floating_toolbar_ = SkinUIComponentUtils::GetBoolean(array);
  }
}

void SkinUIComponent::InitializeSettings() {
  // Do not care the return value of settings_->GetXXXValue because it will
  // leave the value unchanged if failed.
  settings_->GetBooleanValue(
      kSettingsVerticalCandidateView, &vertical_candidate_list_);
  settings_->GetBooleanValue(kSettingsTrackCaret, &track_caret_);
  settings_->GetIntegerValue(kSettingsComposingViewX, &composing_view_x_);
  settings_->GetIntegerValue(kSettingsComposingViewY, &composing_view_y_);
  settings_->GetBooleanValue(
      kSettingsSemiTransparencyToolbar, &semi_tranparent_toolbar_);
  settings_->GetBooleanValue(
      kSettingsIsToolbarMini, &mini_toolbar_);
  settings_->GetBooleanValue(
      kSettingsIsToolbarCollapse, &toolbar_collapsed_);
  settings_->GetIntegerValue(
      kSettingsToolbarPanelX, &toolbar_x_);
  settings_->GetIntegerValue(
      kSettingsToolbarPanelY, &toolbar_y_);
  settings_->GetBooleanValue(
      kSettingsIsToolbarFloating, &floating_toolbar_);
  DCHECK_GE(composing_view_x_, kint16min);
  DCHECK_LE(composing_view_x_, kint16max);
  DCHECK_GE(composing_view_y_, kint16min);
  DCHECK_LE(composing_view_y_, kint16max);
  DCHECK_GE(toolbar_x_, kint16min);
  DCHECK_LE(toolbar_x_, kint16max);
  DCHECK_GE(toolbar_y_, kint16min);
  DCHECK_LE(toolbar_y_, kint16max);
#ifdef OS_WIN
  // Save pid to settingstore to let frontend allow setting foreground window in
  // this process.
  settings_->SetIntegerValue(kSettingsIPCConsolePID, ::GetCurrentProcessId());
#endif
}

bool SkinUIComponent::IsToolbarFloating() {
  return floating_toolbar_;
}

bool SkinUIComponent::IsToolbarMini() {
  return mini_toolbar_;
}

bool SkinUIComponent::IsToolbarSemiTransparency() {
  return semi_tranparent_toolbar_;
}

bool SkinUIComponent::IsToolbarCollapsed() {
  return toolbar_collapsed_;
}

bool SkinUIComponent::SetToolbarCollapsed(const bool is_collapsed) {
  toolbar_collapsed_ = is_collapsed;
  return settings_->SetBooleanValue(kSettingsIsToolbarCollapse, is_collapsed);
}

bool SkinUIComponent::GetToolbarPanelPos(int32* toolbar_panel_pos_x,
                                         int32* toolbar_panel_pos_y) {
  *toolbar_panel_pos_x = toolbar_x_;
  *toolbar_panel_pos_y = toolbar_y_;
  return true;
}

bool SkinUIComponent::SetToolbarPanelPos(const int toolbar_panel_pos_x,
                                         const int toolbar_panel_pos_y) {
  DCHECK_GE(toolbar_panel_pos_x, kint16min);
  DCHECK_LE(toolbar_panel_pos_x, kint16max);
  DCHECK_GE(toolbar_panel_pos_y, kint16min);
  DCHECK_LE(toolbar_panel_pos_y, kint16max);
  toolbar_x_ = toolbar_panel_pos_x;
  toolbar_y_ = toolbar_panel_pos_y;
  return settings_->SetIntegerValue(kSettingsToolbarPanelX, toolbar_x_) &&
         settings_->SetIntegerValue(kSettingsToolbarPanelY, toolbar_y_);
}

bool SkinUIComponent::ShouldShowComposingView() {
  bool composition_visible = should_show_composition_ &&
                             composition_.has_text() &&
                             composition_.text().text().length();
  bool candidate_list_visible =
      should_show_candidate_list_ &&
      candidate_list_.visible() &&
      (candidate_list_.candidate_size() || candidate_list_.has_footnote());
  return composition_visible || candidate_list_visible;
}

void SkinUIComponent::SetRightToLeftLayout(bool right_to_left) {
  if (!skin_.get())
    return;
  ggadget::BasicElement::TextDirection text_direction = right_to_left ?
      ggadget::BasicElement::TEXT_DIRECTION_RIGHT_TO_LEFT :
      ggadget::BasicElement::TEXT_DIRECTION_LEFT_TO_RIGHT;
  ggadget::BasicElement* container =
      skin_->GetComposingView()->GetElementByName(
          skin::kCompositionContainerElement);
  if (container)
    container->SetTextDirection(text_direction);
  skin::CompositionElement* composition = GetCompositionElement();
  composition->SetTextDirection(text_direction);
  skin::CandidateListElement* candidate_list = GetCandidateListElement();
  candidate_list->SetTextDirection(text_direction);
  composing_view_position_->SetRTL(right_to_left);
}

void SkinUIComponent::SetVerticalCandidateLayout(bool vertical) {
  if (vertical != vertical_candidate_list_) {
    vertical_candidate_list_ = vertical;
    skin_->SetVerticalCandidateLayout(vertical);
    InitializeComposingView();
    RefreshUI();
  }
}

}  // namespace components
}  // namespace ime_goopy
