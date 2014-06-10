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

#ifndef GOOPY_COMPONENTS_UI_SKIN_UI_COMPONENT_UTILS_H_
#define GOOPY_COMPONENTS_UI_SKIN_UI_COMPONENT_UTILS_H_

#include "base/basictypes.h"
#include "base/logging.h"
#include "components/ui/ui_types.h"
#include "ipc/protos/ipc.pb.h"

namespace ggadget {
class MenuInterface;
class ResultVariant;
class ViewHostInterface;
#if defined(OS_WIN)
namespace win32 {
class MenuBuilder;
}  // namespace win32
#endif
}  // namespace ggadget

namespace ime_goopy {
namespace components {
// A struct to hold informations of a command (or a candidate command).
struct CommandInfo {
  CommandInfo();
  // Initialize a command.
  CommandInfo(int icid, int param_owner, const std::string& param_command_id);
  // Initialize a candidate command.
  CommandInfo(int icid,
              int param_owner,
              int param_candidate_list_id,
              int param_candidate_index,
              const std::string& param_command_id);
  int icid;
  bool is_candidate_command;
  int owner;
  int candidate_list_id;
  int candidate_index;
  std::string command_id;
  // Defined for Slot.
  bool operator==(const CommandInfo& b) const;
};

class SkinCommandCallbackInterface {
 public:
  virtual void MenuCallback(const char* menu_text,
                            const CommandInfo& command_info) = 0;
};

class SkinUIComponentUtils {
 public: // Platform-independent utility functions.
  // An utility function to convert a ipc::protoData object to a
  // ggadget::ResultVariant object. The caller should also store the return
  // value in ResultVariant to hold the reference count of any
  // ScriptableInterface in the returned ResultVariant.
  static ggadget::ResultVariant DataToVariant(const ipc::proto::Data& data);
  // Adds the commands in command list to the MenuInterface |menu|.
  // Set |is_candidate_command| to false if the command list is associated with
  // any candidate, and the param |candidate_index| and |candidate_list_id| will
  // be omitted.
  static void CommandListToMenuInterface(
      SkinCommandCallbackInterface* owner,
      int icid,
      bool is_candidate_command,
      int candidate_list_id,
      int candidate_index,
      const ipc::proto::CommandList& commands,
      ggadget::MenuInterface* menu);
  static bool GetBoolean(const ipc::proto::VariableArray& value);
  static int64 GetInteger(const ipc::proto::VariableArray& value);

 public: // Platform-dependent utility functions.
#if defined(OS_WIN)
  // Convert a menu builder object to a command list.
  static void MenuInterfaceToCommandList(
      const ggadget::win32::MenuBuilder* menu,
      ipc::proto::CommandList* command_list);
  // Excecute a menu command in |menu| by the string id.
  static void ExecuteMenuCommand(const ggadget::win32::MenuBuilder* menu,
                                 const std::string& id);

#endif // OS_WIN

  // Gets cursor position related to the origin of the top-level window of the
  // view host.
  static Point<int> GetCursorPosOnView(
      ggadget::ViewHostInterface *view_host);

  // Gets cursor to the given position related to the origin of the top-level
  // window of the view host.
  static void SetCursorPosOnView(ggadget::ViewHostInterface *view_host,
                                 Point<int> cursor_pos);

  static Rect<int> GetScreenRectAtPoint(Point<int> pt);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(SkinUIComponentUtils);
};

}  // namespace components
}  // namespace ime_goopy
#endif  // GOOPY_COMPONENTS_UI_SKIN_UI_COMPONENT_UTILS_H_
