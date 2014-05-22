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

#include "ipc/hub_scoped_message_cache.h"

#include "base/logging.h"
#include "base/scoped_ptr.h"
#include "base/stringprintf.h"
#include "ipc/hub_impl.h"
#include "ipc/message_util.h"

namespace ipc {
namespace hub {

static const char kComponentStringID[] = "ScopedMessageCache";
HubScopedMessageCache::HubScopedMessageCache(
    const uint32* cache_message_types,
    int cache_message_type_count,
    uint32 icid,
    HubImpl* hub)
    : hub_(hub),
      icid_(icid),
      self_(NULL) {
  DCHECK(hub);
  DCHECK(cache_message_types);
  DCHECK(cache_message_type_count);
  hub->Attach(this);
  proto::ComponentInfo info;
  std::string name = StringPrintf("%s_%x", kComponentStringID, this);
  for (size_t i = 0; i < cache_message_type_count; ++i) {
    info.add_consume_message(cache_message_types[i]);
    messages_should_cached_.insert(cache_message_types[i]);
  }
  info.set_string_id(name);
  info.add_consume_message(MSG_INPUT_CONTEXT_DELETED);
  self_ = hub_->CreateComponent(this, info, false);
  DCHECK(self_);
  // Attach to the input context.
  bool success = hub_->AttachToInputContext(
      self_, hub->GetInputContext(icid), InputContext::ACTIVE_STICKY, true);
  DCHECK(success);
}

HubScopedMessageCache::~HubScopedMessageCache() {
  hub_->DeleteComponent(this, self_->id());
  hub_->Detach(this);
  while (cached_messages_.size()) {
    Dispatch(cached_messages_.front());
    cached_messages_.pop();
  }
}

bool HubScopedMessageCache::Send(proto::Message* message) {
  if (message->type() == MSG_INPUT_CONTEXT_DELETED) {
    if (!message->has_payload() || !message->payload().uint32_size()) {
      hub_->ReplyError(this, message, ipc::proto::Error::INVALID_MESSAGE);
      return false;
    }
    if (message->payload().uint32(0) == icid_)
      DiscardCachedMessages();
    delete message;
    return true;
  }
  DCHECK(messages_should_cached_.find(message->type()) !=
         messages_should_cached_.end());
  cached_messages_.push(message);
  DLOG(INFO) << "Caching Message type = " << GetMessageName(message->type());
  return true;
}

void HubScopedMessageCache::DiscardCachedMessages() {
  while (!cached_messages_.empty()) {
    delete cached_messages_.front();
    cached_messages_.pop();
  }
}

void HubScopedMessageCache::Dispatch(proto::Message* message) {
  message->set_target(kComponentDefault);
  Component* source = hub_->GetComponent(message->source());
  // If source does not exist anymore, just delete the message.
  if (!source) {
    delete message;
    return;
  }
  hub_->Dispatch(source->connector(), message);
}

}  // namespace hub
}  // namespace ipc
