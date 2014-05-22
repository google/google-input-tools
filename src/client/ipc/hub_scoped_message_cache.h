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

#ifndef GOOPY_IPC_HUB_SCOPED_MESSAGE_CACHE_H_
#define GOOPY_IPC_HUB_SCOPED_MESSAGE_CACHE_H_
#pragma once

#include <queue>
#include <set>

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "ipc/hub.h"

namespace ipc {
namespace hub {
class HubImpl;
class Component;
// A temporary built-in component that will cache messages of certain types when
// the component is alive and re-send the message before it is destroyed.
// Please be aware that if a message have more than one consumer, when
// the HubScopedMessageCache object is destroyed, the active consumer of the
// messages will not guaranteed to be the previous active consumer.
class HubScopedMessageCache : public Hub::Connector {
 public:
  // Constructs a new HubScopedMessageCache object and starts caching messages.
  // params:
  //    |cache_message_types|: the message types that should be cached.
  //    |icid|: the id of the input context in which messages should be cached.
  //    |hub|: the pointer of the hub.
  HubScopedMessageCache(const uint32* cache_message_types,
                        int cache_message_type_count,
                        uint32 icid,
                        HubImpl* hub);
  // Deconstructs the object, stops caching messages and dispatches all cached
  // messages.
  virtual ~HubScopedMessageCache();
  // Overridden from Hub::Connector.
  virtual bool Send(proto::Message* message) OVERRIDE;

  void DiscardCachedMessages();

 private:
  typedef std::set<uint32> MessageTypeSet;
  typedef std::queue<proto::Message*> CachedMessageQueue;

  void Dispatch(proto::Message* message);

  HubImpl* hub_;
  uint32 icid_;
  MessageTypeSet messages_should_cached_;
  Component* self_;
  CachedMessageQueue cached_messages_;

  DISALLOW_COPY_AND_ASSIGN(HubScopedMessageCache);
};

}  // namespace hub
}  // namespace ipc
#endif  // GOOPY_IPC_HUB_SCOPED_MESSAGE_CACHE_H_
