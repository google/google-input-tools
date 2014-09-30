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

#ifndef GOOPY_COMPONENTS_COMMON_MOCK_APP_COMPONENT_H_
#define GOOPY_COMPONENTS_COMMON_MOCK_APP_COMPONENT_H_

#include <string>
#include <vector>

#include "base/basictypes.h"
#include "base/logging.h"
#include "ipc/component_base.h"
#include "ipc/protos/ipc.pb.h"

namespace ipc {
namespace proto {
class KeyEvent;
}  // namespace proto
}  // namespace ipc

namespace ime_goopy {
namespace components {

// Typist simulates user action in a composition scenario.
class Typist {
 public:
  class Delegate {
   public:
    virtual ~Delegate() {
    }

    virtual void SwitchToKeyboardInput() = 0;
    virtual void FocusInputContext() = 0;
    // Called to simulate composition.
    virtual void UserComposite() = 0;
    virtual void HandleKey(Typist* typist, uint32 keycode) = 0;
    virtual void HandleKeyEvent(Typist* typist,
                                const ipc::proto::KeyEvent& key_event) = 0;
    virtual void CompleteComposition(Typist* typist) = 0;
    virtual void CancelComposition(Typist* typist, bool commit) = 0;
    virtual void SelectCandidate(
        Typist* typist, uint32 candidate_index, bool commit) = 0;
    virtual void CandidateListPageDown(Typist* typist) = 0;
    virtual void CandidateListPageUp(Typist* typist) = 0;
    virtual void CandidateListPageResize(Typist* typist, int size) = 0;
    virtual void set_typist(Typist* typist) = 0;
  };

  virtual ~Typist() {
  }

  // Called by test program to start compositon.
  virtual void StartComposite() = 0;
  // Called by delegate to generate a sequence of ipc messages.
  virtual void Composite() = 0;
  // Called to block until all expected messages received.
  virtual void WaitComplete() = 0;
  // Notifies typist of converted composition string.
  virtual void OnMessageReceived(ipc::proto::Message* msg) = 0;
  // Notifies typist of receiving replied messages.
  virtual void OnMessageReplyReceived(ipc::proto::Message* msg) {
    delete msg;
  }
  // Checks if all messages received are correct and in order. not necessary if
  // it's done during OnMessageReceived.
  virtual void CheckResult() = 0;
};

class MockAppComponent : public ipc::ComponentBase,
                         public Typist::Delegate {
 public:
  // Implemented by unittest from main thread to receive notifications.
  class Listener {
   public:
    virtual ~Listener() {
    }
    virtual void OnRegistered() = 0;
    // Context change callbacks.
    virtual void OnAppComponentReady() = 0;
    virtual void OnAppComponentStopped() = 0;
  };

  explicit MockAppComponent(const std::string& comp_id);
  virtual ~MockAppComponent();

  // Overriden from Interface ipc::Component:
  virtual void GetInfo(ipc::proto::ComponentInfo* info) OVERRIDE;
  virtual void Handle(ipc::proto::Message* message) OVERRIDE;

  // Overriden from ipc::ComponentBase:
  virtual void OnRegistered() OVERRIDE;

  // Called to simulate starting composition.
  void Start();
  // Called to simulate finished/interrupted composition.
  void Stop();

  // Overriden from Typist::Delegate:
  virtual void HandleKey(Typist* typist, uint32 keycode) OVERRIDE;
  virtual void HandleKeyEvent(Typist* typist,
                              const ipc::proto::KeyEvent& key_event) OVERRIDE;
  virtual void CompleteComposition(Typist* typist) OVERRIDE;
  virtual void CancelComposition(Typist* typist, bool commit) OVERRIDE;
  virtual void SelectCandidate(
      Typist* typist, uint32 candidate_index, bool commit) OVERRIDE;
  virtual void CandidateListPageDown(Typist* typist) OVERRIDE;
  virtual void CandidateListPageUp(Typist* typist) OVERRIDE;
  virtual void CandidateListPageResize(Typist* typist, int size) OVERRIDE;
  virtual void SwitchToKeyboardInput() OVERRIDE;
  virtual void FocusInputContext() OVERRIDE;
  virtual void UserComposite() OVERRIDE;
  virtual void set_typist(Typist* typist) OVERRIDE;

  void set_listener(Listener* listener) { listener_ = listener; }

 private:
  void CreateInputContextInternal();
  void RequestConsumerInternal();

  uint32 icid_;
  bool ready_;
  Listener* listener_;
  std::string comp_id_;
  Typist* typist_;
  DISALLOW_COPY_AND_ASSIGN(MockAppComponent);
};

}  // namespace components
}  // namespace ime_goopy

#endif  // GOOPY_COMPONENTS_COMMON_MOCK_APP_COMPONENT_H_
