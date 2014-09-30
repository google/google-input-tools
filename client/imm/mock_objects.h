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

#ifndef GOOPY_IMM_MOCK_OBJECTS_H__
#define GOOPY_IMM_MOCK_OBJECTS_H__

#include <atlbase.h>
#include <atltypes.h>

#include "base/basictypes.h"
#include "common/framework_interface.h"
#include "core/core_interface.h"
#include "imm/immdev.h"
#include <gtest/gunit.h>

namespace ime_goopy {
namespace imm {
class MockIMCPolicy {
 public:
  MockIMCPolicy() : ref_count_(0) {
    ZeroMemory(&input_context_, sizeof(input_context_));
  }

  inline LPINPUTCONTEXT LockIMC(HIMC himc) {
    ref_count_++;
    return &input_context_;
  }

  inline BOOL UnlockIMC(HIMC himc) {
    ref_count_--;
    return TRUE;
  }

  INPUTCONTEXT* input_context() { return &input_context_; }

 private:
  int ref_count_;
  INPUTCONTEXT input_context_;
};

class MockImmLockPolicy {
 public:
  static const int kMaxComponentSize = 10240;
  struct Component {
    int ref;
    int size;
    uint8 buffer[kMaxComponentSize];
  };

  static LPINPUTCONTEXT ImmLockIMC(HIMC himc);
  static BOOL ImmUnlockIMC(HIMC himc);
  static LPVOID ImmLockIMCC(HIMCC himcc);
  static BOOL ImmUnlockIMCC(HIMCC himcc);
  static HIMCC ImmCreateIMCC(DWORD size);
  static HIMCC ImmReSizeIMCC(HIMCC himcc, DWORD size);

  static void Reset();
  static HIMCC CreateComponent(int size);
  static void DestroyComponent(HIMCC himcc);

  static int input_context_ref_;
  static INPUTCONTEXT input_context_;
};

class MockMessageQueue {
 public:
  MockMessageQueue(HIMC himc);
  ~MockMessageQueue();
  void AddMessage(UINT message, WPARAM wparam, LPARAM lparam);
  bool Send();
  void Attach(LPTRANSMSGLIST transmsg);
  int Detach();
  void Reset();
  const vector<TRANSMSG>& messages() { return messages_; }
  bool attach_called() { return attach_called_; }
  bool detach_called() { return detach_called_; }

 private:
  vector<TRANSMSG> messages_;
  bool attach_called_;
  bool detach_called_;
  DISALLOW_EVIL_CONSTRUCTORS(MockMessageQueue);
};

class MockContext : public ContextInterface {
 public:
  static const CRect kTestRect;

  MockContext(HIMC himc, MockMessageQueue* message_queue);
  ~MockContext();

  bool GetCaretRectFromComposition(RECT* caret_rect);
  bool GetCaretRectFromCandidate(RECT* caret_rect);
  void OnSystemStatusChange() { }

  // ContextInterface
  void Update(UpdateAction action) {}
  Platform GetPlatform() { return PLATFORM_WINDOWS_UNKNOWN; }
  EngineInterface* GetEngine() { return engine_; }

  // Test
  void set_rect_from_composition(bool value) {
    rect_from_composition_ = value;
  }

  void set_rect_from_candidate(bool value) {
    rect_from_candidate_ = value;
  }

  void set_engine(EngineInterface* engine) { engine_ = engine; }

 private:
  static MockContext* test_context_;
  EngineInterface* engine_;
  bool rect_from_composition_;
  bool rect_from_candidate_;
};


}  // namespace imm
}  // namespace ime_goopy
#endif  // GOOPY_IMM_MOCK_OBJECTS_H__
