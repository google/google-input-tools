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

#include "ipc/hub_component.h"

#include "ipc/hub_hotkey_list.h"
#include "ipc/constants.h"

namespace ipc {
namespace hub {

Component::Component(uint32 id,
                     Connector* connector,
                     const proto::ComponentInfo& info)
    : info_(info),
      connector_(connector) {
  info_.set_id(id);

  int size = info_.produce_message_size();
  for (int i = 0; i < size; ++i)
    produce_set_.insert(info_.produce_message(i));

  // Recreate |info_|'s produce_message array to remove possible duplicated
  // entries.
  info_.clear_produce_message();
  MessageTypeSet::iterator iter = produce_set_.begin();
  MessageTypeSet::iterator end = produce_set_.end();
  for (; iter != end; ++iter)
    info_.add_produce_message(*iter);

  size = info_.consume_message_size();
  for (int i = 0; i < size; ++i)
    consume_set_.insert(info_.consume_message(i));

  // Recreate |info_|'s consume_message array to remove possible duplicated
  // entries.
  info_.clear_consume_message();
  iter = consume_set_.begin();
  end = consume_set_.end();
  for (; iter != end; ++iter)
    info_.add_consume_message(*iter);
}

Component::~Component() {
  HotkeyListMap::iterator i = hotkey_list_map_.begin();
  HotkeyListMap::iterator end = hotkey_list_map_.end();
  for (; i != end; ++i)
    delete i->second;
}

void Component::AddHotkeyList(const proto::HotkeyList& hotkey_list) {
  uint32 list_id = hotkey_list.id();
  HotkeyListMap::iterator i = hotkey_list_map_.find(list_id);

  // Delete the old HotkeyList object.
  if (i != hotkey_list_map_.end())
    delete i->second;

  HotkeyList* new_list = new HotkeyList(hotkey_list);
  new_list->set_owner(id());

  hotkey_list_map_[list_id] = new_list;
}

void Component::RemoveHotkeyList(uint32 id) {
  HotkeyListMap::iterator i = hotkey_list_map_.find(id);
  if (i != hotkey_list_map_.end()) {
    delete i->second;
    hotkey_list_map_.erase(i);
  }
}

const HotkeyList* Component::GetHotkeyList(uint32 id) const {
  HotkeyListMap::const_iterator iter = hotkey_list_map_.find(id);
  return iter != hotkey_list_map_.end() ? iter->second : NULL;
}

bool Component::MatchInfoTemplate(const proto::ComponentInfo& query) const {
  if (query.has_id() && query.id() != info_.id())
    return false;
  if (query.has_string_id() && query.string_id() != info_.string_id())
    return false;
  if (query.has_name() && query.name() != info_.name())
    return false;
  if (query.has_description() && query.description() != info_.description())
    return false;
  if (query.language_size()) {
    // TODO(suzhe): Support BCP-47 language tag format.
    std::set<std::string> languages;
    for (int i = 0; i < info_.language_size(); ++i)
      languages.insert(info_.language(i));
    for (int i = 0; i < query.language_size(); ++i) {
      if (!languages.count(query.language(i)))
        return false;
    }
  }
  if (query.produce_message_size()) {
    for (int i = 0; i < query.produce_message_size(); ++i) {
      if (!produce_set_.count(query.produce_message(i)))
        return false;
    }
  }
  if (query.consume_message_size()) {
    for (int i = 0; i < query.consume_message_size(); ++i) {
      if (!consume_set_.count(query.consume_message(i)))
        return false;
    }
  }
  return true;
}

}  // namespace hub
}  // namespace ipc
