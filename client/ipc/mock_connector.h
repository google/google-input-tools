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

#ifndef GOOPY_IPC_MOCK_CONNECTOR_H_
#define GOOPY_IPC_MOCK_CONNECTOR_H_

#include <map>
#include <vector>

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "ipc/hub.h"

namespace ipc {

namespace proto {
class ComponentInfo;
}  // namespace proto

// A mock Hub::Connector implementation for testing Hub and its built-in
// components.
class MockConnector : public Hub::Connector {
 public:
  MockConnector();
  virtual ~MockConnector();

  // Implementation of Hub::Connector;
  virtual bool Send(proto::Message* message) OVERRIDE;

  // Adds a fake component to this mock connector.
  void AddComponent(const proto::ComponentInfo& info);

  // Removes a fake component by its index.
  void RemoveComponentByIndex(size_t index);

  // Removes a fake component by its component id.
  void RemoveComponent(uint32 id);

  // Attaches this mock connector to a hub. All added components will be
  // registered to the hub automatically.
  void Attach(Hub* hub);

  // Detaches this mock connector from the hub.
  void Detach();

  // Clears all messages stored in |messages_|.
  void ClearMessages();

  Hub* hub_;

  // Components owned by this Connector.
  std::vector<proto::ComponentInfo> components_;

  // Component ID -> index in above |components_| array.
  std::map<uint32, size_t> components_index_;

  // Messages received from the hub.
  std::vector<proto::Message*> messages_;

  // The result that Send() method should return.
  bool send_result_;
};

}  // namespace ipc

#endif  // GOOPY_IPC_MOCK_CONNECTOR_H_
