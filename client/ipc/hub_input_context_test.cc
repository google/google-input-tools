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

#include "ipc/hub_input_context.h"

#include <algorithm>

#include "base/scoped_ptr.h"
#include "ipc/constants.h"
#include "ipc/hub_component.h"
#include "ipc/message_types.h"
#include "ipc/test_util.h"
#include "ipc/testing.h"

namespace {

using ipc::hub::Component;
using ipc::hub::InputContext;
using ipc::CreateTestComponent;

// Messages Hub can produce
const uint32 kHubProduceMessages[] = {
  ipc::MSG_ATTACH_TO_INPUT_CONTEXT,
  ipc::MSG_DETACHED_FROM_INPUT_CONTEXT,
  ipc::MSG_COMPOSITION_CHANGED,
  ipc::MSG_CANDIDATE_LIST_CHANGED,
  ipc::MSG_SELECTED_CANDIDATE_CHANGED,
  ipc::MSG_CANDIDATE_LIST_VISIBILITY_CHANGED,
};

// Messages Hub can consume
const uint32 kHubConsumeMessages[] = {
  ipc::MSG_REGISTER_COMPONENT,
  ipc::MSG_DEREGISTER_COMPONENT,
  ipc::MSG_QUERY_COMPONENT,
  ipc::MSG_CREATE_INPUT_CONTEXT,
  ipc::MSG_DELETE_INPUT_CONTEXT,
  ipc::MSG_QUERY_INPUT_CONTEXT,
  ipc::MSG_FOCUS_INPUT_CONTEXT,
  ipc::MSG_BLUR_INPUT_CONTEXT,
  ipc::MSG_PROCESS_KEY_EVENT,
  ipc::MSG_UPDATE_INPUT_CARET,
  ipc::MSG_SET_COMMAND_LIST,
  ipc::MSG_UPDATE_COMMANDS,
  ipc::MSG_QUERY_COMMAND_LIST,
  ipc::MSG_ADD_HOTKEY_LIST,
  ipc::MSG_REMOVE_HOTKEY_LIST,
  ipc::MSG_CHECK_HOTKEY_CONFLICT,
  ipc::MSG_ACTIVATE_HOTKEY_LIST,
  ipc::MSG_DEACTIVATE_HOTKEY_LIST,
  ipc::MSG_SET_COMPOSITION,
  ipc::MSG_QUERY_COMPOSITION,
  ipc::MSG_SET_CANDIDATE_LIST,
  ipc::MSG_SET_SELECTED_CANDIDATE,
  ipc::MSG_SET_CANDIDATE_LIST_VISIBILITY,
  ipc::MSG_QUERY_CANDIDATE_LIST,
};

// Messages an application can produce
const uint32 kAppProduceMessages[] = {
  ipc::MSG_REGISTER_COMPONENT,
  ipc::MSG_DEREGISTER_COMPONENT,
  ipc::MSG_CREATE_INPUT_CONTEXT,
  ipc::MSG_DELETE_INPUT_CONTEXT,
  ipc::MSG_FOCUS_INPUT_CONTEXT,
  ipc::MSG_BLUR_INPUT_CONTEXT,
  ipc::MSG_PROCESS_KEY_EVENT,
  ipc::MSG_CANCEL_COMPOSITION,
  ipc::MSG_COMPLETE_COMPOSITION,
  ipc::MSG_UPDATE_INPUT_CARET,
  ipc::MSG_SHOW_COMPOSITION_UI,
  ipc::MSG_HIDE_COMPOSITION_UI,
  ipc::MSG_SHOW_CANDIDATE_LIST_UI,
  ipc::MSG_HIDE_CANDIDATE_LIST_UI,
};

// Messages an application can consume
const uint32 kAppConsumeMessages[] = {
  ipc::MSG_COMPOSITION_CHANGED,
  ipc::MSG_INSERT_TEXT,
  ipc::MSG_GET_DOCUMENT_INFO,
  ipc::MSG_GET_DOCUMENT_CONTENT_IN_RANGE,
};

// Messages an input method can produce
const uint32 kIMEProduceMessages[] = {
  ipc::MSG_REGISTER_COMPONENT,
  ipc::MSG_DEREGISTER_COMPONENT,
  ipc::MSG_QUERY_INPUT_CONTEXT,
  ipc::MSG_SET_COMPOSITION,
  ipc::MSG_INSERT_TEXT,
  ipc::MSG_SET_CANDIDATE_LIST,
  ipc::MSG_SET_SELECTED_CANDIDATE,
  ipc::MSG_SET_CANDIDATE_LIST_VISIBILITY,
  ipc::MSG_SET_COMMAND_LIST,
  ipc::MSG_UPDATE_COMMANDS,
  ipc::MSG_ADD_HOTKEY_LIST,
  ipc::MSG_REMOVE_HOTKEY_LIST,
  ipc::MSG_CHECK_HOTKEY_CONFLICT,
  ipc::MSG_ACTIVATE_HOTKEY_LIST,
  ipc::MSG_DEACTIVATE_HOTKEY_LIST,
};

// Messages an input method can consume
const uint32 kIMEConsumeMessages[] = {
  ipc::MSG_ATTACH_TO_INPUT_CONTEXT,
  ipc::MSG_DETACHED_FROM_INPUT_CONTEXT,
  ipc::MSG_FOCUS_INPUT_CONTEXT,
  ipc::MSG_BLUR_INPUT_CONTEXT,
  ipc::MSG_PROCESS_KEY_EVENT,
  ipc::MSG_CANCEL_COMPOSITION,
  ipc::MSG_COMPLETE_COMPOSITION,
  ipc::MSG_CANDIDATE_LIST_SHOWN,
  ipc::MSG_CANDIDATE_LIST_HIDDEN,
  ipc::MSG_CANDIDATE_LIST_PAGE_DOWN,
  ipc::MSG_CANDIDATE_LIST_PAGE_UP,
  ipc::MSG_CANDIDATE_LIST_SCROLL_TO,
  ipc::MSG_CANDIDATE_LIST_PAGE_RESIZE,
  ipc::MSG_SELECT_CANDIDATE,
  ipc::MSG_UPDATE_INPUT_CARET,
};

// Messages a candidate window can produce
const uint32 kCandidateUIProduceMessages[] = {
  ipc::MSG_REGISTER_COMPONENT,
  ipc::MSG_DEREGISTER_COMPONENT,
  ipc::MSG_CANDIDATE_LIST_SHOWN,
  ipc::MSG_CANDIDATE_LIST_HIDDEN,
  ipc::MSG_CANDIDATE_LIST_PAGE_DOWN,
  ipc::MSG_CANDIDATE_LIST_PAGE_UP,
  ipc::MSG_CANDIDATE_LIST_SCROLL_TO,
  ipc::MSG_CANDIDATE_LIST_PAGE_RESIZE,
  ipc::MSG_SELECT_CANDIDATE,
};

// Messages a candidate window can consume
const uint32 kCandidateUIConsumeMessages[] = {
  ipc::MSG_ATTACH_TO_INPUT_CONTEXT,
  ipc::MSG_DETACHED_FROM_INPUT_CONTEXT,
  ipc::MSG_FOCUS_INPUT_CONTEXT,
  ipc::MSG_BLUR_INPUT_CONTEXT,
  ipc::MSG_CANDIDATE_LIST_CHANGED,
  ipc::MSG_SELECTED_CANDIDATE_CHANGED,
  ipc::MSG_CANDIDATE_LIST_VISIBILITY_CHANGED,
  ipc::MSG_UPDATE_INPUT_CARET,
  ipc::MSG_SHOW_CANDIDATE_LIST_UI,
  ipc::MSG_HIDE_CANDIDATE_LIST_UI,
};

// Messages an off-the-spot compose window can produce
const uint32 kComposeUIProduceMessages[] = {
  ipc::MSG_REGISTER_COMPONENT,
  ipc::MSG_DEREGISTER_COMPONENT,
};

// Messages an off-the-spot compose window can consume
const uint32 kComposeUIConsumeMessages[] = {
  ipc::MSG_ATTACH_TO_INPUT_CONTEXT,
  ipc::MSG_DETACHED_FROM_INPUT_CONTEXT,
  ipc::MSG_FOCUS_INPUT_CONTEXT,
  ipc::MSG_BLUR_INPUT_CONTEXT,
  ipc::MSG_COMPOSITION_CHANGED,
  ipc::MSG_UPDATE_INPUT_CARET,
  ipc::MSG_SHOW_COMPOSITION_UI,
  ipc::MSG_HIDE_COMPOSITION_UI,
};

class HubInputContextTest : public ::testing::Test,
                            public InputContext::Delegate {
 protected:
  HubInputContextTest()
      : input_context_(NULL) {
    hub_.reset(CreateTestComponent(0, NULL, "", "", "",
                                   kHubProduceMessages,
                                   arraysize(kHubProduceMessages),
                                   kHubConsumeMessages,
                                   arraysize(kHubConsumeMessages)));
    app1_.reset(CreateTestComponent(1, NULL, "", "", "",
                                    kAppProduceMessages,
                                    arraysize(kAppProduceMessages),
                                    kAppConsumeMessages,
                                    arraysize(kAppConsumeMessages)));
    app2_.reset(CreateTestComponent(2, NULL, "", "", "",
                                    kAppProduceMessages,
                                    arraysize(kAppProduceMessages),
                                    kAppConsumeMessages,
                                    arraysize(kAppConsumeMessages)));
    ime1_.reset(CreateTestComponent(3, NULL, "", "", "",
                                    kIMEProduceMessages,
                                    arraysize(kIMEProduceMessages),
                                    kIMEConsumeMessages,
                                    arraysize(kIMEConsumeMessages)));
    ime2_.reset(CreateTestComponent(4, NULL, "", "", "",
                                    kIMEProduceMessages,
                                    arraysize(kIMEProduceMessages),
                                    kIMEConsumeMessages,
                                    arraysize(kIMEConsumeMessages)));
    candidate_ui_.reset(
        CreateTestComponent(5, NULL, "", "", "",
                            kCandidateUIProduceMessages,
                            arraysize(kCandidateUIProduceMessages),
                            kCandidateUIConsumeMessages,
                            arraysize(kCandidateUIConsumeMessages)));
    compose_ui_.reset(
        CreateTestComponent(6, NULL, "", "", "",
                            kComposeUIProduceMessages,
                            arraysize(kComposeUIProduceMessages),
                            kComposeUIConsumeMessages,
                            arraysize(kComposeUIConsumeMessages)));
  }

  virtual ~HubInputContextTest() {
  }

  // ipc::hub::InputContext::Delegate implementation
  virtual void OnComponentActivated(
      InputContext* input_context,
      Component* component,
      const InputContext::MessageTypeVector& messages) {
    ASSERT_TRUE(!input_context_ || input_context_ == input_context);
    activated_.push_back(component);
    activated_messages_.push_back(messages);
  }

  virtual void OnComponentDeactivated(
      InputContext* input_context,
      Component* component,
      const InputContext::MessageTypeVector& messages) {
    ASSERT_EQ(input_context_, input_context);
    deactivated_.push_back(component);
    deactivated_messages_.push_back(messages);
  }

  virtual void OnComponentDetached(InputContext* input_context,
                                   Component* component,
                                   InputContext::AttachState state) {
    ASSERT_EQ(input_context_, input_context);
    detached_.push_back(component);
  }

  virtual void OnActiveConsumerChanged(
      InputContext* input_context,
      const InputContext::MessageTypeVector& messages) {
    ASSERT_TRUE(!input_context_ || input_context_ == input_context);
    active_consumer_changed_ = messages;
  }

  virtual void MaybeDetachComponent(InputContext* input_context,
                                    Component* component) {
    ASSERT_EQ(input_context_, input_context);
    maybe_detach_.push_back(component);
  }

  virtual void RequestConsumer(InputContext* input_context,
                               const InputContext::MessageTypeVector& messages,
                               Component* exclude) {
    ASSERT_EQ(input_context_, input_context);
    request_consumer_messages_ = messages;
    request_consumer_exclude_ = exclude;
  }

  void ResetDelegate() {
    activated_.clear();
    activated_messages_.clear();
    deactivated_.clear();
    deactivated_messages_.clear();
    detached_.clear();
    maybe_detach_.clear();
    request_consumer_messages_.clear();
    request_consumer_exclude_ = NULL;
    active_consumer_changed_.clear();
  }

  void CheckActiveConsumer(const InputContext* ic,
                           Component* consumer,
                           const uint32* messages,
                           size_t messages_size) {
    for(size_t i = 0; i < messages_size; ++i)
      ASSERT_EQ(consumer, ic->GetActiveConsumer(messages[i]));
  }

  void CheckProducer(const InputContext* ic,
                     const uint32* messages,
                     size_t messages_size) {
    for(size_t i = 0; i < messages_size; ++i)
      ASSERT_TRUE(ic->MayProduce(messages[i], false));
  }

  void CheckMessagesNeedConsumer(const InputContext* ic,
                                 const uint32* messages,
                                 size_t messages_size,
                                 bool include_pending) {
    std::vector<uint32> result;
    ASSERT_EQ(messages_size,
              ic->GetAllMessagesNeedConsumer(include_pending, &result));
    std::sort(result.begin(), result.end());
    for (size_t i = 0; i < messages_size; ++i) {
      ASSERT_TRUE(
          std::binary_search(result.begin(), result.end(), messages[i]));
    }
  }

  void CheckActivatedMessages(size_t index,
                              const uint32* messages,
                              size_t messages_size) {
    ASSERT_LT(index, activated_messages_.size());
    ASSERT_EQ(messages_size, activated_messages_[index].size());
    std::sort(activated_messages_[index].begin(),
              activated_messages_[index].end());
    for (size_t i = 0; i < messages_size; ++i) {
      ASSERT_TRUE(std::binary_search(activated_messages_[index].begin(),
                                     activated_messages_[index].end(),
                                     messages[i]));
    }
  }

  void CheckDeactivatedMessages(size_t index,
                                const uint32* messages,
                                size_t messages_size) {
    ASSERT_LT(index, deactivated_messages_.size());
    ASSERT_EQ(messages_size, deactivated_messages_[index].size());
    std::sort(deactivated_messages_[index].begin(),
              deactivated_messages_[index].end());
    for (size_t i = 0; i < messages_size; ++i) {
      ASSERT_TRUE(std::binary_search(deactivated_messages_[index].begin(),
                                     deactivated_messages_[index].end(),
                                     messages[i]));
    }
  }

  void CheckActiveConsumerChangedMessages(const uint32* messages,
                                          size_t messages_size) {
    ASSERT_EQ(messages_size, active_consumer_changed_.size());
    std::sort(active_consumer_changed_.begin(),
              active_consumer_changed_.end());
    for (size_t i = 0; i < messages_size; ++i) {
      ASSERT_TRUE(std::binary_search(active_consumer_changed_.begin(),
                                     active_consumer_changed_.end(),
                                     messages[i]));
    }
  }

  scoped_ptr<Component> hub_;
  scoped_ptr<Component> app1_;
  scoped_ptr<Component> app2_;
  scoped_ptr<Component> ime1_;
  scoped_ptr<Component> ime2_;
  scoped_ptr<Component> candidate_ui_;
  scoped_ptr<Component> compose_ui_;

  InputContext* input_context_;
  std::vector<Component*> activated_;
  std::vector<InputContext::MessageTypeVector> activated_messages_;
  std::vector<Component*> deactivated_;
  std::vector<InputContext::MessageTypeVector> deactivated_messages_;
  std::vector<Component*> detached_;
  std::vector<Component*> maybe_detach_;
  InputContext::MessageTypeVector request_consumer_messages_;
  Component* request_consumer_exclude_;
  InputContext::MessageTypeVector active_consumer_changed_;
};

// Test basic properties.
TEST_F(HubInputContextTest, Properties) {
  scoped_ptr<InputContext> ic(new InputContext(123, app1_.get(), this));

  EXPECT_EQ(123, ic->id());
  EXPECT_EQ(app1_.get(), ic->owner());

  EXPECT_TRUE(this == ic->delegate());

  ipc::proto::InputContextInfo info;
  ic->GetInfo(&info);

  EXPECT_EQ(123, info.id());
  EXPECT_EQ(app1_->id(), info.owner());
}

// Test owner related behavior.
TEST_F(HubInputContextTest, Owner) {
  scoped_ptr<InputContext> ic(new InputContext(123, app1_.get(), this));
  input_context_ = ic.get();

  ic.get()->SetMessagesNeedConsumer(
      app1_.get(), kAppProduceMessages, arraysize(kAppProduceMessages), NULL);

  // We should get OnComponentActivated() call for the owner.
  ASSERT_EQ(1, activated_.size());
  EXPECT_EQ(app1_.get(), activated_[0]);
  EXPECT_NO_FATAL_FAILURE(CheckActivatedMessages(
      0, kAppConsumeMessages, arraysize(kAppConsumeMessages)));
  EXPECT_NO_FATAL_FAILURE(CheckActiveConsumerChangedMessages(
      kAppConsumeMessages, arraysize(kAppConsumeMessages)));

  // We should get NeedConsumer() call.
  ASSERT_EQ(app1_.get(), request_consumer_exclude_);
  ASSERT_TRUE(request_consumer_messages_.size());
  ResetDelegate();

  // Owner should always be attached.
  EXPECT_EQ(InputContext::ACTIVE_STICKY,
            ic->GetComponentAttachState(app1_.get()));

  // Owner should always be persistent.
  EXPECT_TRUE(ic->IsComponentPersistent(app1_.get()));

  // It should not take effect.
  ic->SetComponentPersistent(app1_.get(), false);
  EXPECT_TRUE(ic->IsComponentPersistent(app1_.get()));

  // Owner should be activated by default.
  EXPECT_TRUE(ic->IsComponentActive(app1_.get()));

  // Owner can not be detached or deactivated.
  EXPECT_FALSE(ic->DetachComponent(app1_.get()));

  // Owner can not be attached with other states.
  EXPECT_TRUE(ic->AttachComponent(
      app1_.get(), InputContext::ACTIVE_STICKY, false));
  EXPECT_TRUE(ic->IsComponentPersistent(app1_.get()));

  EXPECT_FALSE(ic->AttachComponent(
      app1_.get(), InputContext::ACTIVE, true));
  EXPECT_FALSE(ic->AttachComponent(
      app1_.get(), InputContext::PASSIVE, true));
  EXPECT_FALSE(ic->AttachComponent(
      app1_.get(), InputContext::PENDING_PASSIVE, true));
  EXPECT_FALSE(ic->AttachComponent(
      app1_.get(), InputContext::PENDING_ACTIVE, true));
  EXPECT_FALSE(ic->AttachComponent(
      app1_.get(), InputContext::NOT_ATTACHED, true));

  EXPECT_NO_FATAL_FAILURE(CheckActiveConsumer(ic.get(), app1_.get(),
                                              kAppConsumeMessages,
                                              arraysize(kAppConsumeMessages)));
  EXPECT_NO_FATAL_FAILURE(CheckProducer(
      ic.get(), kAppProduceMessages, arraysize(kAppProduceMessages)));

  EXPECT_NO_FATAL_FAILURE(CheckMessagesNeedConsumer(
      ic.get(), kAppProduceMessages, arraysize(kAppProduceMessages), false));

  // Owner can resign an active consumer role.
  const uint32 kAppResignMessages[] = {
    ipc::MSG_GET_DOCUMENT_INFO,
  };
  EXPECT_TRUE(ic->ResignActiveConsumer(app1_.get(), kAppResignMessages, 1));

  // We should get OnComponentDeactivated() call for this message.
  ASSERT_EQ(1, deactivated_.size());
  EXPECT_EQ(app1_.get(), deactivated_[0]);
  EXPECT_EQ(ipc::MSG_GET_DOCUMENT_INFO, deactivated_messages_[0][0]);

  // We should get OnActiveConsumerChanged() call for this message.
  ASSERT_EQ(1, active_consumer_changed_.size());
  EXPECT_EQ(ipc::MSG_GET_DOCUMENT_INFO, active_consumer_changed_[0]);

  // No other component should be activated.
  EXPECT_EQ(0, activated_.size());
  ResetDelegate();

  EXPECT_FALSE(ic->HasActiveConsumer(ipc::MSG_GET_DOCUMENT_INFO));
  EXPECT_FALSE(ic->MayConsume(ipc::MSG_GET_DOCUMENT_INFO, false));

  // Attach the owner again won't activate the owner's resigned consumer roles.
  EXPECT_TRUE(ic->AttachComponent(
      app1_.get(), InputContext::ACTIVE_STICKY, true));
  EXPECT_FALSE(ic->HasActiveConsumer(ipc::MSG_GET_DOCUMENT_INFO));
  EXPECT_FALSE(ic->MayConsume(ipc::MSG_GET_DOCUMENT_INFO, false));
  EXPECT_EQ(0, activated_.size());
  EXPECT_EQ(0, deactivated_.size());
  EXPECT_EQ(0, active_consumer_changed_.size());

  // Get the active consumer role again.
  EXPECT_TRUE(ic->AssignActiveConsumer(app1_.get(), kAppResignMessages, 1));
  ASSERT_EQ(1, activated_.size());
  EXPECT_EQ(app1_.get(), activated_[0]);
  EXPECT_EQ(ipc::MSG_GET_DOCUMENT_INFO, activated_messages_[0][0]);

  ASSERT_EQ(1, active_consumer_changed_.size());
  EXPECT_EQ(ipc::MSG_GET_DOCUMENT_INFO, active_consumer_changed_[0]);

  // No other component should be deactivated.
  EXPECT_EQ(0, deactivated_.size());
  ResetDelegate();

  // Delegate's OnComponentDetached() won't be called for the owner when
  // destroying the input context.
  ic.reset();
  EXPECT_EQ(0, detached_.size());
  EXPECT_EQ(0, deactivated_.size());
}

TEST_F(HubInputContextTest, AttachDetach) {
  scoped_ptr<InputContext> ic(new InputContext(123, app1_.get(), this));
  input_context_ = ic.get();
  ResetDelegate();

  // Attach a component with a pending state.
  EXPECT_TRUE(ic->AttachComponent(
      ime1_.get(), InputContext::PENDING_PASSIVE, false));
  EXPECT_EQ(InputContext::PENDING_PASSIVE,
            ic->GetComponentAttachState(ime1_.get()));
  EXPECT_TRUE(ic->IsComponentPending(ime1_.get()));
  EXPECT_TRUE(ic->IsComponentPendingPassive(ime1_.get()));
  EXPECT_FALSE(ic->IsComponentReallyAttached(ime1_.get()));

  EXPECT_TRUE(ic->AttachComponent(
      ime1_.get(), InputContext::PENDING_ACTIVE, false));
  EXPECT_EQ(InputContext::PENDING_ACTIVE,
            ic->GetComponentAttachState(ime1_.get()));
  EXPECT_TRUE(ic->IsComponentPending(ime1_.get()));
  EXPECT_TRUE(ic->IsComponentPendingActive(ime1_.get()));

  // Detach a pending component.
  EXPECT_TRUE(ic->DetachComponent(ime1_.get()));
  EXPECT_EQ(InputContext::NOT_ATTACHED,
            ic->GetComponentAttachState(ime1_.get()));

  // Attach a component.
  EXPECT_TRUE(ic->AttachComponent(
      ime1_.get(), InputContext::PASSIVE, false));
  EXPECT_EQ(InputContext::PASSIVE, ic->GetComponentAttachState(ime1_.get()));
  EXPECT_FALSE(ic->IsComponentPending(ime1_.get()));
  EXPECT_TRUE(ic->IsComponentReallyAttached(ime1_.get()));

  ASSERT_EQ(1, activated_.size());
  EXPECT_EQ(ime1_.get(), activated_[0]);

  // Can't revert back to a pending state.
  EXPECT_FALSE(ic->AttachComponent(
      ime1_.get(), InputContext::PENDING_PASSIVE, false));
  EXPECT_EQ(InputContext::PASSIVE, ic->GetComponentAttachState(ime1_.get()));

  ResetDelegate();
  EXPECT_TRUE(ic->DetachComponent(ime1_.get()));
  EXPECT_EQ(1, detached_.size());
  EXPECT_EQ(InputContext::NOT_ATTACHED,
            ic->GetComponentAttachState(ime1_.get()));

  // Attach a component persistently.
  EXPECT_TRUE(ic->AttachComponent(ime1_.get(), InputContext::PASSIVE, true));
  EXPECT_TRUE(ic->IsComponentPersistent(ime1_.get()));
  EXPECT_EQ(InputContext::PASSIVE, ic->GetComponentAttachState(ime1_.get()));

  ResetDelegate();

  // We can change a component's persistent state.
  ic->SetComponentPersistent(ime1_.get(), false);
  EXPECT_FALSE(ic->IsComponentPersistent(ime1_.get()));
  EXPECT_TRUE(ic->AttachComponent(ime1_.get(), InputContext::PASSIVE, true));
  EXPECT_TRUE(ic->IsComponentPersistent(ime1_.get()));
}

TEST_F(HubInputContextTest, RedundantComponent) {
  scoped_ptr<InputContext> ic(new InputContext(123, app1_.get(), this));
  input_context_ = ic.get();
  ResetDelegate();

  EXPECT_TRUE(ic->AttachComponent(
      ime1_.get(), InputContext::PASSIVE, false));
  ResetDelegate();

  // No message need consumer, so ime1_ should be redundant.
  ic->MaybeDetachRedundantComponents();
  EXPECT_EQ(1, maybe_detach_.size());
  EXPECT_EQ(ime1_.get(), maybe_detach_[0]);
  ResetDelegate();

  // Set some messages need consumer.
  const uint32 kAppMessagesNeedConsumer[] = {
    ipc::MSG_CREATE_INPUT_CONTEXT,
    ipc::MSG_DELETE_INPUT_CONTEXT,
    ipc::MSG_FOCUS_INPUT_CONTEXT,
    ipc::MSG_BLUR_INPUT_CONTEXT,
    ipc::MSG_PROCESS_KEY_EVENT,
  };

  std::vector<uint32> already_have_consumers;
  ic->SetMessagesNeedConsumer(app1_.get(),
                              kAppMessagesNeedConsumer,
                              arraysize(kAppMessagesNeedConsumer),
                              &already_have_consumers);

  // ime1_ can consume three of them. But the first two still need consumers.
  EXPECT_EQ(2, request_consumer_messages_.size());
  for (size_t i = 0; i < 2; ++i) {
    EXPECT_EQ(1, std::count(request_consumer_messages_.begin(),
                            request_consumer_messages_.end(),
                            kAppMessagesNeedConsumer[i]));
  }

  EXPECT_EQ(3, already_have_consumers.size());
  for (size_t i = 2; i < arraysize(kAppMessagesNeedConsumer); ++i) {
    EXPECT_EQ(1, std::count(already_have_consumers.begin(),
                            already_have_consumers.end(),
                            kAppMessagesNeedConsumer[i]));
  }

  ResetDelegate();

  // No redundant component.
  ic->MaybeDetachRedundantComponents();
  EXPECT_EQ(0, maybe_detach_.size());

  const uint32 kAppMessagesNeedConsumer2[] = {
    ipc::MSG_CREATE_INPUT_CONTEXT,
    ipc::MSG_DELETE_INPUT_CONTEXT,
  };

  // Remove some messages that need consumer.
  ic->SetMessagesNeedConsumer(app1_.get(),
                              kAppMessagesNeedConsumer2,
                              arraysize(kAppMessagesNeedConsumer2),
                              NULL);

  EXPECT_EQ(1, maybe_detach_.size());
  EXPECT_EQ(ime1_.get(), maybe_detach_[0]);
  ResetDelegate();

  // Persistent component should not be treated as redundant at all.
  ic->SetComponentPersistent(ime1_.get(), true);
  ic->MaybeDetachRedundantComponents();
  EXPECT_EQ(0, maybe_detach_.size());

  // Try to detach a pending component which may be expected to consume some
  // messages.
  EXPECT_TRUE(ic->DetachComponent(ime1_.get()));
  ic->SetMessagesNeedConsumer(app1_.get(),
                              kAppMessagesNeedConsumer,
                              arraysize(kAppMessagesNeedConsumer),
                              NULL);
  EXPECT_EQ(5, request_consumer_messages_.size());

  EXPECT_TRUE(ic->AttachComponent(
      ime1_.get(), InputContext::PENDING_PASSIVE, false));
  ResetDelegate();

  EXPECT_TRUE(ic->DetachComponent(ime1_.get()));
  EXPECT_EQ(3, request_consumer_messages_.size());
  for (size_t i = 2; i < arraysize(kAppMessagesNeedConsumer); ++i) {
    EXPECT_EQ(1, std::count(request_consumer_messages_.begin(),
                            request_consumer_messages_.end(),
                            kAppMessagesNeedConsumer[i]));
  }

  EXPECT_TRUE(ic->AttachComponent(
      ime1_.get(), InputContext::PASSIVE, false));
  ResetDelegate();

  // when activating ime2_, ime1_ should be detached.
  EXPECT_TRUE(ic->AttachComponent(
      ime2_.get(), InputContext::ACTIVE, false));

  EXPECT_EQ(1, maybe_detach_.size());
  EXPECT_EQ(ime1_.get(), maybe_detach_[0]);
  ResetDelegate();
}

TEST_F(HubInputContextTest, Comprehensive) {
  scoped_ptr<InputContext> ic(new InputContext(123, app1_.get(), this));
  input_context_ = ic.get();
  ResetDelegate();

  ic.get()->SetMessagesNeedConsumer(
      app1_.get(), kAppProduceMessages, arraysize(kAppProduceMessages), NULL);

  // Attach the mock hub component.
  EXPECT_TRUE(ic->AttachComponent(
      hub_.get(), InputContext::ACTIVE_STICKY, true));
  EXPECT_TRUE(ic->IsComponentActive(hub_.get()));

  ic.get()->SetMessagesNeedConsumer(
      hub_.get(), kHubProduceMessages, arraysize(kHubProduceMessages), NULL);

  // Check delegate result data.
  ASSERT_EQ(1, activated_.size());
  EXPECT_EQ(hub_.get(), activated_[0]);
  EXPECT_NO_FATAL_FAILURE(CheckActivatedMessages(
      0, kHubConsumeMessages, arraysize(kHubConsumeMessages)));
  EXPECT_NO_FATAL_FAILURE(CheckActiveConsumerChangedMessages(
      kHubConsumeMessages, arraysize(kHubConsumeMessages)));
  ASSERT_EQ(0, deactivated_.size());
  ResetDelegate();

  EXPECT_NO_FATAL_FAILURE(CheckActiveConsumer(ic.get(), hub_.get(),
                                              kHubConsumeMessages,
                                              arraysize(kHubConsumeMessages)));

  // Attach another application component won't replace the owner.
  EXPECT_TRUE(ic->AttachComponent(
      app2_.get(), InputContext::ACTIVE_STICKY, true));
  EXPECT_FALSE(ic->IsComponentActive(app2_.get()));
  EXPECT_NO_FATAL_FAILURE(CheckActiveConsumer(ic.get(), app1_.get(),
                                              kAppConsumeMessages,
                                              arraysize(kAppConsumeMessages)));
  EXPECT_TRUE(ic->DetachComponent(app2_.get()));
  EXPECT_NO_FATAL_FAILURE(CheckActiveConsumer(ic.get(), app1_.get(),
                                              kAppConsumeMessages,
                                              arraysize(kAppConsumeMessages)));
  // Check delegate result data.
  ASSERT_EQ(1, detached_.size());
  EXPECT_EQ(app2_.get(), detached_[0]);
  EXPECT_EQ(0, activated_.size());
  EXPECT_EQ(0, deactivated_.size());
  ResetDelegate();

  // All messages don't have active consumer yet.
  const uint32 kMessages1[] = {
    ipc::MSG_ATTACH_TO_INPUT_CONTEXT,
    ipc::MSG_DETACHED_FROM_INPUT_CONTEXT,
    ipc::MSG_CANCEL_COMPOSITION,
    ipc::MSG_COMPLETE_COMPOSITION,
    ipc::MSG_CANDIDATE_LIST_CHANGED,
    ipc::MSG_SELECTED_CANDIDATE_CHANGED,
    ipc::MSG_CANDIDATE_LIST_VISIBILITY_CHANGED,
    ipc::MSG_SHOW_COMPOSITION_UI,
    ipc::MSG_HIDE_COMPOSITION_UI,
    ipc::MSG_SHOW_CANDIDATE_LIST_UI,
    ipc::MSG_HIDE_CANDIDATE_LIST_UI,
  };

  EXPECT_NO_FATAL_FAILURE(CheckMessagesNeedConsumer(
      ic.get(), kMessages1, arraysize(kMessages1), false));

  // Attach input method component.
  EXPECT_TRUE(ic->AttachComponent(
      ime1_.get(), InputContext::PENDING_PASSIVE, false));
  EXPECT_EQ(0, activated_.size());

  ic.get()->SetMessagesNeedConsumer(
      ime1_.get(), kIMEProduceMessages, arraysize(kIMEProduceMessages), NULL);

  EXPECT_NO_FATAL_FAILURE(CheckMessagesNeedConsumer(
      ic.get(), kMessages1, arraysize(kMessages1), false));

  // All messages don't have active consumer taking the input method into
  // account.
  const uint32 kMessages2[] = {
    ipc::MSG_CANDIDATE_LIST_CHANGED,
    ipc::MSG_SELECTED_CANDIDATE_CHANGED,
    ipc::MSG_CANDIDATE_LIST_VISIBILITY_CHANGED,
    ipc::MSG_SHOW_COMPOSITION_UI,
    ipc::MSG_HIDE_COMPOSITION_UI,
    ipc::MSG_SHOW_CANDIDATE_LIST_UI,
    ipc::MSG_HIDE_CANDIDATE_LIST_UI,
  };

  EXPECT_NO_FATAL_FAILURE(CheckMessagesNeedConsumer(
      ic.get(), kMessages2, arraysize(kMessages2), true));

  // Turn the input method into passive state.
  EXPECT_TRUE(ic->AttachComponent(
      ime1_.get(), InputContext::PASSIVE, false));
  EXPECT_EQ(InputContext::PASSIVE, ic->GetComponentAttachState(ime1_.get()));

  EXPECT_NO_FATAL_FAILURE(CheckMessagesNeedConsumer(
      ic.get(), kMessages2, arraysize(kMessages2), false));

  // Messages should be actively consumed by the input method.
  const uint32 kMessages3[] = {
    ipc::MSG_ATTACH_TO_INPUT_CONTEXT,
    ipc::MSG_DETACHED_FROM_INPUT_CONTEXT,
    ipc::MSG_CANCEL_COMPOSITION,
    ipc::MSG_COMPLETE_COMPOSITION,
    ipc::MSG_CANDIDATE_LIST_SHOWN,
    ipc::MSG_CANDIDATE_LIST_HIDDEN,
    ipc::MSG_CANDIDATE_LIST_PAGE_DOWN,
    ipc::MSG_CANDIDATE_LIST_PAGE_UP,
    ipc::MSG_CANDIDATE_LIST_SCROLL_TO,
    ipc::MSG_CANDIDATE_LIST_PAGE_RESIZE,
    ipc::MSG_SELECT_CANDIDATE,
  };

  EXPECT_NO_FATAL_FAILURE(CheckActiveConsumer(
      ic.get(), ime1_.get(), kMessages3, arraysize(kMessages3)));

  // Check delegate result data.
  ASSERT_EQ(1, activated_.size());
  EXPECT_EQ(ime1_.get(), activated_[0]);
  EXPECT_NO_FATAL_FAILURE(CheckActivatedMessages(
      0, kMessages3, arraysize(kMessages3)));
  EXPECT_NO_FATAL_FAILURE(CheckActiveConsumerChangedMessages(
      kMessages3, arraysize(kMessages3)));
  EXPECT_EQ(0, deactivated_.size());
  ResetDelegate();

  // It's not allowed to turn a component back into pending state.
  EXPECT_FALSE(ic->AttachComponent(
      ime1_.get(), InputContext::PENDING_ACTIVE, false));
  EXPECT_FALSE(ic->AttachComponent(
      ime1_.get(), InputContext::PENDING_PASSIVE, false));
  EXPECT_EQ(InputContext::PASSIVE, ic->GetComponentAttachState(ime1_.get()));

  // Turn the input method into active state.
  EXPECT_TRUE(ic->AttachComponent(
      ime1_.get(), InputContext::ACTIVE, false));
  EXPECT_EQ(InputContext::ACTIVE, ic->GetComponentAttachState(ime1_.get()));

  EXPECT_NO_FATAL_FAILURE(CheckMessagesNeedConsumer(
      ic.get(), kMessages2, arraysize(kMessages2), false));

  EXPECT_NO_FATAL_FAILURE(CheckActiveConsumer(
      ic.get(), ime1_.get(), kMessages3, arraysize(kMessages3)));

  // Check delegate result data.
  EXPECT_EQ(0, activated_.size());
  EXPECT_EQ(0, deactivated_.size());

  // Attach Candidate UI.
  EXPECT_TRUE(ic->AttachComponent(
      candidate_ui_.get(), InputContext::PASSIVE, false));
  EXPECT_EQ(InputContext::PASSIVE,
            ic->GetComponentAttachState(candidate_ui_.get()));

  ic.get()->SetMessagesNeedConsumer(candidate_ui_.get(),
                                    kCandidateUIProduceMessages,
                                    arraysize(kCandidateUIProduceMessages),
                                    NULL);

  // The candidate UI shouldn't take over any active consumer role from the
  // input method.
  EXPECT_NO_FATAL_FAILURE(CheckActiveConsumer(
      ic.get(), ime1_.get(), kMessages3, arraysize(kMessages3)));

  const uint32 kMessages4[] = {
    ipc::MSG_CANDIDATE_LIST_CHANGED,
    ipc::MSG_SELECTED_CANDIDATE_CHANGED,
    ipc::MSG_CANDIDATE_LIST_VISIBILITY_CHANGED,
    ipc::MSG_SHOW_CANDIDATE_LIST_UI,
    ipc::MSG_HIDE_CANDIDATE_LIST_UI,
  };

  // The candidate UI should actively consume remained messages.
  EXPECT_NO_FATAL_FAILURE(CheckActiveConsumer(
      ic.get(), candidate_ui_.get(), kMessages4, arraysize(kMessages4)));

  // Check delegate result data.
  ASSERT_EQ(1, activated_.size());
  EXPECT_EQ(candidate_ui_.get(), activated_[0]);
  EXPECT_NO_FATAL_FAILURE(CheckActivatedMessages(
      0, kMessages4, arraysize(kMessages4)));
  EXPECT_NO_FATAL_FAILURE(CheckActiveConsumerChangedMessages(
      kMessages4, arraysize(kMessages4)));
  EXPECT_EQ(0, deactivated_.size());
  ResetDelegate();

  // Attach the off-the-spot compose UI.
  EXPECT_TRUE(ic->AttachComponent(
      compose_ui_.get(), InputContext::PASSIVE, false));
  EXPECT_EQ(InputContext::PASSIVE,
            ic->GetComponentAttachState(compose_ui_.get()));

  ic.get()->SetMessagesNeedConsumer(compose_ui_.get(),
                                    kComposeUIProduceMessages,
                                    arraysize(kComposeUIProduceMessages),
                                    NULL);

  const uint32 kMessages5[] = {
    ipc::MSG_SHOW_COMPOSITION_UI,
    ipc::MSG_HIDE_COMPOSITION_UI,
  };

  // The compose UI should actively consume remained messages.
  EXPECT_NO_FATAL_FAILURE(CheckActiveConsumer(
      ic.get(), compose_ui_.get(), kMessages5, arraysize(kMessages5)));

  // Check delegate result data.
  ASSERT_EQ(1, activated_.size());
  EXPECT_EQ(compose_ui_.get(), activated_[0]);
  EXPECT_NO_FATAL_FAILURE(CheckActivatedMessages(
      0, kMessages5, arraysize(kMessages5)));
  EXPECT_NO_FATAL_FAILURE(CheckActiveConsumerChangedMessages(
      kMessages5, arraysize(kMessages5)));
  EXPECT_EQ(0, deactivated_.size());
  ResetDelegate();

  // All messages should now have active consumer.
  EXPECT_NO_FATAL_FAILURE(CheckMessagesNeedConsumer(ic.get(), NULL, 0, false));

  std::vector<Component*> consumers;
  // Test InputContext::GetAllConsumers().
  EXPECT_EQ(
      2, ic->GetAllConsumers(ipc::MSG_COMPOSITION_CHANGED, false, &consumers));
  EXPECT_EQ(app1_.get(), consumers[0]);
  EXPECT_EQ(compose_ui_.get(), consumers[1]);

  // Attach another input method.
  EXPECT_TRUE(ic->AttachComponent(
      ime2_.get(), InputContext::PENDING_PASSIVE, false));
  EXPECT_EQ(0, activated_.size());
  EXPECT_EQ(0, deactivated_.size());

  ic.get()->SetMessagesNeedConsumer(
      ime2_.get(), kIMEProduceMessages, arraysize(kIMEProduceMessages), NULL);

  EXPECT_EQ(
      3, ic->GetAllConsumers(ipc::MSG_PROCESS_KEY_EVENT, true, &consumers));
  EXPECT_EQ(hub_.get(), consumers[0]);
  // The sequence of returned consumers are not fixed.
  EXPECT_TRUE(ime1_.get() == consumers[1] || ime1_.get() == consumers[2]);
  EXPECT_TRUE(ime2_.get() == consumers[1] || ime2_.get() == consumers[2]);

  // Activated the second input method
  EXPECT_TRUE(ic->AttachComponent(
      ime2_.get(), InputContext::ACTIVE, false));
  EXPECT_NO_FATAL_FAILURE(CheckActiveConsumer(
      ic.get(), ime2_.get(), kMessages3, arraysize(kMessages3)));
  EXPECT_FALSE(ic->IsComponentActive(ime1_.get()));

  // Check delegate result data.
  ASSERT_EQ(1, activated_.size());
  EXPECT_EQ(ime2_.get(), activated_[0]);
  ASSERT_EQ(1, deactivated_.size());
  EXPECT_EQ(ime1_.get(), deactivated_[0]);
  ASSERT_EQ(1, maybe_detach_.size());
  EXPECT_EQ(ime1_.get(), maybe_detach_[0]);
  ResetDelegate();

  // Detach the second input method. The first input method should be activated
  // again.
  EXPECT_TRUE(ic->DetachComponent(ime2_.get()));
  ASSERT_EQ(1, deactivated_.size());
  EXPECT_EQ(ime2_.get(), deactivated_[0]);
  ASSERT_EQ(1, detached_.size());
  EXPECT_EQ(ime2_.get(), detached_[0]);
  ASSERT_EQ(1, activated_.size());
  EXPECT_EQ(ime1_.get(), activated_[0]);
  EXPECT_NO_FATAL_FAILURE(CheckActiveConsumer(
      ic.get(), ime1_.get(), kMessages3, arraysize(kMessages3)));
  ResetDelegate();

  ic.reset();
  // OnComponentDetached() should be called for hub_, ime2_, candidate_ui_ and
  // compose_ui_.
  ASSERT_EQ(4, detached_.size());
}

}  // namespace
