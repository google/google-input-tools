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

#include "components/win_frontend/daily_pinger_component.h"
#include <string>
#include "base/basictypes.h"
#include "base/logging.h"
#include "base/scoped_ptr.h"
#include "base/stringprintf.h"
#include "common/omaha_const.h"
#include "common/registry.h"
#include "common/registry_monitor.h"
#include "ipc/component_base.h"
#include "ipc/message_types.h"

namespace ime_goopy {
namespace components {

namespace {

const int kConsumeMessages[] = {
  ipc::MSG_INSERT_TEXT,
};

}  // namespace

using ipc::proto::ComponentInfo;
using ipc::proto::Message;

DailyPingerComponent::DailyPingerComponent(ipc::ComponentBase* owner)
    : ipc::SubComponentBase(owner) {
}

DailyPingerComponent::~DailyPingerComponent() {
}

void DailyPingerComponent::GetInfo(ComponentInfo* info) {
  for (size_t i = 0; i < arraysize(kConsumeMessages); ++i)
    info->add_consume_message(kConsumeMessages[i]);
}

void DailyPingerComponent::OnRegistered() {
}

void DailyPingerComponent::OnDeregistered() {
}

bool DailyPingerComponent::Handle(Message* message) {
  if (message->type() == ipc::MSG_INSERT_TEXT &&
      message->has_payload() &&
      message->payload().has_composition() &&
      message->payload().composition().has_text() &&
      !message->payload().composition().text().text().empty()) {
    scoped_ptr<RegistryKey> registry(
        RegistryKey::OpenKey(HKEY_CURRENT_USER,
                             WideStringPrintf(
                                 L"%s\\%s",
                                 kOmahaClientStateKey,
                                 kOmahaAppGuid),
                             KEY_WOW64_32KEY | KEY_WRITE,
                             true /* Create the value if not exists */));
    if (registry.get())
      registry->SetStringValue(kOmahaDailyPingValue, L"1");
  }
  return false;
}

}  // namespace components
}  // namespace ime_goopy
