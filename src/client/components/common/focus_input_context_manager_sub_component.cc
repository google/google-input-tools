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
#include "components/common/focus_input_context_manager_sub_component.h"
#include "base/logging.h"
#include "base/scoped_ptr.h"
#include "ipc/component_base.h"
#include "ipc/constants.h"
#include "ipc/protos/ipc.pb.h"
#include "ipc/message_types.h"
#include "ipc/message_util.h"

namespace {

const uint32 kProduceMessages[] = {
  ipc::MSG_QUERY_INPUT_CONTEXT,
};

const uint32 kConsumeMessages[] = {
  ipc::MSG_INPUT_CONTEXT_GOT_FOCUS,
  ipc::MSG_INPUT_CONTEXT_LOST_FOCUS,
};

}  // namespace
namespace ime_goopy {
namespace components {

FocusInputContextManagerSubComponent::FocusInputContextManagerSubComponent(
    ipc::ComponentBase* owner) : SubComponentBase(owner), focus_icid_(-1) {
}

void FocusInputContextManagerSubComponent::GetInfo(
    ipc::proto::ComponentInfo* info) {
  for (size_t i = 0; i < arraysize(kProduceMessages); ++i)
    info->add_produce_message(kProduceMessages[i]);
  for (size_t i = 0; i < arraysize(kConsumeMessages); ++i)
    info->add_consume_message(kConsumeMessages[i]);
}

bool FocusInputContextManagerSubComponent::Handle(
    ipc::proto::Message* message) {
  switch (message->type()) {
    case ipc::MSG_INPUT_CONTEXT_GOT_FOCUS:
      focus_icid_ = message->icid();
      break;
    case ipc::MSG_INPUT_CONTEXT_LOST_FOCUS:
      focus_icid_ = 0;
      break;
  }
  return false;
}

void FocusInputContextManagerSubComponent::OnRegistered() {
  scoped_ptr<ipc::proto::Message> mptr(ipc::NewMessage(
      ipc::MSG_QUERY_INPUT_CONTEXT,
      owner_->id(),
      ipc::kComponentDefault,
      ipc::kInputContextFocused,
      true));
  ipc::proto::Message* reply = NULL;
  if (!owner_->SendWithReply(mptr.release(), -1, &reply)) {
    DLOG(ERROR) << "SendWithReply fails type = MSG_QUERY_INPUT_CONTEXT";
    return;
  }
  DCHECK(reply);
  if (reply->has_payload() && reply->payload().uint32_size())
    focus_icid_ = reply->payload().uint32(0);
  delete reply;
}

void FocusInputContextManagerSubComponent::OnDeregistered() {
  focus_icid_ = 0;
}

uint32 FocusInputContextManagerSubComponent::GetFocusIcid() const {
  return focus_icid_;
}

}  // namespace components
}  // namespace ime_goopy
