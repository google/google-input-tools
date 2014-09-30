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

#include <atlbase.h>
#include <atltypes.h>
#include <string>

#include "base/basictypes.h"
#include "common/framework_interface.h"
#include "common/mock_engine.h"
#include "imm/context_manager.h"
#include "imm/mock_objects.h"
#include "imm/ui_window.h"
#include <gtest/gunit.h>

namespace ime_goopy {
namespace imm {
class MockUIManager : public UIManagerInterface {
 public:
  MockUIManager() {
    Reset();
  }

  struct UpdateAction {
    UIType type;
    UICommand command;
  };

  // UIManagerInterface
  void SetEngine(EngineInterface* engine) { engine_ = engine; }
  void UpdateCaretRect(const RECT& caret_rect) { caret_rect_ = caret_rect; }
  void UpdateUI(UIType type, UICommand command) {
    UpdateAction action = {type, command};
    update_actions_.push_back(action);
  }

  const UpdateAction& actions(int index) { return update_actions_[index]; }
  int actions_count() { return static_cast<int>(update_actions_.size()); }
  void Reset() {
    ZeroMemory(&caret_rect_, sizeof(caret_rect_));
    update_actions_.clear();
    engine_ = NULL;
  }
  const CRect caret_rect() { return caret_rect_; }
  const EngineInterface* engine() { return engine_; }

 private:
  EngineInterface* engine_;
  CRect caret_rect_;
  vector<UpdateAction> update_actions_;
};

MockContext* MockContext::test_context_ = NULL;

// Just make hwnd a valid value.
static const HWND kDummyHWND = reinterpret_cast<HWND>(1);
typedef ContextManagerT<MockContext> TestContextManager;

MockContext* ContextManagerT<MockContext>::GetFromWindow(HWND hwnd) {
  return Get(0);
}

class UIWindowTest : public testing::Test {
 public:
  UIWindowTest() : ui_window_(kDummyHWND, &ui_manager_) {
  }

  void SetUp() {
    context_manager_ = &TestContextManager::Instance();
    context_ = new MockContext(0, new MockMessageQueue(0));
    EXPECT_TRUE(context_manager_->Add(0, context_));
    handled_ = FALSE;
    engine_.reset(new MockEngine());
    context_->set_engine(engine_.get());
  }

  void TearDown() {
    context_manager_->DestroyAll();
    context_ = NULL;
  }

 protected:
  MockUIManager ui_manager_;
  UIWindowT<MockContext> ui_window_;
  MockContext* context_;
  scoped_ptr<MockEngine> engine_;
  BOOL handled_;
  TestContextManager* context_manager_;
};

TEST_F(UIWindowTest, OnStartComposition) {
  // Can't get caret rect from composition.
  ui_manager_.Reset();
  context_->set_rect_from_composition(false);
  ui_window_.OnStartComposition(0, 0, 0, handled_);
  EXPECT_EQ(0, ui_manager_.actions_count());

  // Got rect from composition.
  context_->set_rect_from_composition(true);
  ui_manager_.Reset();
  ui_window_.OnStartComposition(0, 0, 0, handled_);
  EXPECT_EQ(MockContext::kTestRect, ui_manager_.caret_rect());
  EXPECT_EQ(1, ui_manager_.actions_count());
  EXPECT_EQ(UIManagerInterface::TYPE_COMPOSITION, ui_manager_.actions(0).type);
  EXPECT_EQ(UIManagerInterface::COMMAND_SHOW, ui_manager_.actions(0).command);

  // No context.
  context_manager_->DestroyAll();
  ui_manager_.Reset();
  ui_window_.OnStartComposition(0, 0, 0, handled_);
  EXPECT_EQ(0, ui_manager_.actions_count());
}

TEST_F(UIWindowTest, OnComposition) {
  // Composition not started.
  ui_window_.OnComposition(0, 0, 0, handled_);
  EXPECT_EQ(0, ui_manager_.actions_count());

  // Composition started.
  context_->set_rect_from_composition(true);
  ui_window_.OnStartComposition(0, 0, 0, handled_);
  ui_manager_.Reset();
  ui_window_.OnComposition(0, 0, 0, handled_);
  EXPECT_EQ(1, ui_manager_.actions_count());
  EXPECT_EQ(UIManagerInterface::TYPE_COMPOSITION, ui_manager_.actions(0).type);
  EXPECT_EQ(UIManagerInterface::COMMAND_UPDATE, ui_manager_.actions(0).command);
}

TEST_F(UIWindowTest, OnEndComposition) {
  // Composition not started.
  ui_window_.OnEndComposition(0, 0, 0, handled_);
  EXPECT_EQ(0, ui_manager_.actions_count());

  // Composition started.
  context_->set_rect_from_composition(true);
  ui_window_.OnStartComposition(0, 0, 0, handled_);
  ui_manager_.Reset();
  ui_window_.OnEndComposition(0, 0, 0, handled_);
  EXPECT_EQ(1, ui_manager_.actions_count());
  EXPECT_EQ(UIManagerInterface::TYPE_COMPOSITION, ui_manager_.actions(0).type);
  EXPECT_EQ(UIManagerInterface::COMMAND_HIDE, ui_manager_.actions(0).command);
}

TEST_F(UIWindowTest, StatusWindow) {
  ui_manager_.Reset();
  ui_window_.OnNotify(0, IMN_CLOSESTATUSWINDOW, 0, handled_);
  EXPECT_EQ(1, ui_manager_.actions_count());
  EXPECT_EQ(UIManagerInterface::TYPE_STATUS, ui_manager_.actions(0).type);
  EXPECT_EQ(UIManagerInterface::COMMAND_HIDE, ui_manager_.actions(0).command);

  ui_manager_.Reset();
  ui_window_.OnNotify(0, IMN_OPENSTATUSWINDOW, 0, handled_);
  EXPECT_EQ(1, ui_manager_.actions_count());
  EXPECT_EQ(UIManagerInterface::TYPE_STATUS, ui_manager_.actions(0).type);
  EXPECT_EQ(UIManagerInterface::COMMAND_SHOW, ui_manager_.actions(0).command);
}

TEST_F(UIWindowTest, OpenCandidate) {
  // Can't get caret rect from candidate.
  ui_manager_.Reset();
  context_->set_rect_from_candidate(false);
  ui_window_.OnNotify(0, IMN_OPENCANDIDATE, 0, handled_);
  EXPECT_EQ(0, ui_manager_.actions_count());

  // Got rect from composition.
  ui_manager_.Reset();
  context_->set_rect_from_candidate(true);
  ui_window_.OnNotify(0, IMN_OPENCANDIDATE, 0, handled_);
  EXPECT_EQ(MockContext::kTestRect, ui_manager_.caret_rect());
  EXPECT_EQ(1, ui_manager_.actions_count());
  EXPECT_EQ(UIManagerInterface::TYPE_CANDIDATES, ui_manager_.actions(0).type);
  EXPECT_EQ(UIManagerInterface::COMMAND_SHOW, ui_manager_.actions(0).command);

  // No context.
  ui_manager_.Reset();
  context_manager_->DestroyAll();
  ui_window_.OnNotify(0, IMN_OPENCANDIDATE, 0, handled_);
  EXPECT_EQ(0, ui_manager_.actions_count());
}

TEST_F(UIWindowTest, ChangeCandidate) {
  // Candidate not opened.
  ui_window_.OnNotify(0, IMN_CHANGECANDIDATE, 0, handled_);
  EXPECT_EQ(1, ui_manager_.actions_count());
  EXPECT_EQ(UIManagerInterface::TYPE_CANDIDATES, ui_manager_.actions(0).type);
  EXPECT_EQ(UIManagerInterface::COMMAND_UPDATE, ui_manager_.actions(0).command);

  // Composition started.
  context_->set_rect_from_candidate(true);
  ui_window_.OnNotify(0, IMN_OPENCANDIDATE, 0, handled_);
  ui_manager_.Reset();
  ui_window_.OnNotify(0, IMN_CHANGECANDIDATE, 0, handled_);
  EXPECT_EQ(1, ui_manager_.actions_count());
  EXPECT_EQ(UIManagerInterface::TYPE_CANDIDATES, ui_manager_.actions(0).type);
  EXPECT_EQ(UIManagerInterface::COMMAND_UPDATE, ui_manager_.actions(0).command);
}

TEST_F(UIWindowTest, CloseCandidate) {
  // Composition not started.
  ui_window_.OnNotify(0, IMN_CLOSECANDIDATE, 0, handled_);
  EXPECT_EQ(0, ui_manager_.actions_count());

  // Composition started.
  context_->set_rect_from_candidate(true);
  ui_window_.OnNotify(0, IMN_OPENCANDIDATE, 0, handled_);
  ui_manager_.Reset();
  ui_window_.OnNotify(0, IMN_CLOSECANDIDATE, 0, handled_);
  EXPECT_EQ(1, ui_manager_.actions_count());
  EXPECT_EQ(UIManagerInterface::TYPE_CANDIDATES, ui_manager_.actions(0).type);
  EXPECT_EQ(UIManagerInterface::COMMAND_HIDE, ui_manager_.actions(0).command);
}

TEST_F(UIWindowTest, CompositionPosition) {
  // Can't get caret rect from composition.
  ui_window_.OnNotify(0, IMN_SETCOMPOSITIONWINDOW, 0, handled_);
  EXPECT_NE(MockContext::kTestRect, ui_manager_.caret_rect());

  // Got caret rect.
  context_->set_rect_from_composition(true);
  ui_window_.OnNotify(0, IMN_SETCOMPOSITIONWINDOW, 0, handled_);
  EXPECT_EQ(MockContext::kTestRect, ui_manager_.caret_rect());
}

TEST_F(UIWindowTest, CandidatePosition) {
  // Can't get caret rect from candidates.
  ui_window_.OnNotify(0, IMN_SETCANDIDATEPOS, 0, handled_);
  EXPECT_NE(MockContext::kTestRect, ui_manager_.caret_rect());

  // Got caret rect.
  context_->set_rect_from_candidate(true);
  ui_window_.OnNotify(0, IMN_SETCANDIDATEPOS, 0, handled_);
  EXPECT_EQ(MockContext::kTestRect, ui_manager_.caret_rect());
}

TEST_F(UIWindowTest, OnSetContext) {
  // No opened window.
  ui_window_.OnSetContext(0, TRUE, ISC_SHOWUIALL, handled_);
  EXPECT_EQ(engine_.get(), ui_manager_.engine());

  // Composition and candidate opened.
  ui_manager_.Reset();
  context_->set_rect_from_composition(true);
  ui_window_.OnStartComposition(0, 0, 0, handled_);
  context_->set_rect_from_candidate(true);
  ui_window_.OnNotify(0, IMN_OPENCANDIDATE, 0, handled_);
  ui_manager_.Reset();
  ui_window_.OnSetContext(0, TRUE, ISC_SHOWUIALL, handled_);
  EXPECT_EQ(engine_.get(), ui_manager_.engine());
  EXPECT_EQ(2, ui_manager_.actions_count());
  EXPECT_EQ(UIManagerInterface::TYPE_COMPOSITION, ui_manager_.actions(0).type);
  EXPECT_EQ(UIManagerInterface::COMMAND_SHOW, ui_manager_.actions(0).command);
  EXPECT_EQ(UIManagerInterface::TYPE_CANDIDATES, ui_manager_.actions(1).type);
  EXPECT_EQ(UIManagerInterface::COMMAND_SHOW, ui_manager_.actions(1).command);

  // wparam is FALSE.
  ui_manager_.Reset();
  ui_window_.OnSetContext(0, FALSE, ISC_SHOWUIALL, handled_);
  EXPECT_TRUE(NULL == ui_manager_.engine());
  EXPECT_EQ(2, ui_manager_.actions_count());
  EXPECT_EQ(UIManagerInterface::TYPE_CANDIDATES, ui_manager_.actions(0).type);
  EXPECT_EQ(UIManagerInterface::COMMAND_HIDE, ui_manager_.actions(0).command);
  EXPECT_EQ(UIManagerInterface::TYPE_COMPOSITION, ui_manager_.actions(1).type);
  EXPECT_EQ(UIManagerInterface::COMMAND_HIDE, ui_manager_.actions(1).command);

  // Context is NULL.
  context_manager_->DestroyAll();
  ui_manager_.Reset();
  ui_window_.OnSetContext(0, TRUE, ISC_SHOWUIALL, handled_);
  EXPECT_TRUE(NULL == ui_manager_.engine());
  EXPECT_EQ(2, ui_manager_.actions_count());
  EXPECT_EQ(UIManagerInterface::TYPE_CANDIDATES, ui_manager_.actions(0).type);
  EXPECT_EQ(UIManagerInterface::COMMAND_HIDE, ui_manager_.actions(0).command);
  EXPECT_EQ(UIManagerInterface::TYPE_COMPOSITION, ui_manager_.actions(1).type);
  EXPECT_EQ(UIManagerInterface::COMMAND_HIDE, ui_manager_.actions(1).command);
}

TEST_F(UIWindowTest, OnSelect) {
  // wparam: TRUE, context: not NULL.
  ui_window_.OnSelect(0, TRUE, 0, handled_);
  EXPECT_EQ(engine_.get(), ui_manager_.engine());

  // wparam: FALSE, context: not NULL.
  ui_manager_.Reset();
  ui_window_.OnSelect(0, FALSE, 0, handled_);
  EXPECT_TRUE(NULL == ui_manager_.engine());

  context_manager_->DestroyAll();

  // wparam: TRUE, context: NULL.
  ui_manager_.Reset();
  ui_window_.OnSelect(0, TRUE, 0, handled_);
  EXPECT_TRUE(NULL == ui_manager_.engine());

  // wparam: TRUE, context: NULL.
  ui_manager_.Reset();
  ui_window_.OnSelect(0, TRUE, 0, handled_);
  EXPECT_TRUE(NULL == ui_manager_.engine());
}
}  // namespace imm
}  // namespace ime_goopy
