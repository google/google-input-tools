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

#include <string>
#include "base/basictypes.h"
#include "common/framework_interface.h"
#include "common/mock_engine.h"
#include "imm/context.h"
#include "imm/context_manager.h"
#include "imm/debug.h"
#include "imm/mock_objects.h"
#include "imm/immdev.h"
#include <gtest/gunit.h>

namespace ime_goopy {
const wchar_t InputMethod::kUIClassName[] = L"GPY2_TEST_UI";
const wchar_t InputMethod::kDisplayName[] = L"Goopy2 TEST";
const DWORD InputMethod::kConversionModeMask = IME_CMODE_FULLSHAPE |
                                               IME_CMODE_NATIVE |
                                               IME_CMODE_SYMBOL;
const DWORD InputMethod::kSentenceModeMask = IME_SMODE_NONE;

static bool _show_configure_window_called = false;

bool InputMethod::ShowConfigureWindow(HWND parent) {
  _show_configure_window_called = true;
  EXPECT_TRUE(NULL == parent);
  return true;
}

EngineInterface* InputMethod::CreateEngine(ContextInterface* context) {
  return new MockEngine();
}

UIManagerInterface* InputMethod::CreateUIManager(HWND parent) {
  return NULL;
}
namespace imm {
static const HIMC kDummyHIMC = reinterpret_cast<HIMC>(1);

class ExportTest : public testing::Test {
 public:
  void SetUp() {
    ContextManager::Instance().DestroyAll();
    message_queue_ = new MessageQueue(kDummyHIMC);
    engine_ = new MockEngine;
    context_ = new Context(kDummyHIMC, message_queue_);
    context_->set_engine(engine_);
    ContextManager::Instance().Add(kDummyHIMC, context_);
  }

  void TearDown() {
    ContextManager::Instance().DestroyAll();
  }

 protected:
  Context* context_;
  MockEngine* engine_;
  MessageQueue* message_queue_;
};

TEST_F(ExportTest, ImeInquire) {
  IMEINFO ime_info;
  wchar_t class_name[MAX_PATH];
  ImeInquire(&ime_info, class_name, 0);

  EXPECT_STREQ(InputMethod::kUIClassName, class_name);
  EXPECT_EQ(0, ime_info.dwPrivateDataSize);
}

TEST_F(ExportTest, ImeConfigure) {
  _show_configure_window_called = false;
  ImeConfigure(NULL, NULL, 0, NULL);
  EXPECT_TRUE(_show_configure_window_called);
}

TEST_F(ExportTest, ImeDestroy) {
  EXPECT_TRUE(ImeDestroy(0));
  EXPECT_FALSE(ImeDestroy(1));
}

TEST_F(ExportTest, ImeEscape) {
  wchar_t name[MAX_PATH];
  ImeEscape(NULL, IME_ESC_IME_NAME, name);
  EXPECT_STREQ(InputMethod::kDisplayName, name);
}

TEST_F(ExportTest, ImeProcessKey) {
  uint8 key_state[256];
  EXPECT_TRUE(ImeProcessKey(kDummyHIMC, 0, 0, key_state));
  EXPECT_EQ(1, engine_->commands().size());
  EXPECT_TRUE(engine_->commands()[0]->is<ShouldProcessKeyCommand>());
}

TEST_F(ExportTest, ImeSelect) {
  ContextManager& context_manager = ContextManager::Instance();
  context_manager.DestroyAll();
  EXPECT_FALSE(context_manager.Get(kDummyHIMC));

  ImeSelect(kDummyHIMC, TRUE);
  EXPECT_TRUE(context_manager.Get(kDummyHIMC));

  ImeSelect(kDummyHIMC, FALSE);
  EXPECT_FALSE(context_manager.Get(kDummyHIMC));
  context_manager.DestroyAll();
}

TEST_F(ExportTest, ImeToAsciiEx) {
  uint8 key_state[256];
  TRANSMSGLIST transmsglist;
  EXPECT_EQ(0, ImeToAsciiEx(0, 0, key_state, &transmsglist, 0, kDummyHIMC));
  EXPECT_EQ(1, engine_->commands().size());
  EXPECT_TRUE(engine_->commands()[0]->is<ProcessKeyCommand>());
}

TEST_F(ExportTest, NotifyIME) {
  EXPECT_TRUE(NotifyIME(kDummyHIMC, NI_COMPOSITIONSTR, CPS_COMPLETE, 0));
  EXPECT_EQ(1, engine_->commands().size());
  EXPECT_TRUE(engine_->commands()[0]->is<CommitCommand>());
}
}  // namespace imm
}  // namespace ime_goopy
