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

#include "ipc/hub_input_method_manager.h"

#include <algorithm>

#include "base/logging.h"
#include "base/scoped_ptr.h"
#include "ipc/hub_impl.h"
#include "ipc/hub_scoped_message_cache.h"
#include "ipc/message_util.h"

namespace {

// Messages can be consumed by this built-in component.
const uint32 kConsumeMessages[] = {
  ipc::MSG_COMPONENT_CREATED,
  ipc::MSG_COMPONENT_DELETED,
  ipc::MSG_INPUT_CONTEXT_CREATED,
  ipc::MSG_INPUT_CONTEXT_DELETED,
  ipc::MSG_COMPONENT_ATTACHED,
  ipc::MSG_ACTIVE_CONSUMER_CHANGED,

  ipc::MSG_LIST_INPUT_METHODS,
  ipc::MSG_SWITCH_TO_INPUT_METHOD,
  ipc::MSG_SWITCH_TO_NEXT_INPUT_METHOD_IN_LIST,
  ipc::MSG_SWITCH_TO_PREVIOUS_INPUT_METHOD,
  ipc::MSG_QUERY_ACTIVE_INPUT_METHOD,
};
const size_t kConsumeMessagesSize = arraysize(kConsumeMessages);

const uint32 kProduceMessages[] = {
  ipc::MSG_INPUT_METHOD_ACTIVATED,
  ipc::MSG_CANCEL_COMPOSITION,
};
const size_t kProduceMessagesSize = arraysize(kProduceMessages);

const char kStringID[] = "com.google.ime.goopy.ipc.hub.input-method-manager";
const char kName[] = "Goopy IPC Hub Input Method Manager";

static const bool kUseGlobalInputMethod = false;

const uint32 kMessagesShouldCacheWhenSwitching[] = {
  ipc::MSG_SEND_KEY_EVENT,
  // We should not cache ipc::MSG_PROCESS_KEY_EVENT because it will prevent the
  // input method from being activated, so current we can not handle the case
  // that some application send ipc::MSG_PROCESS_KEY_EVENT directly. But now we
  // don't have such app.
  ipc::MSG_CANCEL_COMPOSITION,
  ipc::MSG_COMPLETE_COMPOSITION,
  ipc::MSG_SWITCH_TO_INPUT_METHOD,
  ipc::MSG_SWITCH_TO_NEXT_INPUT_METHOD_IN_LIST,
  ipc::MSG_SWITCH_TO_PREVIOUS_INPUT_METHOD,
  ipc::MSG_QUERY_ACTIVE_INPUT_METHOD,
};

const size_t kMessagesShouldCacheWhenSwitchingSize =
    arraysize(kMessagesShouldCacheWhenSwitching);

}  // namespace

namespace ipc {
namespace hub {

// A class that stores the switching state of input methods in a context.
// An input method switching action is finished only if:
//   1. the new input method component is attached to the input context.
//   2. the new input method component is activated for message
//      MSG_PROCESS_KEY_EVENT in the icid.
// So we use a InputMethodSwitchingData object to keep track of the state of an
// input method switching action and caches the ime related input messages until
// the switching action finished.
class HubInputMethodManager::InputMethodSwitchingData {
 public:
  enum SwitchingState {
    IME_ACTIVATED_NEEDED = 0x1,
    IME_ATTACHED_NEEDED = 0x2,
  };
  InputMethodSwitchingData(HubImpl* hub,
                           HubInputMethodManager* owner,
                           InputContext* ic,
                           uint32 new_input_method_id,
                           bool need_attached)
      : state_(IME_ACTIVATED_NEEDED),
        new_input_method_id_(new_input_method_id),
        owner_(owner),
        hub_(hub),
        icid_(0) {
    DCHECK(hub_);
    DCHECK(ic);
    icid_ = ic->id();
    if (need_attached)
      state_ |= IME_ATTACHED_NEEDED;
    // Hub will ignore those messages that the component can not consume.
    ic->ResignActiveConsumer(owner_->self_,
                             kMessagesShouldCacheWhenSwitching,
                             kMessagesShouldCacheWhenSwitchingSize);
    message_cache_.reset(new HubScopedMessageCache(
        kMessagesShouldCacheWhenSwitching,
        kMessagesShouldCacheWhenSwitchingSize,
        ic->id(),
        hub_));
    DCHECK(message_cache_.get());
  }

  ~InputMethodSwitchingData() {
    // The input context may have been deleted, so we need to get it from ic
    // instead of memorising it in the constructor.
    InputContext* ic = hub_->GetInputContext(icid_);
    if (ic) {
      ic->AssignActiveConsumer(owner_->self_,
                               kMessagesShouldCacheWhenSwitching,
                               kMessagesShouldCacheWhenSwitchingSize);
    } else {
      DiscardCachedMessages();
    }
  }
  // Checks if the component_id is the id of the target input method, and update
  // the state by clearing the bits in state_changed.
  // Returns true if the target input method is ready for use.
  bool CheckAndUpdateState(uint32 component_id, int state_changed) {
    if (component_id == new_input_method_id_)
      state_ &= ~state_changed;
    return !state_;
  }

  bool IsTargetInputMethod(uint32 component_id) {
    return component_id == new_input_method_id_;
  }

  void DiscardCachedMessages() {
    message_cache_->DiscardCachedMessages();
  }

  uint32 new_input_method_id() {
    return new_input_method_id_;
  }

 private:
  int state_;
  uint32 new_input_method_id_;
  HubImpl* hub_;
  uint32 icid_;
  HubInputMethodManager* owner_;
  scoped_ptr<HubScopedMessageCache> message_cache_;
  DISALLOW_COPY_AND_ASSIGN(InputMethodSwitchingData);
};

HubInputMethodManager::HubInputMethodManager(HubImpl* hub)
    : self_(NULL),
      hub_(hub),
      use_global_input_method_(kUseGlobalInputMethod) {

  // Register this built-in component.
  hub->Attach(this);

  proto::ComponentInfo info;
  info.set_string_id(kStringID);
  info.set_name(kName);

  for(size_t i = 0; i < kConsumeMessagesSize; ++i)
    info.add_consume_message(kConsumeMessages[i]);

  for(size_t i = 0; i < kProduceMessagesSize; ++i)
    info.add_produce_message(kProduceMessages[i]);

  self_ = hub->CreateComponent(this, info, true);
  DCHECK(self_);
}

HubInputMethodManager::~HubInputMethodManager() {
  hub_->Detach(this);
  // |self_| will be deleted automatically when detaching from Hub.
}

bool HubInputMethodManager::Send(proto::Message* message) {
  Component* source = hub_->GetComponent(message->source());
  DCHECK(source);
  switch (message->type()) {
    case MSG_COMPONENT_CREATED:
      return OnMsgComponentCreated(message);
    case MSG_COMPONENT_DELETED:
      return OnMsgComponentDeleted(message);
    case MSG_INPUT_CONTEXT_CREATED:
      return OnMsgInputContextCreated(message);
    case MSG_INPUT_CONTEXT_DELETED:
      return OnMsgInputContextDeleted(message);
    case MSG_COMPONENT_ATTACHED:
      return OnMsgComponentAttached(message);
    case MSG_ACTIVE_CONSUMER_CHANGED:
      return OnMsgActiveConsumerChanged(message);
    case MSG_LIST_INPUT_METHODS:
      return OnMsgListInputMethods(source, message);
    case MSG_SWITCH_TO_INPUT_METHOD:
      return OnMsgSwitchToInputMethod(source, message);
    case MSG_SWITCH_TO_NEXT_INPUT_METHOD_IN_LIST:
      return OnMsgSwitchToNextInputMethodInList(source, message);
    case MSG_SWITCH_TO_PREVIOUS_INPUT_METHOD:
      return OnMsgSwitchToPreviousInputMethod(source, message);
    case MSG_QUERY_ACTIVE_INPUT_METHOD:
      return OnMsgQueryActiveInputMethod(source, message);
    case MSG_CANCEL_COMPOSITION:
      DCHECK_EQ(proto::Message::IS_REPLY, message->reply_mode());
      return OnMsgCancelCompositionReply(source, message);
    default:
      break;
  }
  DLOG(ERROR) << "Unexpected message:" << GetMessageName(message->type());
  delete message;
  return false;
}

bool HubInputMethodManager::OnMsgComponentCreated(proto::Message* message) {
  DCHECK(message->has_payload() && message->payload().component_info_size());
  scoped_ptr<proto::Message> mptr(message);

  const uint32 id = message->payload().component_info(0).id();
  if (!IsInputMethod(hub_->GetComponent(id)))
    return true;

  std::vector<uint32>::iterator i = std::lower_bound(
      all_input_methods_.begin(), all_input_methods_.end(), id);
  DCHECK(i == all_input_methods_.end() || *i != id);
  all_input_methods_.insert(i, id);

  DLOG(INFO) << "Input Method added: id:" << id << " string_id:"
             << message->payload().component_info(0).string_id();
  return true;
}

bool HubInputMethodManager::OnMsgComponentDeleted(proto::Message* message) {
  DCHECK(message->has_payload() && message->payload().uint32_size());
  scoped_ptr<proto::Message> mptr(message);

  const uint32 id = message->payload().uint32(0);
  std::vector<uint32>::iterator i = std::lower_bound(
      all_input_methods_.begin(), all_input_methods_.end(), id);
  if (i != all_input_methods_.end() && *i == id) {
    all_input_methods_.erase(i);
    DLOG(INFO) << "Input Method deleted: id:" << id;
  }
  // If target input method is deleted from host, we should stop caching
  // messages in all contexts that is switching to the deleted input method,
  // And we should dispatch the caching messages in case that some frontends
  // are waiting for the reply of MSG_SEND_KEY_EVENT.
  std::vector<uint32> should_remove;
  for (InputMethodSwitchingDataMap::iterator it = switching_data_.begin();
       it != switching_data_.end();
       ++it) {
    if (it->second->IsTargetInputMethod(id))
      should_remove.push_back(it->first);
  }
  for (size_t i = 0; i < should_remove.size(); ++i)
    DeleteSwitchingData(should_remove[i], false);
  return true;
}

bool HubInputMethodManager::OnMsgInputContextCreated(proto::Message* message) {
  DCHECK(message->has_payload() && message->payload().has_input_context_info());
  scoped_ptr<proto::Message> mptr(message);

  const uint32 icid = message->payload().input_context_info().id();
  InputContext* ic = hub_->GetInputContext(icid);

  hub_->AttachToInputContext(self_, ic, InputContext::ACTIVE_STICKY, true);
  return true;
}

bool HubInputMethodManager::OnMsgInputContextDeleted(proto::Message* message) {
  DCHECK(message->has_payload() && message->payload().uint32_size());
  scoped_ptr<proto::Message> mptr(message);

  const uint32 icid = message->payload().uint32(0);
  current_input_methods_.erase(icid);
  previous_input_methods_.erase(icid);
  DeleteSwitchingData(icid, true);
  return true;
}

bool HubInputMethodManager::OnMsgComponentAttached(proto::Message* message) {
  scoped_ptr<proto::Message> mptr(message);
  if (!mptr->has_payload() || mptr->payload().uint32_size() != 2)
    return false;
  uint32 icid = mptr->payload().uint32(0);
  uint32 component = mptr->payload().uint32(1);
  UpdateSwitchingData(icid, component,
                      InputMethodSwitchingData::IME_ATTACHED_NEEDED);
  return true;
}

bool HubInputMethodManager::OnMsgActiveConsumerChanged(
    proto::Message* message) {
  DCHECK(message->has_payload() && message->payload().uint32_size());
  scoped_ptr<proto::Message> mptr(message);

  bool input_method_changed = false;
  const int size = message->payload().uint32_size();
  for (int i = 0; i < size; ++i) {
    if (message->payload().uint32(i) == MSG_PROCESS_KEY_EVENT) {
      input_method_changed = true;
      break;
    }
  }

  if (!input_method_changed)
    return true;

  // TODO(suzhe): check if |current| is a valid input method.
  InputContext* ic = hub_->GetInputContext(message->icid());
  Component* current = GetCurrentInputMethod(ic);
  std::map<uint32, std::string>::iterator iter =
      current_input_methods_.find(ic->id());

  if (iter != current_input_methods_.end()) {
    // Do nothing if the input method is not changed.
    if (current && current->string_id() == iter->second)
      return true;
    previous_input_methods_[ic->id()] = iter->second;
  } else if (ic->id() != kInputContextNone &&
             previous_input_methods_.count(kInputContextNone)) {
    // It's the first time that an input method is attached to the input
    // context, so we use the global previous input method for it.
    previous_input_methods_[ic->id()] =
        previous_input_methods_[kInputContextNone];
  }

  if (current)
    current_input_methods_[ic->id()] = current->string_id();

  // Broadcast MSG_INPUT_METHOD_ACTIVATED only when necessary.
  if (ic->MayConsume(MSG_INPUT_METHOD_ACTIVATED, false)) {
    message->set_type(MSG_INPUT_METHOD_ACTIVATED);
    message->set_source(self_->id());
    message->set_target(kComponentBroadcast);
    message->set_reply_mode(proto::Message::NO_REPLY);
    message->clear_payload();
    if (current) {
      message->mutable_payload()->add_component_info()->CopyFrom(
          current->info());
    }
    hub_->Dispatch(this, mptr.release());
  }
  if (!current)
    return true;
  UpdateSwitchingData(ic->id(), current->id(),
                      InputMethodSwitchingData::IME_ACTIVATED_NEEDED);
  return true;
}

bool HubInputMethodManager::OnMsgListInputMethods(
    Component* source, proto::Message* message) {
  scoped_ptr<proto::Message> mptr(message);
  Connector* connector = source->connector();
  // The message sender must wait for the reply message.
  if (message->reply_mode() != proto::Message::NEED_REPLY) {
    return hub_->ReplyError(
        connector, mptr.release(), proto::Error::INVALID_REPLY_MODE);
  }

  InputContext* ic = hub_->GetInputContext(message->icid());
  if (!ic) {
    return hub_->ReplyError(
        connector, mptr.release(), proto::Error::INVALID_INPUT_CONTEXT);
  }

  ConvertToReplyMessage(message);
  proto::MessagePayload* payload = message->mutable_payload();
  payload->Clear();

  const size_t size = all_input_methods_.size();
  for (size_t i = 0; i < size; ++i) {
    Component* input_method = hub_->GetComponent(all_input_methods_[i]);
    if (!hub_->IsComponentValid(input_method))
      continue;
    payload->add_component_info()->CopyFrom(input_method->info());
    payload->add_boolean(ValidateInputMethod(input_method, ic));
  }

  connector->Send(mptr.release());
  return true;
}

bool HubInputMethodManager::OnMsgSwitchToInputMethod(
    Component* source, proto::Message* message) {
  DLOG(INFO) << "OnMsgSwitchToInputMethod.";
  scoped_ptr<proto::Message> mptr(message);
  Connector* connector = source->connector();
  InputContext* ic = hub_->GetInputContext(message->icid());
  if (!ic) {
    return hub_->ReplyError(
        connector, mptr.release(), proto::Error::INVALID_INPUT_CONTEXT);
  }
  if (!message->has_payload()) {
    return hub_->ReplyError(
        connector, mptr.release(), proto::Error::INVALID_PAYLOAD);
  }

  const proto::MessagePayload& payload = message->payload();

  // Either integer id or string id should be used.
  if ((payload.uint32_size() && payload.string_size()) ||
      (!payload.uint32_size() && !payload.string_size())) {
    return hub_->ReplyError(
        connector, mptr.release(), proto::Error::INVALID_PAYLOAD);
  }

  Component* input_method = NULL;
  if (payload.uint32_size())
    input_method = hub_->GetComponent(payload.uint32(0));
  else
    input_method = hub_->GetComponentByStringID(payload.string(0));

  if (!IsInputMethod(input_method) || !ValidateInputMethod(input_method, ic)) {
    return hub_->ReplyError(
        connector, mptr.release(), proto::Error::INVALID_PAYLOAD);
  }

  const bool result = SwitchToInputMethod(ic, input_method);
  return hub_->ReplyBoolean(connector, mptr.release(), result);
}

bool HubInputMethodManager::OnMsgSwitchToNextInputMethodInList(
    Component* source, proto::Message* message) {
  DLOG(INFO) << "OnMsgSwitchToNextInputMethodInList.";
  scoped_ptr<proto::Message> mptr(message);
  Connector* connector = source->connector();
  InputContext* ic = hub_->GetInputContext(message->icid());
  if (!ic) {
    return hub_->ReplyError(
        connector, mptr.release(), proto::Error::INVALID_INPUT_CONTEXT);
  }

  const bool result = SwitchToNextInputMethodInList(ic);
  return hub_->ReplyBoolean(connector, mptr.release(), result);
}

bool HubInputMethodManager::OnMsgSwitchToPreviousInputMethod(
    Component* source, proto::Message* message) {
  DLOG(INFO) << "OnMsgSwitchToPreviousInputMethod.";
  scoped_ptr<proto::Message> mptr(message);
  Connector* connector = source->connector();
  InputContext* ic = hub_->GetInputContext(message->icid());
  if (!ic) {
    return hub_->ReplyError(
        connector, mptr.release(), proto::Error::INVALID_INPUT_CONTEXT);
  }

  const bool result = SwitchToPreviousInputMethod(ic);
  return hub_->ReplyBoolean(connector, mptr.release(), result);
}

bool HubInputMethodManager::OnMsgQueryActiveInputMethod(
    Component* source, proto::Message* message) {
  DLOG(INFO) << "OnMsgQueryActiveInputMethod.";
  scoped_ptr<proto::Message> mptr(message);
  Connector* connector = source->connector();
  // The message sender must wait for the reply message.
  if (message->reply_mode() != proto::Message::NEED_REPLY) {
    return hub_->ReplyError(
        connector, mptr.release(), proto::Error::INVALID_REPLY_MODE);
  }

  InputContext* ic = hub_->GetInputContext(message->icid());
  if (!ic) {
    return hub_->ReplyError(
        connector, mptr.release(), proto::Error::INVALID_INPUT_CONTEXT);
  }

  Component* current = GetCurrentInputMethod(ic);
  if (!current) {
    return hub_->ReplyError(
        connector, mptr.release(), proto::Error::COMPONENT_NOT_FOUND);
  }

  ConvertToReplyMessage(message);
  message->mutable_payload()->Clear();
  message->mutable_payload()->add_component_info()->CopyFrom(current->info());

  connector->Send(mptr.release());
  return true;
}

bool HubInputMethodManager::OnMsgCancelCompositionReply(
    Component* source, proto::Message* message) {
  scoped_ptr<proto::Message> mptr(message);
  if (mptr->reply_mode() != proto::Message::IS_REPLY)
    return false;
  int icid = message->icid();
  Component* new_input_method = hub_->GetComponent(
      switching_data_[icid]->new_input_method_id());
  InputContext* ic = hub_->GetInputContext(icid);
  SwitchToInputMethodAfterCancelComposition(ic, new_input_method);
  return true;
}

Component* HubInputMethodManager::GetCurrentInputMethod(
    const InputContext* ic) const {
  return ic->GetActiveConsumer(MSG_PROCESS_KEY_EVENT);
}

Component* HubInputMethodManager::GetPreviousInputMethod(
    const InputContext* ic) const {
  std::map<uint32, std::string>::const_iterator i =
      previous_input_methods_.find(ic->id());

  if (i == previous_input_methods_.end())
    return GetNextInputMethodInList(ic);

  Component* previous = hub_->GetComponentByStringID(i->second);
  return (IsInputMethod(previous) && ValidateInputMethod(previous, ic)) ?
      previous : GetNextInputMethodInList(ic);
}

Component* HubInputMethodManager::GetNextInputMethodInList(
    const InputContext* ic) const {
  if (all_input_methods_.empty())
    return NULL;

  Component* current = GetCurrentInputMethod(ic);
  uint32 current_id = current ? current->id() : kComponentDefault;

  std::vector<uint32>::const_iterator current_iter = std::upper_bound(
      all_input_methods_.begin(), all_input_methods_.end(), current_id);

  std::vector<uint32>::const_iterator i = current_iter;
  for (; i != all_input_methods_.end(); ++i) {
    Component* next = hub_->GetComponent(*i);
    if (hub_->IsComponentValid(next) && ValidateInputMethod(next, ic))
      return next;
  }

  for (i = all_input_methods_.begin(); i != current_iter; ++i) {
    Component* next = hub_->GetComponent(*i);
    if (hub_->IsComponentValid(next) && ValidateInputMethod(next, ic))
      return next;
  }

  return NULL;
}

bool HubInputMethodManager::SwitchToInputMethod(InputContext* ic,
                                                Component* input_method) {
  DLOG(INFO) << "Switch input method of ic: " << ic->id() << " to:"
             << input_method->string_id();

  Component* current = GetCurrentInputMethod(ic);
  if (current == input_method)
    return true;

  CreateSwitchingData(ic, input_method);
  if (ic->id() != kInputContextNone && hub_->IsComponentValid(current) &&
      current->CanConsume(MSG_CANCEL_COMPOSITION)) {
    proto::Message* message = new proto::Message();
    message->set_type(MSG_CANCEL_COMPOSITION);
    message->set_source(self_->id());
    message->set_target(current->id());
    message->set_icid(ic->id());
    message->set_reply_mode(ipc::proto::Message::NEED_REPLY);
    current->connector()->Send(message);
    return true;
  } else {
    return SwitchToInputMethodAfterCancelComposition(ic, input_method);
  }
}

bool HubInputMethodManager::SwitchToInputMethodAfterCancelComposition(
    InputContext* ic,
    Component* input_method) {
  // The target input method may take a while to actually attach to the input
  // context and start to work, so we need to cache the input method related
  // messages until it is attached.
  uint32 icid = use_global_input_method_ ? kInputContextNone : ic->id();

  InputContext::AttachState attach_state = hub_->RequestAttachToInputContext(
      input_method, ic, InputContext::ACTIVE, false);
  bool success = (attach_state != InputContext::NOT_ATTACHED);
  if (!success)
    DeleteSwitchingData(icid, false);

  // If |use_global_input_method_| is true, then we need to switch the input
  // method of all input contexts.
  if (success && use_global_input_method_) {
    const std::set<uint32>& all = self_->attached_input_contexts();
    std::set<uint32>::const_iterator i = all.begin();
    for (; i != all.end(); ++i) {
      InputContext* other = hub_->GetInputContext(*i);
      if (other == ic)
        continue;
      if (ValidateInputMethod(input_method, other)) {
        hub_->RequestAttachToInputContext(
            input_method, other, InputContext::ACTIVE, false);
      }
    }
  }
  return success;
}

bool HubInputMethodManager::SwitchToNextInputMethodInList(InputContext* ic) {
  Component* input_method = GetNextInputMethodInList(ic);
  return input_method && SwitchToInputMethod(ic, input_method);
}

bool HubInputMethodManager::SwitchToPreviousInputMethod(InputContext* ic) {
  Component* input_method = GetPreviousInputMethod(ic);
  return input_method && SwitchToInputMethod(ic, input_method);
}

bool HubInputMethodManager::IsInputMethod(Component* component) const {
  return hub_->IsComponentValid(component) &&
      component->CanConsume(MSG_ATTACH_TO_INPUT_CONTEXT) &&
      component->CanConsume(MSG_PROCESS_KEY_EVENT) &&
      component->CanConsume(MSG_CANCEL_COMPOSITION) &&
      component->CanConsume(MSG_COMPLETE_COMPOSITION);
}

bool HubInputMethodManager::ValidateInputMethod(Component* input_method,
                                                const InputContext* ic) const {
  // TODO(suzhe): validate detailed information of the input context, such as
  // allowed character set, etc.
  return true;
}

void HubInputMethodManager::CreateSwitchingData(InputContext* ic,
                                                Component* new_input_method) {
  DCHECK(switching_data_.find(ic->id()) == switching_data_.end());
  InputContext::AttachState state =
      ic->GetComponentAttachState(new_input_method);
  switching_data_[ic->id()] = new InputMethodSwitchingData(
      hub_,
      this,
      ic,
      new_input_method->id(),
      state != InputContext::ACTIVE && state != InputContext::ACTIVE_STICKY);
}

void HubInputMethodManager::UpdateSwitchingData(uint32 icid,
                                                uint32 component_id,
                                                int state_mask) {
  InputMethodSwitchingDataMap::iterator it = switching_data_.find(icid);
  if (it == switching_data_.end())
    return;
  HubInputMethodManager::InputMethodSwitchingData* data = it->second;
  if (data &&
      data->CheckAndUpdateState(component_id, state_mask)) {
    DeleteSwitchingData(icid, false);
  }
}

void HubInputMethodManager::DeleteSwitchingData(uint32 icid,
                                                bool discard_cache) {
  InputMethodSwitchingDataMap::iterator it = switching_data_.find(icid);
  if (it == switching_data_.end())
    return;
  HubInputMethodManager::InputMethodSwitchingData* data = it->second;
  switching_data_.erase(icid);
  if (data) {
    if (discard_cache)
      data->DiscardCachedMessages();
    delete data;
  }
}

}  // namespace hub
}  // namespace ipc
