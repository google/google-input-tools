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

#ifndef GOOPY_IPC_HUB_IMPL_TEST_BASE_H_
#define GOOPY_IPC_HUB_IMPL_TEST_BASE_H_

#include <map>

#include "base/basictypes.h"
#include "ipc/hub.h"
#include "ipc/testing.h"

namespace ipc {

class MockConnector;

namespace proto {
class ComponentInfo;
}  // namespace proto

namespace hub {

class Component;
class HubImpl;
class InputContext;

class HubImplTestBase : public ::testing::Test {
 protected:
  HubImplTestBase();
  virtual ~HubImplTestBase();

  // Overridden from ::testing::Test:
  virtual void SetUp();
  virtual void TearDown();

  bool IsConnectorAttached(Hub::Connector* connector) const;
  Component* GetComponent(uint32 id) const;
  InputContext* GetInputContext(uint32 icid) const;
  uint32 GetFocusedInputContextID() const;
  void VerifyDefaultComponent() const;
  void VerifyDefaultInputContext() const;
  void VerifyComponent(const proto::ComponentInfo& info) const;
  void CreateInputContext(MockConnector* connector,
                          uint32 app_id,
                          uint32* icid);
  void QueryActiveConsumers(uint32 icid,
                            const uint32* messages,
                            uint32* consumers,
                            int size);
  void CheckActiveConsumers(uint32 icid,
                            const uint32* messages,
                            int messages_size,
                            const uint32* consumers,
                            int consumers_size);
  void ActivateComponent(uint32 icid, uint32 component_id);
  void CheckAndReplyMsgAttachToInputContext(MockConnector* connector,
                                            uint32 component_id,
                                            uint32 icid,
                                            bool active);
  void CheckMsgComponentActivated(MockConnector* connector,
                                  uint32 component_id,
                                  uint32 icid,
                                  const uint32* activated_messages,
                                  size_t activated_messages_size);
  void FocusOrBlurInputContext(MockConnector* connector,
                               uint32 component_id,
                               uint32 icid,
                               bool focus);
  void CheckFocusChangeMessages(MockConnector* connector,
                                uint32 component_id,
                                uint32 icid_lost_focus,
                                uint32 icid_got_focus);
  void RequestConsumers(MockConnector* connector,
                        uint32 component_id,
                        uint32 icid,
                        const uint32* messages,
                        size_t size);
  void CheckInputContext(uint32 query_icid, uint32 icid, uint32 owner);

  HubImpl* hub_;

  // first = message type, second = component id
  std::map<uint32, uint32> builtin_consumers_;
};

}  // namespace hub
}  // namespace ipc


#endif  // GOOPY_IPC_HUB_IMPL_TEST_BASE_H_
