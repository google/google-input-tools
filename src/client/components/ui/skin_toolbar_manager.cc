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

#include "components/ui/skin_toolbar_manager.h"

#if defined(OS_WIN)
#include <gdiplus.h>
#endif
#include <string>
#include <vector>

#include <google/protobuf/repeated_field.h>

#include "base/logging.h"
#include "base/string_utils_win.h"
#include "components/ui/skin_ui_component_utils.h"
#include "ipc/protos/ipc.pb.h"
#include "ipc/settings_client.h"

#pragma push_macro("DLOG")
#pragma push_macro("LOG")
#include "skin/skin.h"
#include "skin/skin_consts.h"
#include "skin/toolbar_element.h"
#include "third_party/google_gadgets_for_linux/ggadget/basic_element.h"
#include "third_party/google_gadgets_for_linux/ggadget/button_element.h"
#include "third_party/google_gadgets_for_linux/ggadget/common.h"
#include "third_party/google_gadgets_for_linux/ggadget/elements.h"
#include "third_party/google_gadgets_for_linux/ggadget/image_interface.h"
#include "third_party/google_gadgets_for_linux/ggadget/menu_interface.h"
#include "third_party/google_gadgets_for_linux/ggadget/scriptable_binary_data.h"
#include "third_party/google_gadgets_for_linux/ggadget/slot.h"
#include "third_party/google_gadgets_for_linux/ggadget/variant.h"
#include "third_party/google_gadgets_for_linux/ggadget/view.h"
#include "third_party/google_gadgets_for_linux/ggadget/view_host_interface.h"
#if defined(OS_WIN)
#include "third_party/google_gadgets_for_linux/ggadget/win32/gdiplus_image.h"
#endif
#pragma pop_macro("LOG")
#pragma pop_macro("DLOG")

namespace {
static const float kSettingsSemiTransparency = 0.5;
static const float kMiniScale = 0.75;
static const char kTextFormatPrefix[] = "text";
static const char kDefaultToolbarElementName[] = "toolbar_element";
const char kVirtualKeyboardComponentPrefix[] =
    "com.google.input_tools.virtual_keyboard";
// Compare function for ComponentInfo.
static bool ComponentInfoCmp(const ipc::proto::ComponentInfo& left,
                             const ipc::proto::ComponentInfo& right) {
  if (left.language_size() == 0)
    return true;
  if (right.language_size() == 0)
    return false;
  return left.language(0) < right.language(0);
}

struct LanguageDisplayName {
  std::string language;
  const char* display_name;
};

static const LanguageDisplayName kLanguageDisplayName[] = {
  {"am", "AMHARIC"},
  {"ar", "ARABIC"},
  {"bn", "BENGALI"},
  {"el", "GREEK"},
  {"fa", "FARSI"},
  {"gu", "GUJARATI"},
  {"he", "HEBREW"},
  {"hi", "HINDI"},
  {"kn", "KANNADA"},
  {"ml", "MALAYALAM"},
  {"mr", "MARATHI"},
  {"ne", "NEPALI"},
  {"or", "ORIYA"},
  {"pa", "PUNJABI"},
  {"ru", "RUSSIAN"},
  {"sa", "SANSKRIT"},
  {"si", "SINHALESE"},
  {"sr", "SERBIAN"},
  {"ta", "TAMIL"},
  {"te", "TELUGU"},
  {"ti", "TIGRINYA"},
  {"ur", "URDU"},
};

std::string GetLanguageDisplayName(const std::string& language_code) {
  for (int i = 0; i < arraysize(kLanguageDisplayName); ++i) {
    if (kLanguageDisplayName[i].language == language_code) {
      return kLanguageDisplayName[i].display_name;
    }
  }
  return "";
};

}  // namespace

namespace ime_goopy {
namespace components {

// This class implements some util methods for the other manager classes.
class ToolbarManager::ToolbarUtils {
 public:
  static void SetIconGroupByName(const char* normal_image,
                                 const char* disabled_image,
                                 const char* down_image,
                                 const char* over_image,
                                 ipc::proto::IconGroup* icon_group);

  static void SetButtonIconsFromCommand(const ipc::proto::Command& command,
                                        skin::Skin* skin);

  static void CommandListToMenuInterface(
      const ipc::proto::CommandList& command_list,
      int icid,
      ggadget::MenuInterface* menu_interface,
      Delegate* delegate);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(ToolbarUtils);
};

// This class supplies some methods to process the toolbar panel.
class ToolbarManager::PanelManager {
 public:
  PanelManager(ToolbarManager::Delegate* delegate,
               skin::Skin* skin);
  ~PanelManager();

  bool Initialize();
  void SetVisible(bool is_visible);
  bool IsVisible() const;
  void UpdatePanelView();

 private:
  // Handlers of mouse move/drag events.
  void OnToolbarMouseMove(bool is_mouse_hover);
  void OnToolbarEndMoveDrag(int32 x, int32 y);

  void RegisterPanel();
  void UpdateOpacity();

  skin::Skin* skin_;
  ToolbarManager::Delegate* delegate_;

  bool is_mouse_hover_;
  bool is_visible_;

  DISALLOW_COPY_AND_ASSIGN(PanelManager);
};

// This class manages the toolbar_element. It takes responsibility for managing
// all the elements on toolbar_element, which includes registering element
// events and showing button icons.
class ToolbarManager::ToolbarElementManager {
 public:
  enum ButtonAttribute {
    BUTTON_ALWAYS_VISIBLE = 1,
    BUTTON_ALWAYS_INVISIBLE = 1 << 1,
    BUTTON_IS_UI_BUTTON = 1 << 2,
    // The default button layout is forward.
    BUTTON_LAYOUT_BACKWARD = 1 << 3,
  };

  ToolbarElementManager(ToolbarManager::Delegate* delegate,
                        skin::Skin* skin);
  ~ToolbarElementManager();

  bool Initialize();
  void SetButtonsFromCommandLists(const CommandLists& command_lists);

 private:
  // Handlers of mouse click/right click events.
  void OnButtonClicked(const CommandInfo& command_info);
  void OnToolbarCollapseExpandButtonClicked();
  void OnElementContextMenu(
      ggadget::BasicElement* element,
      ggadget::MenuInterface* menu_interface,
      const ipc::proto::Command* command);

  // Appends special buttons.
  void AppendDefaultElements();
  void RegisterToolbarCollapseExpandButton();
  // Removes all the component buttons. Before that, the method will disconnect
  // all the connections with those buttons.
  void RemoveComponentButtons();
  ggadget::BasicElement* AppendButton(const ipc::proto::Command& command,
                                      uint8 button_attributes);

  skin::Skin* skin_;
  ToolbarManager::Delegate* delegate_;

  // Stores the two state icons of toolbar_collapse_expand button.
  ipc::proto::Command expand_command_;
  ipc::proto::Command collapse_command_;
  skin::ToolbarElement* toolbar_element_;
  // Stores all the buttons owned by other components;
  std::vector<std::string> component_buttons_;
  // Stores the connections of all the component commands.
  std::vector<ggadget::Connection*> component_connections_;
  std::vector<ipc::proto::Command*> cached_commands_;

  DISALLOW_COPY_AND_ASSIGN(ToolbarElementManager);
};

class ToolbarManager::ImeSelectionManager {
 public:
  ImeSelectionManager(ToolbarManager::Delegate* delegate,
                      skin::Skin* skin);
  ~ImeSelectionManager();

  bool Initialize();
  void SetInputMethodList(const ComponentInfos& input_method_list);
  void SetActiveInputMethod(
      const ipc::proto::ComponentInfo& active_input_method);

 private:
  // Handlers of mouse click/right click events.
  void OnImeSelectionButtonClicked(ggadget::BasicElement* element);
  void OnImeSelectionContextMenu(
      ggadget::BasicElement* element,
      ggadget::MenuInterface* menu_interface);
  void OnInputMethodSelected(const char* menu_text,
                             uint32 input_method_id);

  void RegisterImeSelectionButton();
  void AddInputMethodToMenuInterface(
      const ipc::proto::ComponentInfo& input_method,
      ggadget::MenuInterface* menu_interface);
  // Given the name of a input method, gets the corresponding index in
  // input_method_list_ and returns true; otherwise, returns false.
  bool GetInputMethodIndexFromName(const std::string& name, int32* index);

  skin::Skin* skin_;
  ToolbarManager::Delegate* delegate_;

  std::string active_input_method_;
  std::string active_language_;
  // Stores input_mehod_list for building ime selection menu.
  ComponentInfos input_method_list_;

  DISALLOW_COPY_AND_ASSIGN(ImeSelectionManager);
};

class ToolbarManager::ImeMenuManager {
 public:
  ImeMenuManager(ToolbarManager::Delegate* delegate,
                 skin::Skin* skin);
  ~ImeMenuManager();

  bool Initialize();
  void SetCommandLists(const CommandLists& command_lists);
  void AddImeCommandListToMenuInterface(
      ggadget::MenuInterface* menu_interface);

 private:
  // Handlers of mouse click/right click events.
  void OnImeMenuButtonClicked(ggadget::BasicElement* element);
  void OnImeMenuContextMenu(ggadget::MenuInterface* menu_interface);

  void RegisterImeMenuButton();

  skin::Skin* skin_;
  ToolbarManager::Delegate* delegate_;

  // Stores the command_lists for building ime menu.
  CommandLists command_lists_;

  DISALLOW_COPY_AND_ASSIGN(ImeMenuManager);
};

void ToolbarManager::ToolbarUtils::SetIconGroupByName(
    const char* normal_image,
    const char* disabled_image,
    const char* down_image,
    const char* over_image,
    ipc::proto::IconGroup* icon_group) {
  icon_group->mutable_normal()->set_data(normal_image);
  icon_group->mutable_normal()->set_format(kTextFormatPrefix);
  icon_group->mutable_disabled()->set_data(disabled_image);
  icon_group->mutable_disabled()->set_format(kTextFormatPrefix);
  icon_group->mutable_down()->set_data(down_image);
  icon_group->mutable_down()->set_format(kTextFormatPrefix);
  icon_group->mutable_over()->set_data(over_image);
  icon_group->mutable_over()->set_format(kTextFormatPrefix);
}

void ToolbarManager::ToolbarUtils::SetButtonIconsFromCommand(
    const ipc::proto::Command& command,
    skin::Skin* skin) {
  if (!skin)
    return;

  ggadget::ButtonElement* element = ggadget::down_cast<ggadget::ButtonElement*>(
      skin->GetMainView()->GetElementByName(command.id().c_str()));
  DCHECK(element);
  if (!element) {
    DLOG(ERROR) << "Missing element: " << command.id();
    return;
  }

  const ggadget::Variant& src =
      SkinUIComponentUtils::DataToVariant(command.state_icon().normal()).v();

  switch (src.type()) {
    case ggadget::Variant::TYPE_STRING: {
      // Set button images by paths.
      skin->SetButtonImagesByNames(
          element,
          command.state_icon().normal().data().c_str(),
          command.state_icon().down().data().c_str(),
          command.state_icon().over().data().c_str(),
          command.state_icon().disabled().data().c_str());
      break;
    }
    case ggadget::Variant::TYPE_SCRIPTABLE: {
      // Set button images by binaries.
      element->SetImage(SkinUIComponentUtils::DataToVariant(
          command.state_icon().normal()).v());
      element->SetDownImage(SkinUIComponentUtils::DataToVariant(
          command.state_icon().down()).v());
      element->SetOverImage(SkinUIComponentUtils::DataToVariant(
          command.state_icon().over()).v());
      element->SetDisabledImage(SkinUIComponentUtils::DataToVariant(
          command.state_icon().disabled()).v());
      break;
    }
    default:
      return;
  }
}

void ToolbarManager::ToolbarUtils::CommandListToMenuInterface(
    const ipc::proto::CommandList& command_list,
    int icid,
    ggadget::MenuInterface* menu_interface,
    Delegate* delegate) {
  SkinUIComponentUtils::CommandListToMenuInterface(delegate,
                                                   // Since it isn't candidate
                                                   // command, set the second
                                                   // parameter to be false and
                                                   // the next two parameters
                                                   // are no use.
                                                   icid,
                                                   false,
                                                   -1,
                                                   -1,
                                                   command_list,
                                                   menu_interface);
}


ToolbarManager::PanelManager::PanelManager(ToolbarManager::Delegate* delegate,
                                           skin::Skin* skin)
    : delegate_(delegate),
      skin_(skin),
      is_mouse_hover_(false),
      is_visible_(true) {
  DCHECK(delegate_);
  DCHECK(skin_);
}

ToolbarManager::PanelManager::~PanelManager() {
}

bool ToolbarManager::PanelManager::Initialize() {
  RegisterPanel();
  return true;
}

void ToolbarManager::PanelManager::RegisterPanel() {
  ggadget::View* view = skin_->GetMainView();
  DCHECK(view);
  if (!view)
    return;
  // Register handlers of mouse move/drag events.
  view->ConnectOnMouseOverEvent(
      ggadget::NewSlot(this,
                       &PanelManager::OnToolbarMouseMove,
                       true));
  view->ConnectOnMouseOutEvent(
      ggadget::NewSlot(this,
                       &PanelManager::OnToolbarMouseMove,
                       false));

  ggadget::ViewHostInterface* view_host = view->GetViewHost();
  DCHECK(view_host);
  if (!view_host)
    return;
  view_host->ConnectOnEndMoveDrag(
      ggadget::NewSlot(this,
                       &PanelManager::OnToolbarEndMoveDrag));
}

void ToolbarManager::PanelManager::SetVisible(bool is_visible) {
  if (is_visible_ == is_visible)
    return;

  is_visible_ = is_visible;
  UpdatePanelView();
}

bool ToolbarManager::PanelManager::IsVisible() const {
  if (!is_visible_)
    return false;

  bool is_floating = delegate_->IsToolbarFloating();
  if (!is_floating)
    return false;

  return true;
}

void ToolbarManager::PanelManager::UpdatePanelView() {
  if (!IsVisible()) {
    skin_->CloseMainView();
    return;
  }

  ggadget::View* view = skin_->GetMainView();
  ggadget::ViewHostInterface* view_host = skin_->GetMainView()->GetViewHost();
  DCHECK(view_host);
  if (!view_host)
    return;

  bool is_mini = delegate_->IsToolbarMini();
  view_host->SetZoom(is_mini ? kMiniScale : 1);

  // If the position has not been set, put the toolbar in the lower right corner
  // of screen.
  int32 toolbar_panel_pos_x = kint16max;
  int32 toolbar_panel_pos_y = kint16max;

  delegate_->GetToolbarPanelPos(&toolbar_panel_pos_x, &toolbar_panel_pos_y);
  view_host->SetWindowPosition(toolbar_panel_pos_x, toolbar_panel_pos_y);
  view_host->GetWindowPosition(&toolbar_panel_pos_x, &toolbar_panel_pos_y);
  delegate_->SetToolbarPanelPos(toolbar_panel_pos_x, toolbar_panel_pos_y);

  UpdateOpacity();

  skin_->ShowMainView();
}

void ToolbarManager::PanelManager::OnToolbarMouseMove(bool is_mouse_hover) {
  is_mouse_hover_ = is_mouse_hover;
  UpdateOpacity();
}

void ToolbarManager::PanelManager::OnToolbarEndMoveDrag(int32 x, int32 y) {
  delegate_->SetToolbarPanelPos(x, y);
}

void ToolbarManager::PanelManager::UpdateOpacity() {
  ggadget::ViewHostInterface* view_host = skin_->GetMainView()->GetViewHost();
  DCHECK(view_host);
  if (!view_host)
    return;

  bool is_semi_transparency = delegate_->IsToolbarSemiTransparency();
  if (is_semi_transparency && !is_mouse_hover_) {
    view_host->SetOpacity(kSettingsSemiTransparency);
  } else {
    view_host->SetOpacity(1.0);
  }
}

ToolbarManager::ToolbarElementManager::ToolbarElementManager(
    ToolbarManager::Delegate* delegate,
    skin::Skin* skin)
    : delegate_(delegate),
      skin_(skin) {
  DCHECK(delegate_);
  DCHECK(skin_);
}

ToolbarManager::ToolbarElementManager::~ToolbarElementManager() {
}

bool ToolbarManager::ToolbarElementManager::Initialize() {
  component_buttons_.clear();
  component_connections_.clear();

  ggadget::View* view = skin_->GetMainView();
  toolbar_element_ = ggadget::down_cast<skin::ToolbarElement*>(
      view->GetElementByName(skin::kToolbarElement));
  // If toolbar_element_ has not been defined in xml files, create a new one.
  // We don't need to delete it explicitly since it will be deleted when view is
  // destroyed.
  if (!toolbar_element_) {
    toolbar_element_ = new skin::ToolbarElement(view,
                                                kDefaultToolbarElementName);
    view->GetChildren()->AppendElement(toolbar_element_);
  }

  toolbar_element_->Initialize();
  bool is_collapsed = delegate_->IsToolbarCollapsed();
  toolbar_element_->SetCollapsed(is_collapsed);

  // Do these append and register works after toolbar_element_ is initialized.
  AppendDefaultElements();
  RegisterToolbarCollapseExpandButton();

  return true;
}

void ToolbarManager::ToolbarElementManager::SetButtonsFromCommandLists(
    const CommandLists& command_lists) {
  // Do some clean work before set/reset new buttons.
  RemoveComponentButtons();

  for (size_t i = 0; i < command_lists.size(); ++i) {
    const ipc::proto::CommandList& command_list = command_lists.Get(i);
    for (size_t j = 0; j < command_list.command_size(); ++j) {
      const ipc::proto::Command& command = command_list.command(j);
      if (!command.has_id() || !command.has_state_icon())
        continue;

      ggadget::BasicElement* element = AppendButton(command, 0);
      DCHECK(element);
      if (!element)
        continue;

      CommandInfo command_info(delegate_->GetIcid(),
                               command_list.owner(),
                               command.id());
      // Register handler of mouse click event.
      component_connections_.push_back(element->ConnectOnClickEvent(
          ggadget::NewSlot(this,
                           &ToolbarElementManager::OnButtonClicked,
                           command_info)));
      // If the command has sub command_list, register handler of mouse right
      // click event.
      if (command.has_sub_commands()) {
        ipc::proto::Command* new_command = new ipc::proto::Command;
        new_command->CopyFrom(command);
        cached_commands_.push_back(new_command);
        component_connections_.push_back(
            skin_->ConnectOnElementContextMenuEvent(
                element,
                ggadget::NewSlot(this,
                                 &ToolbarElementManager::OnElementContextMenu,
                                 const_cast<const ipc::proto::Command*>(
                                     new_command))));
      }
    }
  }
}

void ToolbarManager::ToolbarElementManager::OnButtonClicked(
    const CommandInfo& command_info) {
  delegate_->ExecuteCommand(command_info.owner,
                            command_info.icid,
                            command_info.command_id);
}

void ToolbarManager::ToolbarElementManager::
    OnToolbarCollapseExpandButtonClicked() {
  // Reverse the is_collapsed property of toolbar_element_.
  bool is_collapsed = !toolbar_element_->IsCollapsed();

  toolbar_element_->SetCollapsed(is_collapsed);
  ToolbarManager::ToolbarUtils::SetButtonIconsFromCommand(
      is_collapsed ? expand_command_ : collapse_command_,
      skin_);
  delegate_->SetToolbarCollapsed(is_collapsed);
  ggadget::BasicElement* element = skin_->GetMainView()->GetElementByName(
      skin::kToolbarCollapseExpandButton);
  DCHECK(element);
  if (!element)
    return;
  std::wstring tooltip =
    toolbar_element_->IsCollapsed() ? L"EXPAND" : L"COLLAPSE";
  element->SetTooltip(WideToUtf8(tooltip));
}

void ToolbarManager::ToolbarElementManager::OnElementContextMenu(
    ggadget::BasicElement* element,
    ggadget::MenuInterface* menu_interface,
    const ipc::proto::Command* command) {
  ToolbarUtils::CommandListToMenuInterface(command->sub_commands(),
                                           delegate_->GetIcid(),
                                           menu_interface,
                                           delegate_);
}

void ToolbarManager::ToolbarElementManager::AppendDefaultElements() {
  ipc::proto::Command command;
  // Append ime_selection button to toolbar if it has not been defined in xml
  // files.
  command.set_id(skin::kImeSelectionButton);
  ggadget::BasicElement* element =
      skin_->GetMainView()->GetElementByName(command.id().c_str());
  if (!element)
    AppendButton(command, BUTTON_IS_UI_BUTTON | BUTTON_LAYOUT_BACKWARD);

  // Append ime_menu button to toolbar.
  command.set_id(skin::kImeMenuButton);
  AppendButton(command, BUTTON_IS_UI_BUTTON | BUTTON_LAYOUT_BACKWARD);
  // Append toolbar_collapse_expand Button.
  command.set_id(skin::kToolbarCollapseExpandButton);
  AppendButton(command,
               BUTTON_ALWAYS_VISIBLE |
               BUTTON_IS_UI_BUTTON |
               BUTTON_LAYOUT_BACKWARD);
}

void ToolbarManager::ToolbarElementManager::
    RegisterToolbarCollapseExpandButton() {
  collapse_command_.set_id(skin::kToolbarCollapseExpandButton);
  expand_command_.set_id(skin::kToolbarCollapseExpandButton);
  bool rtl = skin_->GetMainView()->IsTextRTL();
  if (rtl) {
    // In rtl layout, we should swap the collapse and expand icons because
    // they are direction sensitive.
    ToolbarUtils::SetIconGroupByName(skin::kToolbarCollapseIcon,
                                     skin::kToolbarCollapseDisabledIcon,
                                     skin::kToolbarCollapseDownIcon,
                                     skin::kToolbarCollapseOverIcon,
                                     expand_command_.mutable_state_icon());
    ToolbarUtils::SetIconGroupByName(skin::kToolbarExpandIcon,
                                     skin::kToolbarExpandDisabledIcon,
                                     skin::kToolbarExpandDownIcon,
                                     skin::kToolbarExpandOverIcon,
                                     collapse_command_.mutable_state_icon());
  } else {
    ToolbarUtils::SetIconGroupByName(skin::kToolbarCollapseIcon,
                                     skin::kToolbarCollapseDisabledIcon,
                                     skin::kToolbarCollapseDownIcon,
                                     skin::kToolbarCollapseOverIcon,
                                     collapse_command_.mutable_state_icon());
    ToolbarUtils::SetIconGroupByName(skin::kToolbarExpandIcon,
                                     skin::kToolbarExpandDisabledIcon,
                                     skin::kToolbarExpandDownIcon,
                                     skin::kToolbarExpandOverIcon,
                                     expand_command_.mutable_state_icon());
  }

  // If the toolbar_element_ is collapsed, the direction of the button should
  // be "expand", and vice versa.
  ToolbarUtils::SetButtonIconsFromCommand(
      toolbar_element_->IsCollapsed() ? expand_command_ : collapse_command_,
      skin_);

  ggadget::BasicElement* element =
      skin_->GetMainView()->GetElementByName(
          skin::kToolbarCollapseExpandButton);
  DCHECK(element);
  if (!element)
    return;
  // Register handler of mouse click event.
  element->ConnectOnClickEvent(ggadget::NewSlot(
      this,
      &ToolbarElementManager::OnToolbarCollapseExpandButtonClicked));
  std::wstring tooltip =
          toolbar_element_->IsCollapsed() ? L"EXPAND" : L"COLLAPSE";
  element->SetTooltip(WideToUtf8(tooltip));
}

void ToolbarManager::ToolbarElementManager::RemoveComponentButtons() {
  // Be sure to disconnect all the connections of the component commands before
  // remove the corresponding buttons.
  for (size_t i = 0; i < component_connections_.size(); ++i) {
    if (component_connections_[i])
      component_connections_[i]->Disconnect();
  }
  for (size_t i = 0; i < cached_commands_.size(); ++i) {
    if (cached_commands_[i])
      delete cached_commands_[i];
  }
  cached_commands_.clear();
  component_connections_.clear();

  for (size_t i = 0; i < component_buttons_.size(); ++i)
    toolbar_element_->RemoveButton(component_buttons_[i].c_str());
  component_buttons_.clear();
}

ggadget::BasicElement* ToolbarManager::ToolbarElementManager::AppendButton(
    const ipc::proto::Command& command,
    uint8 button_attributes) {
  skin::ToolbarElement::ButtonVisibilityType visibility;
  bool is_visible = false;
  // visibility records the ButtonVisibilityType of this button in
  // toolbar_element, while is_visible records the visible property of a basic
  // element. They are depend on the parameter always_visible, command's visible
  // property and toolbar_element's is_collapsed property.
  if (button_attributes & BUTTON_ALWAYS_VISIBLE) {
    visibility = skin::ToolbarElement::ALWAYS_VISIBLE;
    is_visible = true;
  } else if (!command.visible()) {
    visibility = skin::ToolbarElement::ALWAYS_INVISIBLE;
    is_visible = false;
  } else {
    visibility = skin::ToolbarElement::NORMAL_VISIBILITY;
    is_visible = !toolbar_element_->IsCollapsed();
  }

  ggadget::LinearElement::LayoutDirection direction =
      (button_attributes & BUTTON_LAYOUT_BACKWARD) ?
      ggadget::LinearElement::LAYOUT_BACKWARD:
      ggadget::LinearElement::LAYOUT_FORWARD;

  ggadget::BasicElement* element =
      toolbar_element_->AddButton(command.id().c_str(), direction, visibility);
  element->SetEnabled(command.enabled());
  element->SetVisible(is_visible);
  element->SetTooltip(command.tooltip().text());
  ToolbarUtils::SetButtonIconsFromCommand(command, skin_);

  // If it is not ui button, add it to component_buttons_ list.
  if (!(button_attributes & BUTTON_IS_UI_BUTTON))
    component_buttons_.push_back(command.id());

  return element;
}

ToolbarManager::ImeSelectionManager::ImeSelectionManager(
    ToolbarManager::Delegate* delegate,
    skin::Skin* skin)
    : delegate_(delegate),
      skin_(skin) {
  DCHECK(delegate_);
  DCHECK(skin_);
}

ToolbarManager::ImeSelectionManager::~ImeSelectionManager() {
}

bool ToolbarManager::ImeSelectionManager::Initialize() {
  input_method_list_.Clear();

  RegisterImeSelectionButton();

  return true;
}

void ToolbarManager::ImeSelectionManager::SetInputMethodList(
    const ComponentInfos& input_method_list) {
  DCHECK_GT(input_method_list.size(), 0);
  if (!input_method_list.size())
    return;
  input_method_list_ = input_method_list;

  // Since the size of input_method_list_ is small, bubble sort is enough.
  for (int i = 0; i < input_method_list_.size() - 1; ++i) {
    for (int j = i + 1; j < input_method_list_.size(); ++j) {
      if (!ComponentInfoCmp(input_method_list_.Get(i),
                            input_method_list_.Get(j)))
        input_method_list_.SwapElements(i, j);
    }
  }
}

void ToolbarManager::ImeSelectionManager::SetActiveInputMethod(
    const ipc::proto::ComponentInfo& active_input_method) {
  active_input_method_ = active_input_method.string_id();
  if (active_input_method.language_size())
    active_language_ = active_input_method.language(0);
  ipc::proto::Command command;
  command.set_id(skin::kImeSelectionButton);
  *command.mutable_state_icon() = active_input_method.icon();
  ToolbarUtils::SetButtonIconsFromCommand(command, skin_);
}

void ToolbarManager::ImeSelectionManager::OnImeSelectionButtonClicked(
    ggadget::BasicElement* element) {
  // Treat left click as right click for this button.
  ggadget::ViewHostInterface* view_host = element->GetView()->GetViewHost();

  if (view_host) {
    view_host->ShowContextMenu(ggadget::MouseEvent::BUTTON_RIGHT);
  }
}

/* The view of ime selection menu should be like this:
 *  ___________________________________   _____________________________
 * |   icon_1  | input_method_1        | | icon_2_1 | input_method_2_1 |
 * |           | language_2            |-| icon_2_2 | input_method_2_2 |
 * |   icon_3  | input_method_3        | | icon_2_3 | input_method_2_3 |
 * |           |        ... ...        | |  ... ... |     ... ...      |
 * |___________|_______________________| |__________|__________________|
 *
 */
void ToolbarManager::ImeSelectionManager::OnImeSelectionContextMenu(
    ggadget::BasicElement* element,
    ggadget::MenuInterface* menu_interface) {
  ggadget::MenuInterface* sub_menu_interface = NULL;
  std::string last_language = "";
  // Traverse the input_method_list_.
  for (size_t i = 0; i < input_method_list_.size(); ++i) {
    const ipc::proto::ComponentInfo& input_method = input_method_list_.Get(i);
    if (!input_method.language_size() || input_method.language(0).empty())
      continue;
    // TODO(synch): find a technique to allow ime components to prevent
    // themselves from appearing in the language list.
    if (input_method.string_id().find(kVirtualKeyboardComponentPrefix) == 0)
      continue;
    // If the language of this input method should be added to the
    // sub_menu_interface, set this boolean value to be true.
    bool is_sub_language = true;
    if (input_method.language(0) != last_language) {
      // If the language of next index is the same with this one, create a new
      // sub_menu_interface.
      if (i + 1 != input_method_list_.size() &&
          input_method_list_.Get(i + 1).language(0) ==
          input_method.language(0)) {
        sub_menu_interface = menu_interface->AddPopup(
                GetLanguageDisplayName(input_method.language(0)).c_str(),
                ggadget::MenuInterface::MENU_ITEM_PRI_GADGET);
        is_sub_language = true;
      } else {
        is_sub_language = false;
      }
    }

    AddInputMethodToMenuInterface(input_method,
                                  is_sub_language ? sub_menu_interface :
                                                    menu_interface);
    last_language = input_method.language(0);
  }
}

void ToolbarManager::ImeSelectionManager::OnInputMethodSelected(
    const char* menu_text,
    uint32 input_method_id) {
  delegate_->SelectInputMethod(input_method_id);
}

void ToolbarManager::ImeSelectionManager::RegisterImeSelectionButton() {
  ggadget::BasicElement* element =
      skin_->GetMainView()->GetElementByName(skin::kImeSelectionButton);
  DCHECK(element);
  if (!element)
    return;
  skin_->ConnectOnElementContextMenuEvent(
      element,
      NewSlot(this,
              &ToolbarManager::ImeSelectionManager::OnImeSelectionContextMenu));
  // Register handlers of mouse click/right click event.
  element->ConnectOnClickEvent(
      ggadget::NewSlot(this,
                       &ImeSelectionManager::OnImeSelectionButtonClicked,
                       element));
  std::wstring tooltip = L"SELECT_IME";
  element->SetTooltip(WideToUtf8(tooltip));
}

void ToolbarManager::ImeSelectionManager::AddInputMethodToMenuInterface(
    const ipc::proto::ComponentInfo& input_method,
    ggadget::MenuInterface* menu_interface) {
  ggadget::ImageInterface *image_icon = NULL;

  if (input_method.has_icon()) {
    ggadget::ResultVariant result_src =
        SkinUIComponentUtils::DataToVariant(input_method.icon().normal());
    const ggadget::Variant& src = result_src.v();

    // TODO(jiangwei): Move these code to util files for more general usage.
    switch (src.type()) {
      case ggadget::Variant::TYPE_STRING: {
        const char *file_name = ggadget::VariantValue<const char*>()(src);
        // Get ImageInterface of icon from global resource.
        image_icon =
            skin_->GetMainView()->LoadImageFromGlobal(file_name, false);
        break;
      }
      case ggadget::Variant::TYPE_SCRIPTABLE: {
        // TODO(chencaibin): Implement for other platforms
#ifdef OS_WIN
        const ggadget::ScriptableBinaryData* binary =
            ggadget::VariantValue<const ggadget::ScriptableBinaryData*>()(src);
        image_icon = new ggadget::win32::GdiplusImage();
        // Get ImageInterface of icon directly from icon binary.
        ggadget::down_cast<ggadget::win32::GdiplusImage*>(image_icon)->
            Init(input_method.name(), binary->data(), false);
        break;
#endif
      }
      default:
        return;
    }
  }

  bool checked = (input_method.string_id() == active_input_method_);
  if (active_input_method_.find(kVirtualKeyboardComponentPrefix) == 0 &&
      input_method.language_size()) {
    checked = input_method.language(0) == active_language_;
  }
  int flag = checked ? ggadget::MenuInterface::MENU_ITEM_FLAG_CHECKED : 0;

  // menu_interface will own the image_icon object.
  menu_interface->AddItem(
      input_method.name().c_str(),
      flag,
      image_icon,
      ggadget::NewSlot(this,
                       &ImeSelectionManager::OnInputMethodSelected,
                       input_method.id()),
      ggadget::MenuInterface::MENU_ITEM_PRI_GADGET);
}

bool ToolbarManager::ImeSelectionManager::GetInputMethodIndexFromName(
    const std::string& name, int32* index) {
  for (size_t i = 0; i < input_method_list_.size(); ++i) {
    const ipc::proto::ComponentInfo& input_method = input_method_list_.Get(i);
    if (input_method.name() == name) {
      *index = i;
      return true;
    }
  }

  DLOG(ERROR) << "Unexpected input method name received: " << name;
  return false;
}

ToolbarManager::ImeMenuManager::ImeMenuManager(
    ToolbarManager::Delegate* delegate,
    skin::Skin* skin)
    : delegate_(delegate),
      skin_(skin) {
  DCHECK(delegate_);
  DCHECK(skin_);
}

ToolbarManager::ImeMenuManager::~ImeMenuManager() {
}

bool ToolbarManager::ImeMenuManager::Initialize() {
  command_lists_.Clear();
  RegisterImeMenuButton();

  return true;
}

void ToolbarManager::ImeMenuManager::SetCommandLists(
    const CommandLists& command_lists) {
  command_lists_ = command_lists;
}

void ToolbarManager::ImeMenuManager::AddImeCommandListToMenuInterface(
    ggadget::MenuInterface* menu_interface) {
  bool is_collapsed = delegate_->IsToolbarCollapsed();

  for (size_t i = 0; i < command_lists_.size(); ++i) {
    // Copy a CommandList object.
    // Decide if a command should be added to a menu item accoarding to its
    // property visible. If it's true, add it to a menu item.
    ipc::proto::CommandList command_list = command_lists_.Get(i);
    bool has_item = false;

    for (size_t j = 0; j < command_list.command_size(); ++j) {
      ipc::proto::Command* command = command_list.mutable_command(j);
      // If the visible property is false, do not add the command to ime menu.
      if (!command->visible())
        continue;

      // If the toolbar is collapse, or it's expand but the command does not has
      // state icon, add this command to ime menu.
      if (is_collapsed || !command->has_state_icon()) {
        has_item = true;
      } else {
        command->set_visible(false);
      }
    }

    if (!has_item)
      continue;

    ToolbarUtils::CommandListToMenuInterface(command_list,
                                             delegate_->GetIcid(),
                                             menu_interface,
                                             delegate_);
  }
}

void ToolbarManager::ImeMenuManager::OnImeMenuButtonClicked(
    ggadget::BasicElement* element) {
  ggadget::ViewHostInterface* view_host = element->GetView()->GetViewHost();
  DCHECK(view_host);
  if (!view_host)
    return;

  view_host->ShowContextMenu(ggadget::MouseEvent::BUTTON_RIGHT);
}

void ToolbarManager::ImeMenuManager::OnImeMenuContextMenu(
    ggadget::MenuInterface* menu_interface) {
  delegate_->ConstructIMEMenu(menu_interface);
}

void ToolbarManager::ImeMenuManager::RegisterImeMenuButton() {
  ipc::proto::Command command;
  ipc::proto::IconGroup* icon_group = command.mutable_state_icon();

  command.set_id(skin::kImeMenuButton);
  ToolbarUtils::SetIconGroupByName(skin::kImeMenuIcon,
                                   skin::kImeMenuDisabledIcon,
                                   skin::kImeMenuDownIcon,
                                   skin::kImeMenuOverIcon,
                                   icon_group);
  ToolbarUtils::SetButtonIconsFromCommand(command, skin_);

  ggadget::BasicElement* element =
      skin_->GetMainView()->GetElementByName(skin::kImeMenuButton);
  DCHECK(element);
  if (!element)
    return;

  // Register handlers of mouse click/right click event.
  element->ConnectOnClickEvent(
      ggadget::NewSlot(this,
                       &ImeMenuManager::OnImeMenuButtonClicked,
                       element));
  std::wstring tooltip = L"SHOW_MENU";
  element->SetTooltip(WideToUtf8(tooltip));
}

ToolbarManager::ToolbarManager(Delegate* delegate,
                               skin::Skin* skin) {
  DCHECK(delegate);
  DCHECK(skin);
  panel_manager_.reset(new PanelManager(delegate, skin));
  toolbar_element_manager_.reset(new ToolbarElementManager(delegate, skin));
  ime_selection_manager_.reset(new ImeSelectionManager(delegate, skin));
  ime_menu_manager_.reset(new ImeMenuManager(delegate, skin));
}

ToolbarManager::~ToolbarManager() {
}

bool ToolbarManager::Initialize() {
  return panel_manager_->Initialize() &&
         toolbar_element_manager_->Initialize() &&
         ime_selection_manager_->Initialize() &&
         ime_menu_manager_->Initialize();
}

void ToolbarManager::SetCommandLists(const CommandLists& command_lists) {
  toolbar_element_manager_->SetButtonsFromCommandLists(command_lists);
  ime_menu_manager_->SetCommandLists(command_lists);
}

void ToolbarManager::SetInputMethodList(
    const ComponentInfos& input_method_list) {
  ime_selection_manager_->SetInputMethodList(input_method_list);
}

void ToolbarManager::SetActiveInputMethod(
    const ipc::proto::ComponentInfo& active_input_method) {
  ime_selection_manager_->SetActiveInputMethod(active_input_method);
}

void ToolbarManager::AddImeCommandListToMenuInterface(
    ggadget::MenuInterface* menu_interface) {
  ime_menu_manager_->AddImeCommandListToMenuInterface(menu_interface);
}

void ToolbarManager::SetVisible(bool visible) {
  panel_manager_->SetVisible(visible);
}

bool ToolbarManager::IsVisible() const {
  return panel_manager_->IsVisible();
}

void ToolbarManager::UpdateToolbarView() {
  panel_manager_->UpdatePanelView();
}

}  // namespace components
}  // namespace ime_goopy
