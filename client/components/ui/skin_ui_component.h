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

#ifndef GOOPY_COMPONENTS_UI_SKIN_UI_COMPONENT_H_
#define GOOPY_COMPONENTS_UI_SKIN_UI_COMPONENT_H_

#include "components/ui/ui_component_base.h"

#include "base/basictypes.h"
#include "base/scoped_ptr.h"
#include "components/ui/skin_toolbar_manager.h"
#include "components/ui/skin_ui_component_utils.h"
#include "ipc/settings_client.h"

#pragma push_macro("LOG")
#pragma push_macro("DLOG")
#pragma push_macro("VERIFY")
#include "skin/skin.h"
#pragma pop_macro("VERIFY")
#pragma pop_macro("DLOG")
#pragma pop_macro("LOG")

namespace ggadget {
class HostInterface;
class Variant;
}  // namespace ggadget
namespace ime_goopy {

namespace frontend {
class ToolbarManager;
}  // namespace frontend

namespace skin {
class CandidateListElement;
class CompositionElement;
class SkinHostWin;
}  // namespace skin

namespace components {
class ComposingWindowPosition;
class CursorTrapper;

// A component used skin to display ime UI.
class SkinUIComponent
    : public UIComponentBase,
      public ToolbarManager::Delegate,
      public ipc::SettingsClient::Delegate {
 public:
  SkinUIComponent();
  virtual ~SkinUIComponent();

  // Overridden from ComponentBase:
  virtual void OnRegistered() OVERRIDE;
  virtual void OnDeregistered() OVERRIDE;

  // Overridden from SkinCommandCallbackInterface.
  virtual void MenuCallback(const char* menu_text,
                            const CommandInfo& command_info) OVERRIDE;
  // Overridden from ToolbarManager::Delegate;
  virtual int GetIcid() const OVERRIDE {
    return GetFocusedIcid();
  }
  virtual void SelectInputMethod(uint32 input_method_id) OVERRIDE;
  virtual void ExecuteCommand(int owner,
                              int icid,
                              const std::string& id) OVERRIDE;
  virtual void ConstructIMEMenu(ggadget::MenuInterface* menu) OVERRIDE;
  virtual bool IsToolbarFloating() OVERRIDE;
  virtual bool IsToolbarMini() OVERRIDE;
  virtual bool IsToolbarSemiTransparency() OVERRIDE;
  virtual bool IsToolbarCollapsed() OVERRIDE;
  virtual bool SetToolbarCollapsed(bool is_collapsed) OVERRIDE;
  virtual bool GetToolbarPanelPos(int32* toolbar_panel_pos_x,
                                  int32* toolbar_panel_pos_y) OVERRIDE;
  virtual bool SetToolbarPanelPos(int toolbar_panel_pos_x,
                                  int toolbar_panel_pos_y) OVERRIDE;

  // Overridden from ipc::SetttingsClient::Delegate.
  virtual void OnValueChanged(const std::string& key,
                              const ipc::proto::VariableArray& array) OVERRIDE;

 protected:
  virtual std::string GetComponentStringID() OVERRIDE;
  virtual std::string GetComponentName() OVERRIDE;
  virtual void SetComposition(
      const ipc::proto::Composition* composition) OVERRIDE;
  virtual void SetCandidateList(
      const ipc::proto::CandidateList* candidate_list) OVERRIDE;
  virtual void SetSelectedCandidate(int candidate_list_id,
                                    int candidate_index) OVERRIDE;
  virtual void SetCompositionVisibility(bool show) OVERRIDE;
  virtual void SetCandidateListVisibility(bool show) OVERRIDE;
  virtual void SetToolbarVisibility(bool show) OVERRIDE;
  virtual void SetCommandList(const CommandLists& command_lists) OVERRIDE;
  virtual void ChangeCandidateListVisibility(int id, bool visible) OVERRIDE;
  virtual void SetInputMethods(const ComponentInfos& components) OVERRIDE;
  virtual void SetActiveInputMethod(
      const ipc::proto::ComponentInfo& component) OVERRIDE;
  virtual void SetInputCaret(const ipc::proto::InputCaret& caret) OVERRIDE;

 private:  // Platform-dependent methods
  // Event handler for showing context menu. Returns true if you want skin to
  // display the menu.
  bool OnShowContextMenu(ggadget::MenuInterface* menu);
  void ConstructCandidateMenu(uint32 candidate_index,
                              ggadget::MenuInterface* menu);

 private:

  void UpdateComposingViewPosition();
  ime_goopy::skin::CandidateListElement* GetCandidateListElement();
  ime_goopy::skin::CompositionElement* GetCompositionElement();
  void InitializeComposingView();
  void InitializeToolbarView();
  void PageUpButtonCallback(ggadget::BasicElement* element);
  void PageDownButtonCallback(ggadget::BasicElement* element);
  void ComposingViewDragEndCallback(int x, int y);
  void SearchButtonCallback();
  void UpdateComposingWindowVisibility();

  void CandidateSelectCallback(uint32 candidate_index, bool commit);
  void GetSettingsOrDefault(const std::string& key,
                            const ipc::proto::Variable& default_value,
                            ipc::proto::Variable* value);
  void InitializeSettings();
  bool ShouldShowComposingView();
  void SetVerticalCandidateLayout(bool vertical);
  void SetRightToLeftLayout(bool right_to_left);
  void ShowFirstRun();

  scoped_ptr<ime_goopy::skin::Skin> skin_;
  scoped_ptr<ime_goopy::skin::SkinHostWin> skin_host_;
  scoped_ptr<ToolbarManager> tool_bar_;
  scoped_ptr<CursorTrapper> cursor_trapper_;
  ipc::SettingsClient* settings_;
  ipc::proto::CandidateList candidate_list_;
  ipc::proto::Composition composition_;
  scoped_ptr<ComposingWindowPosition> composing_view_position_;
  bool should_show_composition_;
  bool should_show_candidate_list_;
  bool should_show_toolbar_;
  bool is_candidate_list_shown_;

  // Cached settings.
  bool vertical_candidate_list_;
  bool track_caret_;
  bool floating_toolbar_;
  bool mini_toolbar_;
  bool semi_tranparent_toolbar_;
  bool toolbar_collapsed_;
  int64 composing_view_x_;
  int64 composing_view_y_;
  int64 toolbar_x_;
  int64 toolbar_y_;

  DISALLOW_COPY_AND_ASSIGN(SkinUIComponent);
};

}  // namespace components
}  // namespace ime_goopy
#endif  // GOOPY_COMPONENTS_UI_SKIN_UI_COMPONENT_H_
