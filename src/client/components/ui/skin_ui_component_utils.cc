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

#include "components/ui/skin_ui_component_utils.h"

#include "base/stringprintf.h"

#pragma push_macro("LOG")
#pragma push_macro("DLOG")
#include "third_party/google_gadgets_for_linux/ggadget/menu_interface.h"
#include "third_party/google_gadgets_for_linux/ggadget/scriptable_binary_data.h"
#include "third_party/google_gadgets_for_linux/ggadget/slot.h"
#include "third_party/google_gadgets_for_linux/ggadget/variant.h"
#pragma pop_macro("DLOG")
#pragma pop_macro("LOG")


namespace ime_goopy {
namespace components {
namespace {
static const char kTextFormatPrefix[] = "text";
} // namespace

CommandInfo::CommandInfo()
  : icid(0),
    is_candidate_command(false),
    owner(0),
    candidate_list_id(0),
    candidate_index(0) {
}

CommandInfo::CommandInfo(int param_icid,
                         int param_owner,
                         const std::string& param_command_id)
  : icid(param_icid),
    is_candidate_command(false),
    owner(param_owner),
    candidate_list_id(0),
    candidate_index(0),
    command_id(param_command_id) {
}

CommandInfo::CommandInfo(int param_icid,
                         int param_owner,
                         int param_candidate_list_id,
                         int param_candidate_index,
                         const std::string& param_command_id)
  : icid(param_icid),
    is_candidate_command(true),
    owner(param_owner),
    candidate_list_id(param_candidate_list_id),
    candidate_index(param_candidate_index),
    command_id(param_command_id) {
}

bool CommandInfo::operator==(const CommandInfo& b) const {
  return ((is_candidate_command && b.is_candidate_command &&
           candidate_list_id == b.candidate_list_id &&
           candidate_index == b.candidate_index) ||
          (!is_candidate_command && !b.is_candidate_command)) &&
         owner == b.owner &&
         command_id == b.command_id &&
         icid == b.icid;
}

void SkinUIComponentUtils::CommandListToMenuInterface(
    SkinCommandCallbackInterface* owner,
    int icid,
    bool is_candidate_command,
    int candidate_list_id,
    int candidate_index,
    const ipc::proto::CommandList& commands,
    ggadget::MenuInterface* menu) {
  CommandInfo command_info;
  command_info.icid = icid;
  command_info.is_candidate_command = is_candidate_command;
  if (is_candidate_command) {
    command_info.candidate_index = candidate_index;
    command_info.candidate_list_id = candidate_list_id;
  }
  command_info.owner = commands.owner();
  for (int i = 0; i < commands.command_size(); ++i) {
    const ipc::proto::Command& command = commands.command(i);
    if (command.has_visible() && !command.visible())
      continue;
    if (command.has_sub_commands()) {
      ggadget::MenuInterface* sub_menu = menu->AddPopup(
          command.title().text().c_str(),
          ggadget::MenuInterface::MENU_ITEM_PRI_GADGET);
      CommandListToMenuInterface(
          owner, icid, is_candidate_command, candidate_list_id, candidate_index,
          command.sub_commands(), sub_menu);
    } else {
      int menu_flag = 0;
      switch (command.state()) {
        case ipc::proto::Command::CHECKED:
          menu_flag = ggadget::MenuInterface::MENU_ITEM_FLAG_CHECKED;
          break;
        case ipc::proto::Command::SEPARATOR:
          menu_flag = ggadget::MenuInterface::MENU_ITEM_FLAG_SEPARATOR;
          break;
        default:
          break;
      }
      if (command.has_enabled() && !command.enabled())
        menu_flag |= ggadget::MenuInterface::MENU_ITEM_FLAG_GRAYED;
      command_info.command_id = command.id();
      std::string menu_text = command.title().text();
      if (command.has_hotkey_hint() && command.hotkey_hint().has_text()) {
        menu_text += '\t';
        menu_text += command.hotkey_hint().text();
      }

      menu->AddItem(menu_text.c_str(),
                    menu_flag,
                    0,
                    ggadget::NewSlot(
                        owner,
                        &SkinCommandCallbackInterface::MenuCallback,
                        command_info),
                    ggadget::MenuInterface::MENU_ITEM_PRI_GADGET);
    }
  }
}

ggadget::ResultVariant SkinUIComponentUtils::DataToVariant(
    const ipc::proto::Data& data) {
  if (data.has_format() && data.format().find(kTextFormatPrefix) == 0) {
    return ggadget::ResultVariant(ggadget::Variant(data.data()));
  } else {
    ggadget::ScriptableBinaryData* binary =
      new ggadget::ScriptableBinaryData(data.data());
    return ggadget::ResultVariant(ggadget::Variant(binary));
  }
}

bool SkinUIComponentUtils::GetBoolean(const ipc::proto::VariableArray& value) {
  if (value.variable_size() != 1 ||
      value.variable(0).type() != ipc::proto::Variable::BOOLEAN) {
    DCHECK(false);
  }
  return value.variable(0).boolean();
}

int64 SkinUIComponentUtils::GetInteger(const ipc::proto::VariableArray& value) {
  if (value.variable_size() != 1 ||
      value.variable(0).type() != ipc::proto::Variable::INTEGER) {
    DCHECK(false);
  }
  return value.variable(0).integer();
}

}  // namespace components
}  // namespace ime_goopy
