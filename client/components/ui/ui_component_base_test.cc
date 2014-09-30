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

#include "components/ui/ui_component_base.h"

#include "base/logging.h"
#include "ipc/constants.h"
#include "ipc/message_types.h"
#include "ipc/mock_component_host.h"
#include "ipc/protos/ipc.pb.h"
#include "ipc/testing.h"
#include "ipc/test_util.h"

namespace {

class MockedUIComponent : public ime_goopy::components::UIComponentBase {
 public:
  MockedUIComponent()
    : selected_candidate_index_(0),
      selected_candidate_list_id_(0),
      candidate_list_ui_shown_(false),
      composition_ui_shown_(false),
      toolbar_ui_shown_(false) {
  }
  virtual bool Initialize() {
    return true;
  }

 protected:
  virtual std::string GetComponentStringID() OVERRIDE {
    return "com.google.ime.goopy.mockedui";
  }
  virtual std::string GetComponentName() OVERRIDE {
    return "Mocked UI";
  }
  virtual void SetComposition(
      const ipc::proto::Composition* composition) OVERRIDE {
    composition_.CopyFrom(*composition);
  }
  virtual void SetCandidateList(
      const ipc::proto::CandidateList* candidate_list) OVERRIDE {
    candidate_list_.CopyFrom(*candidate_list);
  }
  virtual void SetSelectedCandidate(int candidate_list_id,
                                    int candidate_index) OVERRIDE {
    selected_candidate_list_id_ = candidate_list_id;
    selected_candidate_index_ = candidate_index;
  }
  virtual void SetCompositionVisibility(bool show) OVERRIDE {
    composition_ui_shown_ = show;
  }
  virtual void SetCandidateListVisibility(bool show) OVERRIDE {
    candidate_list_ui_shown_ = show;
  }
  virtual void SetToolbarVisibility(bool show) OVERRIDE {
    toolbar_ui_shown_ = show;
  }
  virtual void SetCommandList(const CommandLists& command_lists) OVERRIDE {
  }
  virtual void ChangeCandidateListVisibility(int id, bool visible) OVERRIDE {
  }
  virtual void SetInputMethods(const ComponentInfos& components) OVERRIDE {
  }
  virtual void SetActiveInputMethod(
      const ipc::proto::ComponentInfo& component) OVERRIDE {
  }
  virtual void SetInputCaret(const ipc::proto::InputCaret& caret) OVERRIDE {
  }

 private:
  ipc::proto::CandidateList candidate_list_;
  ipc::proto::Composition composition_;
  int selected_candidate_list_id_;
  int selected_candidate_index_;
  bool candidate_list_ui_shown_;
  bool composition_ui_shown_;
  bool toolbar_ui_shown_;
  FRIEND_TEST(UIComponentBaseTest, ShowHideTest);
  FRIEND_TEST(UIComponentBaseTest, CandidateSelectTest);
};

class UIComponentBaseTest : public ::testing::Test {
 protected:
  UIComponentBaseTest() : icid_(ipc::kInputContextNone) {
  }
  virtual ~UIComponentBaseTest() { }
  virtual void SetUp() {
    icid_ = 1;
    host_.AddComponent(&ui_);
    scoped_ptr<ipc::proto::Message> mptr;
    // Focus the input context.
    mptr.reset(
        NewMessageForTest(ipc::MSG_INPUT_CONTEXT_GOT_FOCUS,
                          ipc::proto::Message::NO_REPLY,
                          ipc::kComponentDefault,
                          ipc::kComponentBroadcast,
                          icid_));
    ui_.Handle(mptr.release());
  }
  virtual void TearDown() {
    host_.RemoveComponent(&ui_);
  }

  MockedUIComponent ui_;
  ipc::MockComponentHost host_;
  uint32 icid_;
};

TEST_F(UIComponentBaseTest, ShowHideTest) {
  EXPECT_FALSE(ui_.candidate_list_ui_shown_);
  EXPECT_FALSE(ui_.composition_ui_shown_);
  EXPECT_FALSE(ui_.toolbar_ui_shown_);
  // Show candidate list.
  scoped_ptr<ipc::proto::Message> mptr(
      NewMessageForTest(ipc::MSG_SHOW_CANDIDATE_LIST_UI,
                        ipc::proto::Message::NO_REPLY,
                        ipc::kComponentDefault,
                        ipc::kComponentBroadcast,
                        icid_));
  ui_.Handle(mptr.release());
  EXPECT_TRUE(ui_.candidate_list_ui_shown_);

  // Hide candidate list.
  mptr.reset(
      NewMessageForTest(ipc::MSG_HIDE_CANDIDATE_LIST_UI,
                        ipc::proto::Message::NO_REPLY,
                        ipc::kComponentDefault,
                        ipc::kComponentBroadcast,
                        icid_));
  ui_.Handle(mptr.release());
  EXPECT_FALSE(ui_.candidate_list_ui_shown_);

  // Show composition.
  mptr.reset(
      NewMessageForTest(ipc::MSG_SHOW_COMPOSITION_UI,
                        ipc::proto::Message::NO_REPLY,
                        ipc::kComponentDefault,
                        ipc::kComponentBroadcast,
                        icid_));
  ui_.Handle(mptr.release());
  EXPECT_TRUE(ui_.composition_ui_shown_);

  // Hide composition.
  mptr.reset(
      NewMessageForTest(ipc::MSG_HIDE_COMPOSITION_UI,
                        ipc::proto::Message::NO_REPLY,
                        ipc::kComponentDefault,
                        ipc::kComponentBroadcast,
                        icid_));
  ui_.Handle(mptr.release());
  EXPECT_FALSE(ui_.composition_ui_shown_);

  // Show toolbar.
  mptr.reset(
      NewMessageForTest(ipc::MSG_SHOW_TOOLBAR_UI,
                        ipc::proto::Message::NO_REPLY,
                        ipc::kComponentDefault,
                        ipc::kComponentBroadcast,
                        icid_));
  ui_.Handle(mptr.release());
  EXPECT_TRUE(ui_.toolbar_ui_shown_);

  // Hide toolbar.
  mptr.reset(
      NewMessageForTest(ipc::MSG_HIDE_TOOLBAR_UI,
                        ipc::proto::Message::NO_REPLY,
                        ipc::kComponentDefault,
                        ipc::kComponentBroadcast,
                        icid_));
  ui_.Handle(mptr.release());
  EXPECT_FALSE(ui_.toolbar_ui_shown_);
}

TEST_F(UIComponentBaseTest, CandidateSelectTest) {
  // Set candidate list.
  ipc::proto::CandidateList candidate_list;
  candidate_list.set_id(1);
  candidate_list.add_candidate();
  candidate_list.add_candidate();
  scoped_ptr<ipc::proto::Message> mptr(
      NewMessageForTest(ipc::MSG_CANDIDATE_LIST_CHANGED,
                        ipc::proto::Message::NO_REPLY,
                        ipc::kComponentDefault,
                        ipc::kComponentBroadcast,
                        icid_));
  mptr->mutable_payload()->mutable_candidate_list()->CopyFrom(candidate_list);
  ui_.Handle(mptr.release());
  EXPECT_EQ(candidate_list.candidate_size(),
            ui_.candidate_list_.candidate_size());
  mptr.reset(
      NewMessageForTest(ipc::MSG_SELECTED_CANDIDATE_CHANGED,
                        ipc::proto::Message::NO_REPLY,
                        ipc::kComponentDefault,
                        ipc::kComponentBroadcast,
                        icid_));
  mptr->mutable_payload()->add_uint32(1);
  mptr->mutable_payload()->add_uint32(1);
  ui_.Handle(mptr.release());
  EXPECT_EQ(1, ui_.selected_candidate_list_id_);
  EXPECT_EQ(1, ui_.selected_candidate_index_);
}
}  // namespacge

int main(int argc, char* argv[]) {
#if defined(OS_WIN)
  testing::InitGoogleTest(&argc, argv);
  logging::InitLogging("", logging::LOG_TO_BOTH_FILE_AND_SYSTEM_DEBUG_LOG,
                       logging::DONT_LOCK_LOG_FILE,
                       logging::APPEND_TO_OLD_LOG_FILE);
#elif defined(OS_LINUX)
  testing::InitGoogleTest(&argc, argv);
#endif
  return RUN_ALL_TESTS();
}
