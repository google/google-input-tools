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

#include "ipc/hub_hotkey_manager.h"

#include <set>

#include "base/logging.h"
#include "base/scoped_ptr.h"
#include "ipc/hub_hotkey_list.h"
#include "ipc/hub_impl.h"
#include "ipc/message_util.h"

namespace {

// Messages may be produced by this built-in component.
const uint32 kProduceMessages[] = {
  ipc::MSG_REQUEST_CONSUMER,
  ipc::MSG_PROCESS_KEY_EVENT,
  ipc::MSG_ACTIVE_HOTKEY_LIST_UPDATED,
};
const uint32 kProduceMessagesSize = arraysize(kProduceMessages);

// Messages can be consumed by this built-in component.
const uint32 kConsumeMessages[] = {
  ipc::MSG_INPUT_CONTEXT_GOT_FOCUS,
  ipc::MSG_ACTIVE_CONSUMER_CHANGED,
  ipc::MSG_ATTACH_TO_INPUT_CONTEXT,
  ipc::MSG_DETACHED_FROM_INPUT_CONTEXT,

  ipc::MSG_SEND_KEY_EVENT,
  ipc::MSG_ADD_HOTKEY_LIST,
  ipc::MSG_REMOVE_HOTKEY_LIST,
  ipc::MSG_CHECK_HOTKEY_CONFLICT,
  ipc::MSG_ACTIVATE_HOTKEY_LIST,
  ipc::MSG_DEACTIVATE_HOTKEY_LIST,
  ipc::MSG_QUERY_ACTIVE_HOTKEY_LIST,
};
const size_t kConsumeMessagesSize = arraysize(kConsumeMessages);

const uint32 kMessagesNeedConsumer[] = {
  ipc::MSG_PROCESS_KEY_EVENT,
};
const size_t kMessagesNeedConsumerSize = arraysize(kMessagesNeedConsumer);

const char kStringID[] =
  "com.google.ime.goopy.ipc.hub.hotkey-manager";

const char kName[] = "Goopy IPC Hub Hotkey Manager";

}  // namespace

namespace ipc {
namespace hub {

HubHotkeyManager::HubHotkeyManager(HubImpl* hub)
    : self_(0),
      hub_(hub),
      message_serial_(0) {
  hub->Attach(this);

  proto::ComponentInfo info;
  info.set_string_id(kStringID);
  info.set_name(kName);

  for(size_t i = 0; i < arraysize(kProduceMessages); ++i)
    info.add_produce_message(kProduceMessages[i]);

  for(size_t i = 0; i < arraysize(kConsumeMessages); ++i)
    info.add_consume_message(kConsumeMessages[i]);

  Component* component = hub->CreateComponent(this, info, true);
  DCHECK(component);

  self_ = component->id();
}

HubHotkeyManager::~HubHotkeyManager() {
  hub_->Detach(this);
}

bool HubHotkeyManager::Send(proto::Message* message) {
  Component* source = hub_->GetComponent(message->source());
  DCHECK(source);
  switch (message->type()) {
    case MSG_INPUT_CONTEXT_GOT_FOCUS:
      return OnMsgInputContextGotFocus(source, message);
    case MSG_ACTIVE_CONSUMER_CHANGED:
      return OnMsgActiveConsumerChanged(source, message);
    case MSG_ATTACH_TO_INPUT_CONTEXT:
      return OnMsgAttachToInputContext(source, message);
    case MSG_DETACHED_FROM_INPUT_CONTEXT:
      return OnMsgDetachedFromInputContext(source, message);
    case MSG_SEND_KEY_EVENT:
      return OnMsgSendKeyEvent(source, message);
    case MSG_ADD_HOTKEY_LIST:
      return OnMsgAddHotkeyList(source, message);
    case MSG_REMOVE_HOTKEY_LIST:
      return OnMsgRemoveHotkeyList(source, message);
    case MSG_CHECK_HOTKEY_CONFLICT:
      return OnMsgCheckHotkeyConflict(source, message);
    case MSG_ACTIVATE_HOTKEY_LIST:
      return OnMsgActivateHotkeyList(source, message);
    case MSG_DEACTIVATE_HOTKEY_LIST:
      return OnMsgDeactivateHotkeyList(source, message);
    case MSG_QUERY_ACTIVE_HOTKEY_LIST:
      return OnMsgQueryActiveHotkeyList(source, message);
    case MSG_PROCESS_KEY_EVENT:
      if (message->reply_mode() == Message::IS_REPLY)
        return OnMsgProcessKeyEventReply(source, message);
      break;
    default:
      break;
  }
  DLOG(ERROR) << "Unexpected message:" << GetMessageName(message->type());
  delete message;
  return false;
}

bool HubHotkeyManager::OnMsgInputContextGotFocus(Component* source,
                                                 Message* message) {
  // Make sure the |message| will be deleted.
  scoped_ptr<Message> mptr(message);

  // We need to clear the previous key event of the input context, when it gets
  // focused, to make sure hotkeys can be matched correctly.
  // And we need to clear the default input context when any input context gets
  // the focus, to make sure a global hotkey won't be triggered when the input
  // focus is moving from one input context to another.
  uint32 icid = message->icid();
  InputContextData* data = GetInputContextData(icid, false);
  if (data)
    data->previous_key_event.Clear();

  if (icid != kInputContextNone) {
    data = GetInputContextData(kInputContextNone, false);
    if (data)
      data->previous_key_event.Clear();
  }
  return true;
}

bool HubHotkeyManager::OnMsgActiveConsumerChanged(Component* source,
                                                  Message* message) {
  // Make sure the |message| will be deleted.
  scoped_ptr<Message> mptr(message);

  uint32 icid = message->icid();
  if (icid == kInputContextNone)
    return true;

  // If an input context's active consumer of MSG_PROCESS_KEY_EVENT message is
  // changed, which means it has been switched to a new input method, then we
  // need to discard all pending key events of the input context. Because the
  // new input method doesn't know anything about those pending key events.
  const proto::MessagePayload& payload = message->payload();
  const int size = payload.uint32_size();
  for (int i = 0; i < size; ++i) {
    if (payload.uint32(i) == MSG_PROCESS_KEY_EVENT) {
      DiscardAllPendingKeyEvents(icid);
      break;
    }
  }
  return true;
}

bool HubHotkeyManager::OnMsgAttachToInputContext(Component* source,
                                                 Message* message) {
  uint32 icid = message->icid();

  // |message| will be deleted in HubImpl::ReplyTrue() method.
  hub_->ReplyTrue(source->connector(), message);

  // Request an input method component to help us consume MSG_PROCESS_KEY_EVENT
  // events.
  if (icid != kInputContextNone) {
    message = NewMessage(
        MSG_REQUEST_CONSUMER, Message::NO_REPLY, kComponentDefault, icid);
    message->mutable_payload()->add_uint32(MSG_PROCESS_KEY_EVENT);
    hub_->Dispatch(this, message);
  }

  return true;
}

bool HubHotkeyManager::OnMsgDetachedFromInputContext(Component* source,
                                                     Message* message) {
  // Make sure the |message| will be deleted.
  scoped_ptr<Message> mptr(message);

  // We no longer provide service to this input context, so just delete all
  // related data.
  DeleteInputContextData(message->icid());
  return true;
}

bool HubHotkeyManager::OnMsgSendKeyEvent(Component* source, Message* message) {
  Connector* connector = source->connector();
  InputContext* ic = hub_->GetInputContext(message->icid());
  if (!ic) {
    return hub_->ReplyError(
        connector, message, proto::Error::INVALID_INPUT_CONTEXT);
  }
  if (!ic->IsComponentReallyAttached(source)) {
    return hub_->ReplyError(
        connector, message, proto::Error::COMPONENT_NOT_ATTACHED);
  }
  if (!message->has_payload() || !message->payload().has_key_event())
    return hub_->ReplyError(connector, message, proto::Error::INVALID_PAYLOAD);

  const proto::KeyEvent& key = message->payload().key_event();

  InputContextData* data = GetInputContextData(ic->id(), true);

  // If the key event matches a hotkey, then we just reply it immediately
  // without sending it to the input method component for processing.
  if (MatchHotkey(ic, data, key))
    return hub_->ReplyTrue(connector, message);

  // Don't bother sending the key event to an input method if it's sent to the
  // default input context.
  if (ic->id() == kInputContextNone)
    return hub_->ReplyFalse(connector, message);

  // If the key event doesn't match any hotkey, then we need to send it to the
  // input method for processing and reply it until we get reply from the input
  // method component.
  const uint32 original_serial = message->serial();

  // Reuse the incoming message object, so that we can retain all message
  // payload and avoid allocating memory again.
  message->set_type(MSG_PROCESS_KEY_EVENT);
  message->set_source(self_);
  message->set_target(kComponentDefault);
  // |message|'s original icid could be kInputContextFocused, so we need to
  // replace it with the real one.
  message->set_icid(ic->id());
  // We can't rely on the original serial number to identify the new message.
  message->set_serial(message_serial_++);
  message->set_reply_mode(Message::NEED_REPLY);

  // Save necessary information for constructing reply message later.
  PendingKeyEvent* pending = &(data->pending_key_events[message->serial()]);
  pending->app_id = source->id();
  pending->serial = original_serial;

  DLOG(INFO) << "Pending Key Event: original_serial:" << original_serial
             << " app_id:" << source->id()
             << " new_serial:" << message->serial()
             << " icid:" << ic->id();

  // Dispatch the key event to the input method component.
  hub_->Dispatch(this, message);
  return true;
}

bool HubHotkeyManager::OnMsgAddHotkeyList(Component* source, Message* message) {
  if (!message->has_payload() || !message->payload().hotkey_list_size()) {
    return hub_->ReplyError(
        source->connector(), message, proto::Error::INVALID_PAYLOAD);
  }

  const int size = message->payload().hotkey_list_size();
  for (int i = 0; i < size; ++i) {
    const proto::HotkeyList& hotkey_list = message->payload().hotkey_list(i);
    source->AddHotkeyList(hotkey_list);

    const uint32 hotkey_list_id = hotkey_list.id();
    std::set<uint32>::const_iterator ic_iter =
        source->attached_input_contexts().begin();
    std::set<uint32>::const_iterator ic_end =
        source->attached_input_contexts().end();
    for (; ic_iter != ic_end; ++ic_iter) {
      InputContext* ic = hub_->GetInputContext(*ic_iter);
      DCHECK(ic);
      ic->ComponentHotkeyListUpdated(source, hotkey_list_id);
    }
  }

  return hub_->ReplyTrue(source->connector(), message);
}

bool HubHotkeyManager::OnMsgRemoveHotkeyList(Component* source,
                                             Message* message) {
  if (!message->has_payload() || !message->payload().uint32_size()) {
    return hub_->ReplyError(
        source->connector(), message, proto::Error::INVALID_PAYLOAD);
  }

  const int size = message->payload().uint32_size();
  for (int i = 0; i < size; ++i) {
    const uint32 hotkey_list_id = message->payload().uint32(i);
    source->RemoveHotkeyList(hotkey_list_id);

    std::set<uint32>::const_iterator ic_iter =
        source->attached_input_contexts().begin();
    std::set<uint32>::const_iterator ic_end =
        source->attached_input_contexts().end();
    for (; ic_iter != ic_end; ++ic_iter) {
      InputContext* ic = hub_->GetInputContext(*ic_iter);
      DCHECK(ic);
      ic->ComponentHotkeyListRemoved(source, hotkey_list_id);
    }
  }

  return hub_->ReplyTrue(source->connector(), message);
}

bool HubHotkeyManager::OnMsgCheckHotkeyConflict(Component* source,
                                                Message* message) {
  // TODO(suzhe)
  return hub_->ReplyError(
      source->connector(), message, proto::Error::NOT_IMPLEMENTED);
}

bool HubHotkeyManager::OnMsgActivateHotkeyList(Component* source,
                                               Message* message) {
  Connector* connector = source->connector();
  InputContext* ic = hub_->GetInputContext(message->icid());
  if (!ic) {
    return hub_->ReplyError(
        connector, message, proto::Error::INVALID_INPUT_CONTEXT);
  }
  if (!ic->IsComponentReallyAttached(source)) {
    return hub_->ReplyError(
        connector, message, proto::Error::COMPONENT_NOT_ATTACHED);
  }
  if (!message->has_payload() || !message->payload().uint32_size())
    return hub_->ReplyError(connector, message, proto::Error::INVALID_PAYLOAD);

  uint32 hotkey_list_id = message->payload().uint32(0);
  if (!source->GetHotkeyList(hotkey_list_id))
    return hub_->ReplyError(connector, message, proto::Error::INVALID_PAYLOAD);

  ic->SetComponentActiveHotkeyList(source, hotkey_list_id);
  return hub_->ReplyTrue(connector, message);
}

bool HubHotkeyManager::OnMsgDeactivateHotkeyList(Component* source,
                                                 Message* message) {
  Connector* connector = source->connector();
  InputContext* ic = hub_->GetInputContext(message->icid());
  if (!ic) {
    return hub_->ReplyError(
        connector, message, proto::Error::INVALID_INPUT_CONTEXT);
  }
  if (!ic->IsComponentReallyAttached(source)) {
    return hub_->ReplyError(
        connector, message, proto::Error::COMPONENT_NOT_ATTACHED);
  }

  ic->UnsetComponentActiveHotkeyList(source);
  return hub_->ReplyTrue(connector, message);
}

bool HubHotkeyManager::OnMsgQueryActiveHotkeyList(Component* source,
                                                  Message* message) {
  Connector* connector = source->connector();
  if (message->reply_mode() != Message::NEED_REPLY) {
    return hub_->ReplyError(
        connector, message, proto::Error::INVALID_REPLY_MODE);
  }

  InputContext* ic = hub_->GetInputContext(message->icid());
  if (!ic) {
    return hub_->ReplyError(
        connector, message, proto::Error::INVALID_INPUT_CONTEXT);
  }

  ConvertToReplyMessage(message);
  const std::vector<const HotkeyList*>& hotkey_lists =
      ic->GetAllActiveHotkeyLists();

  proto::MessagePayload* payload = message->mutable_payload();
  payload->Clear();

  const size_t size = hotkey_lists.size();
  for (size_t i = 0; i < size; ++i)
    payload->add_hotkey_list()->CopyFrom(hotkey_lists[i]->hotkeys());

  hub_->Dispatch(this, message);
  return true;
}

bool HubHotkeyManager::OnMsgProcessKeyEventReply(Component* source,
                                                 Message* message) {
  DLOG(INFO) << "ProcessKeyEventReply: serial:" << message->serial()
             << " ime_id:" << message->source()
             << " icid:" << message->icid();

  Connector* connector = source->connector();
  InputContext* ic = hub_->GetInputContext(message->icid());
  if (!ic) {
    return hub_->ReplyError(
        connector, message, proto::Error::INVALID_INPUT_CONTEXT);
  }

  InputContextData* data = GetInputContextData(ic->id(), false);
  if (!data) {
    return hub_->ReplyError(
        connector, message, proto::Error::INVALID_INPUT_CONTEXT);
  }

  PendingKeyEventMap::iterator pending_iter =
      data->pending_key_events.find(message->serial());
  if (pending_iter == data->pending_key_events.end())
    return hub_->ReplyError(connector, message, proto::Error::INVALID_MESSAGE);

  // Constructs a reply message to the application component by reusing
  // |message| object, so that we can keep the payload.
  message->set_type(MSG_SEND_KEY_EVENT);
  message->set_source(self_);
  message->set_target(pending_iter->second.app_id);
  // |message|'s original icid could be kInputContextFocused, so we need to
  // replace it with the real one.
  message->set_icid(ic->id());
  message->set_serial(pending_iter->second.serial);

  // Remove the pending key event.
  data->pending_key_events.erase(pending_iter);

  // Dispatch the reply message back to the application component.
  hub_->Dispatch(this, message);
  return true;
}

HubHotkeyManager::InputContextData* HubHotkeyManager::GetInputContextData(
    uint32 icid, bool create_new) {
  InputContextDataMap::iterator i = input_context_data_.find(icid);
  if (i != input_context_data_.end())
    return &(i->second);

  if (create_new)
    return &(input_context_data_[icid]);

  return NULL;
}

proto::Message* HubHotkeyManager::NewMessage(
    uint32 type,
    proto::Message::ReplyMode reply_mode,
    uint32 target,
    uint32 icid) {
  Message* message = new proto::Message();
  message->set_type(type);
  message->set_reply_mode(reply_mode);
  message->set_source(self_);
  message->set_target(target);
  message->set_icid(icid);
  if (reply_mode != proto::Message::IS_REPLY)
    message->set_serial(message_serial_++);
  return message;
}

void HubHotkeyManager::ReplyPendingKeyEvent(
      uint32 app_id, uint32 icid, uint32 serial, bool result) {
  Message* message = NewMessage(
      MSG_SEND_KEY_EVENT, Message::IS_REPLY, app_id, icid);
  message->set_serial(serial);
  message->mutable_payload()->add_boolean(result);
  hub_->Dispatch(this, message);
}

void HubHotkeyManager::DiscardAllPendingKeyEvents(uint32 icid) {
  InputContextData* data = GetInputContextData(icid, false);
  if (!data)
    return;

  PendingKeyEventMap::iterator i = data->pending_key_events.begin();
  PendingKeyEventMap::iterator end = data->pending_key_events.end();
  for (; i != end; ++i)
    ReplyPendingKeyEvent(i->second.app_id, icid, i->second.serial, false);

  data->pending_key_events.clear();
}

void HubHotkeyManager::DeleteInputContextData(uint32 icid) {
  DiscardAllPendingKeyEvents(icid);
  input_context_data_.erase(icid);
}

bool HubHotkeyManager::MatchHotkey(InputContext* input_context,
                                   InputContextData* data,
                                   const proto::KeyEvent& key) {
  bool matched = MatchHotkeyInHotkeyLists(
      input_context->GetAllActiveHotkeyLists(), data->previous_key_event, key);

  data->previous_key_event = key;

  if (input_context->id() == kInputContextNone)
    return matched;

  InputContext* global_input_context = hub_->GetInputContext(kInputContextNone);
  InputContextData* global_data = GetInputContextData(kInputContextNone, true);
  if (!matched) {
    matched = MatchHotkeyInHotkeyLists(
        global_input_context->GetAllActiveHotkeyLists(),
        global_data->previous_key_event, key);
  }
  global_data->previous_key_event = key;
  return matched;
}

bool HubHotkeyManager::MatchHotkeyInHotkeyLists(
    const std::vector<const HotkeyList*>& hotkey_lists,
    const proto::KeyEvent& previous_key,
    const proto::KeyEvent& current_key) {
  const size_t size = hotkey_lists.size();
  for (size_t i = 0; i < size; ++i) {
    const proto::Hotkey* hotkey =
        hotkey_lists[i]->Match(previous_key, current_key);
    if (hotkey) {
      DispatchHotkeyMessages(hotkey_lists[i]->owner(), *hotkey);
      return true;
      // Do we need to match against all hotkey lists and dispatch all matched
      // hotkeys?
    }
  }
  return false;
}

void HubHotkeyManager::DispatchHotkeyMessages(uint32 owner_id,
                                              const proto::Hotkey& hotkey) {
  Component* owner = hub_->GetComponent(owner_id);
  DCHECK(owner);

  const int size = hotkey.message_size();
  for (int i = 0; i < size; ++i) {
#if !defined(NDEBUG)
    std::string text;
    PrintMessageToString(hotkey.message(i), &text, false);
    DLOG(INFO) << "Dispatch Hotkey Message:\n" << text;
#endif

    proto::Message* copy = new proto::Message();
    copy->CopyFrom(hotkey.message(i));
    hub_->Dispatch(owner->connector(), copy);
  }
}

}  // namespace hub
}  // namespace ipc
