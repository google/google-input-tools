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

#include "common/mock_engine.h"
#include "core/core_interface.h"
#include "imm/context.h"
#include "imm/immdev.h"
#include "imm/mock_objects.h"
#include "imm/test_main.h"
#include <gtest/gunit.h>

namespace ime_goopy {
namespace imm {

typedef ContextT<MockImmLockPolicy, MockMessageQueue> TestContext;
static const HIMC kDummyHIMC = reinterpret_cast<HIMC>(1);

class ContextTest : public testing::Test {
 public:
  void SetUp() {
    // message_queue_ and engine_ will be deleted inside context_.
    message_queue_ = new MockMessageQueue(NULL);
    engine_ = new MockEngine;
    context_.reset(new TestContext(kDummyHIMC, message_queue_));
    context_->set_engine(engine_);
  }

 protected:
  MockMessageQueue* message_queue_;
  MockEngine* engine_;
  scoped_ptr<TestContext> context_;
};

TEST_F(ContextTest, Update) {
  MockImmLockPolicy::Reset();
  {
    HIMCCLockerT<CompositionString, MockImmLockPolicy> compstr(
        &MockImmLockPolicy::input_context_.hCompStr);
    EXPECT_TRUE(compstr->Initialize());
    HIMCCLockerT<CandidateInfo, MockImmLockPolicy> candinfo(
        &MockImmLockPolicy::input_context_.hCandInfo);
    EXPECT_TRUE(candinfo->Initialize());

    context_->Update(ContextInterface::COMPOSITION_START);
    EXPECT_EQ(1, message_queue_->messages().size());
    EXPECT_EQ(WM_IME_STARTCOMPOSITION, message_queue_->messages()[0].message);
    EXPECT_EQ(0, message_queue_->messages()[0].wParam);
    EXPECT_EQ(0, message_queue_->messages()[0].lParam);

    message_queue_->Reset();
    context_->Update(ContextInterface::COMPOSITION_UPDATE);
    EXPECT_EQ(1, message_queue_->messages().size());
    EXPECT_EQ(WM_IME_COMPOSITION, message_queue_->messages()[0].message);
    EXPECT_EQ(0, message_queue_->messages()[0].wParam);
    EXPECT_EQ(GCS_COMP | GCS_COMPREAD | GCS_CURSORPOS,
              message_queue_->messages()[0].lParam);
    EXPECT_STREQ(MockEngine::kTestComposition, compstr->composition);
    EXPECT_EQ(MockEngine::kTestCompositionLength, compstr->info.dwCompStrLen);
    EXPECT_EQ(MockEngine::kTestCaret, compstr->info.dwCursorPos);
    EXPECT_EQ(0, compstr->info.dwResultStrLen);

    message_queue_->Reset();
    context_->Update(ContextInterface::COMPOSITION_COMMIT);
    EXPECT_EQ(2, message_queue_->messages().size());
    EXPECT_EQ(WM_IME_COMPOSITION, message_queue_->messages()[0].message);
    EXPECT_EQ(0, message_queue_->messages()[0].wParam);
    EXPECT_EQ(GCS_RESULTSTR | GCS_CURSORPOS,
              message_queue_->messages()[0].lParam);
    EXPECT_EQ(WM_IME_ENDCOMPOSITION, message_queue_->messages()[1].message);
    EXPECT_EQ(0, message_queue_->messages()[1].wParam);
    EXPECT_EQ(0, message_queue_->messages()[1].lParam);
    EXPECT_STREQ(L"", compstr->composition);
    EXPECT_EQ(0, compstr->info.dwCompStrLen);
    EXPECT_EQ(MockEngine::kTestResultLength, compstr->info.dwCursorPos);
    EXPECT_STREQ(MockEngine::kTestResult, compstr->result);
    EXPECT_EQ(MockEngine::kTestResultLength, compstr->info.dwResultStrLen);

    message_queue_->Reset();
    context_->Update(ContextInterface::COMPOSITION_CANCEL);
    EXPECT_EQ(2, message_queue_->messages().size());
    EXPECT_EQ(WM_IME_COMPOSITION, message_queue_->messages()[0].message);
    EXPECT_EQ(0, message_queue_->messages()[0].wParam);
    EXPECT_EQ(GCS_COMP | GCS_COMPREAD | GCS_CURSORPOS,
              message_queue_->messages()[0].lParam);
    EXPECT_EQ(WM_IME_ENDCOMPOSITION, message_queue_->messages()[1].message);
    EXPECT_EQ(0, message_queue_->messages()[1].wParam);
    EXPECT_EQ(0, message_queue_->messages()[1].lParam);
    EXPECT_STREQ(L"", compstr->composition);
    EXPECT_EQ(0, compstr->info.dwCompStrLen);
    EXPECT_EQ(0, compstr->info.dwCursorPos);
    EXPECT_STREQ(L"", compstr->result);
    EXPECT_EQ(0, compstr->info.dwResultStrLen);

    message_queue_->Reset();
    context_->Update(ContextInterface::CANDIDATES_SHOW);
    EXPECT_EQ(1, message_queue_->messages().size());
    EXPECT_EQ(WM_IME_NOTIFY, message_queue_->messages()[0].message);
    EXPECT_EQ(IMN_OPENCANDIDATE, message_queue_->messages()[0].wParam);
    EXPECT_EQ(1, message_queue_->messages()[0].lParam);

    message_queue_->Reset();
    context_->Update(ContextInterface::CANDIDATES_UPDATE);
    EXPECT_EQ(1, message_queue_->messages().size());
    EXPECT_EQ(WM_IME_NOTIFY, message_queue_->messages()[0].message);
    EXPECT_EQ(IMN_CHANGECANDIDATE, message_queue_->messages()[0].wParam);
    EXPECT_EQ(1, message_queue_->messages()[0].lParam);
    EXPECT_EQ(2, candinfo->list.info.dwCount);
    EXPECT_STREQ(MockEngine::kTestCandidate1, candinfo->list.text[0]);
    EXPECT_STREQ(MockEngine::kTestCandidate2, candinfo->list.text[1]);

    message_queue_->Reset();
    context_->Update(ContextInterface::CANDIDATES_HIDE);
    EXPECT_EQ(1, message_queue_->messages().size());
    EXPECT_EQ(WM_IME_NOTIFY, message_queue_->messages()[0].message);
    EXPECT_EQ(IMN_CLOSECANDIDATE, message_queue_->messages()[0].wParam);
    EXPECT_EQ(1, message_queue_->messages()[0].lParam);
    EXPECT_EQ(0, candinfo->list.info.dwCount);
  }

  MockImmLockPolicy::DestroyComponent(
    MockImmLockPolicy::input_context_.hCompStr);
  MockImmLockPolicy::DestroyComponent(
    MockImmLockPolicy::input_context_.hCandInfo);
}

TEST_F(ContextTest, OnProcessKey) {
  uint8 key_state[256];
  EXPECT_TRUE(context_->OnProcessKey(0, 0, key_state));
  EXPECT_EQ(1, engine_->commands().size());
  EXPECT_TRUE(engine_->commands()[0]->is<ShouldProcessKeyCommand>());
}

TEST_F(ContextTest, OnNotifyIME) {
  engine_->Reset();
  EXPECT_TRUE(context_->OnNotifyIME(NI_COMPOSITIONSTR, CPS_COMPLETE, 0));
  EXPECT_EQ(1, engine_->commands().size());
  EXPECT_TRUE(engine_->commands()[0]->is<CommitCommand>());

  engine_->Reset();
  EXPECT_TRUE(context_->OnNotifyIME(NI_COMPOSITIONSTR, CPS_CANCEL, 0));
  EXPECT_EQ(1, engine_->commands().size());
  EXPECT_TRUE(engine_->commands()[0]->is<CancelCommand>());
}

TEST_F(ContextTest, OnToAsciiEx) {
  engine_->Reset();
  uint8 key_state[256];
  TRANSMSGLIST list;
  EXPECT_EQ(0, context_->OnToAsciiEx(0, 0, key_state, &list, 0));
  EXPECT_EQ(1, engine_->commands().size());
  EXPECT_TRUE(engine_->commands()[0]->is<ProcessKeyCommand>());
  EXPECT_TRUE(message_queue_->attach_called());
  EXPECT_TRUE(message_queue_->detach_called());
}
}  // namespace imm
}  // namespace ime_goopy
