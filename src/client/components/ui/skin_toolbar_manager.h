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

#ifndef GOOPY_COMPONENTS_UI_SKIN_TOOLBAR_MANAGER_H_
#define GOOPY_COMPONENTS_UI_SKIN_TOOLBAR_MANAGER_H_

#include <string>
#include <vector>

#include "base/basictypes.h"
#include "base/scoped_ptr.h"
#include "components/ui/skin_ui_component_utils.h"

namespace google {
namespace protobuf {
template <class T> class RepeatedPtrField;
}  // namespace protobuf
}  // namespace google

namespace ggadget {
class MenuInterface;
}  // namespace ggadget

namespace ipc {
namespace proto {
class CommandList;
class ComponentInfo;
class VariableArray;
}  // namespace proto
}  // namespace ipc

namespace ime_goopy {
namespace skin {
class Skin;
class ToolbarElement;
}  // namespace skin
namespace components {
// A class for managing the toolbar element.
class ToolbarManager {
 public:
  typedef google::protobuf::RepeatedPtrField<ipc::proto::CommandList>
      CommandLists;
  typedef google::protobuf::RepeatedPtrField<ipc::proto::ComponentInfo>
      ComponentInfos;

  // A delegate interface.
  // The creator of toolbar manager should implement this interface.
  class Delegate : public SkinCommandCallbackInterface {
   public:
    virtual int GetIcid() const = 0;
    virtual void SelectInputMethod(uint32 input_method_id) = 0;
    virtual void ExecuteCommand(int owner, int icid, const std::string& id) = 0;
    virtual void ConstructIMEMenu(ggadget::MenuInterface* menu_interface) = 0;

    virtual bool IsToolbarFloating() = 0;
    virtual bool IsToolbarMini() = 0;
    virtual bool IsToolbarSemiTransparency() = 0;
    virtual bool SetToolbarCollapsed(bool is_collapsed) = 0;
    virtual bool IsToolbarCollapsed() = 0;

    virtual bool SetToolbarPanelPos(int32 toolbar_panel_pos_x,
                                    int32 toolbar_panel_pos_y) = 0;
    virtual bool GetToolbarPanelPos(int32* toolbar_panel_pos_x,
                                    int32* toolbar_panel_pos_y) = 0;
  };

  ToolbarManager(Delegate* delegate,
                 skin::Skin* skin);
  ~ToolbarManager();

  bool Initialize();

  // Sets/Resets all the command lists of toolbar element.
  // This method is called by ui component when command lists changed and
  // need to be updated in toolbar.
  void SetCommandLists(const CommandLists& command_lists);

  // Sets/Resets the input_method_list of toolbar element.
  void SetInputMethodList(const ComponentInfos& input_method_list);

  // Sets/Resets the active_input_method of toolbar element.
  void SetActiveInputMethod(
      const ipc::proto::ComponentInfo& active_input_method);

  // Sets and gets whether or not the toolbar is visible.
  void SetVisible(bool visible);
  bool IsVisible() const;

  // Adds Ime related command_list to menu.
  void AddImeCommandListToMenuInterface(
      ggadget::MenuInterface* menu_interface);

  // Updates the view of toolbar element.
  void UpdateToolbarView();

 private:
  // Some util class and helper managers to support the implement of
  // toolbar_manager.
  class ToolbarUtils;
  class PanelManager;
  class ToolbarElementManager;
  class ImeSelectionManager;
  class ImeMenuManager;

  scoped_ptr<ImeSelectionManager> ime_selection_manager_;
  scoped_ptr<ImeMenuManager> ime_menu_manager_;
  scoped_ptr<ToolbarElementManager> toolbar_element_manager_;
  scoped_ptr<PanelManager> panel_manager_;

  DISALLOW_COPY_AND_ASSIGN(ToolbarManager);
};

}  // namespace components
}  // namespace ime_goopy

#endif  // GOOPY_COMPONENTS_UI_SKIN_TOOLBAR_MANAGER_H_
