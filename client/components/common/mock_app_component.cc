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

#include "components/common/mock_app_component.h"

#include "base/scoped_ptr.h"
#include "components/common/constants.h"
#include "ipc/constants.h"
#include "ipc/message_types.h"

namespace {

enum {
  MSG_MOCK_APP_CREATE_INPUT_CONTEXT = ipc::MSG_USER_DEFINED_START,
  MSG_MOCK_APP_REQUEST_CONSUMER = ipc::MSG_USER_DEFINED_START + 1,
  MSG_MOCK_APP_START_COMPOSITION = ipc::MSG_USER_DEFINED_START + 2,
};

const uint32 kProduceMessages[] = {
  // User defined messgaes.
  MSG_MOCK_APP_CREATE_INPUT_CONTEXT,
  MSG_MOCK_APP_START_COMPOSITION,
  // Context related messages.
  ipc::MSG_CREATE_INPUT_CONTEXT,
  ipc::MSG_DEREGISTER_COMPONENT,
  ipc::MSG_DELETE_INPUT_CONTEXT,
  ipc::MSG_QUERY_ACTIVE_CONSUMER,
  ipc::MSG_REGISTER_COMPONENT,
  ipc::MSG_REQUEST_CONSUMER,
  ipc::MSG_FOCUS_INPUT_CONTEXT,
  // Composition related messages.
  ipc::MSG_COMPLETE_COMPOSITION,
  ipc::MSG_CANCEL_COMPOSITION,
  ipc::MSG_CANDIDATE_LIST_PAGE_DOWN,
  ipc::MSG_CANDIDATE_LIST_PAGE_UP,
  ipc::MSG_CANDIDATE_LIST_PAGE_RESIZE,
  ipc::MSG_SEND_KEY_EVENT,
  ipc::MSG_SELECT_CANDIDATE,
  ipc::MSG_SWITCH_TO_INPUT_METHOD,
};

const uint32 kConsumeMessages[] = {
  // User defined messgaes.
  MSG_MOCK_APP_CREATE_INPUT_CONTEXT,
  MSG_MOCK_APP_REQUEST_CONSUMER,
  MSG_MOCK_APP_START_COMPOSITION,
  // Context related messages.
  ipc::MSG_ACTIVE_CONSUMER_CHANGED,
  ipc::MSG_INPUT_CONTEXT_DELETED,
  ipc::MSG_INPUT_CONTEXT_LOST_FOCUS,
  ipc::MSG_INPUT_CONTEXT_GOT_FOCUS,
  // Composition related messages.
  ipc::MSG_INSERT_TEXT,
  ipc::MSG_SET_CANDIDATE_LIST,
  ipc::MSG_SET_COMPOSITION,
  ipc::MSG_SET_SELECTED_CANDIDATE,
};

const uint32 kRequestConsumerMessages[] = {
  ipc::MSG_SEND_KEY_EVENT,
};

}  // namespace

namespace ime_goopy {
namespace components {

MockAppComponent::MockAppComponent(const std::string& comp_id)
    : icid_(0),
      listener_(NULL),
      ready_(false),
      comp_id_(comp_id) {
}

MockAppComponent::~MockAppComponent() {
}

void MockAppComponent::GetInfo(ipc::proto::ComponentInfo* info) {
  info->set_string_id(comp_id_);

  for (size_t i = 0; i < arraysize(kProduceMessages); ++i)
    info->add_produce_message(kProduceMessages[i]);

  for (size_t i = 0; i < arraysize(kConsumeMessages); ++i)
    info->add_consume_message(kConsumeMessages[i]);
}

void MockAppComponent::Handle(ipc::proto::Message* message) {
  DCHECK(typist_);
  scoped_ptr<ipc::proto::Message> mptr(message);
  switch (mptr->type()) {
    case MSG_MOCK_APP_CREATE_INPUT_CONTEXT:
      CreateInputContextInternal();
      return;
    case MSG_MOCK_APP_START_COMPOSITION:
      if (!ready_)
        ReplyFalse(mptr.release());
      else
        typist_->Composite();
      return;
    case ipc::MSG_ACTIVE_CONSUMER_CHANGED: {
      if (!message->icid())
        break;
      const int type_size = mptr->payload().uint32_size();
      DCHECK(type_size);
      for (int i = 0; i < type_size; ++i) {
        if (mptr->payload().uint32(i) == ipc::MSG_PROCESS_KEY_EVENT) {
          ready_ = true;
          listener_->OnAppComponentReady();
          break;
        }
      }
      break;
    }
    case ipc::MSG_INPUT_CONTEXT_DELETED:
      if (mptr->payload().uint32_size() && mptr->payload().uint32(0) == icid_) {
        icid_ = 0;
        ready_ = false;
        listener_->OnAppComponentStopped();
      }
      break;
    case ipc::MSG_SET_COMPOSITION:
    case ipc::MSG_SET_CANDIDATE_LIST:
    case ipc::MSG_SET_SELECTED_CANDIDATE:
    case ipc::MSG_INSERT_TEXT:
      typist_->OnMessageReceived(mptr.release());
      break;
    case ipc::MSG_SEND_KEY_EVENT:  // reply
      DCHECK_EQ(mptr->reply_mode(), ipc::proto::Message::IS_REPLY);
      typist_->OnMessageReplyReceived(mptr.release());
      break;
    default:
      DLOG(ERROR) << "Can't handle message type: " << mptr->type();
      ReplyError(mptr.release(), ipc::proto::Error::INVALID_MESSAGE, NULL);
      break;
  }

  if (mptr.get())
    ReplyTrue(mptr.release());
}

void MockAppComponent::OnRegistered() {
  listener_->OnRegistered();
}

void MockAppComponent::Start() {
  scoped_ptr<ipc::proto::Message> mptr(
      NewMessage(MSG_MOCK_APP_CREATE_INPUT_CONTEXT,
                 ipc::kInputContextNone, false));
  mptr->set_source(id());
  mptr->set_target(id());
  if (!Send(mptr.release(), NULL))
    DLOG(ERROR) << "Send error: MSG_MOCK_APP_CREATE_INPUT_CONTEXT";
}

void MockAppComponent::Stop() {
  scoped_ptr<ipc::proto::Message> mptr(NewMessage(
      ipc::MSG_DELETE_INPUT_CONTEXT,
      icid_,
      false));

  if (!Send(mptr.release(), NULL))
    DLOG(ERROR) << "Send error: MSG_DELETE_INPUT_CONTEXT";
}

void MockAppComponent::HandleKey(Typist* typist, uint32 keycode) {
  DCHECK_EQ(typist_, typist);
  // Convert key code
  ipc::proto::KeyEvent key_event;
  key_event.set_keycode(keycode);
  char key_state[256] = {0};
  DCHECK_LT(keycode, 256);
  key_state[keycode] |= 0x80;
  key_event.set_native_key_event(key_state, 256);
  key_event.set_type(ipc::proto::KeyEvent::DOWN);
  HandleKeyEvent(typist, key_event);
}

void MockAppComponent::HandleKeyEvent(Typist* typist,
                                      const ipc::proto::KeyEvent& key_event) {
  DCHECK_EQ(typist_, typist);
  scoped_ptr<ipc::proto::Message> mptr(NewMessage(ipc::MSG_SEND_KEY_EVENT,
                                                  icid_,
                                                  true));
  mptr->mutable_payload()->mutable_key_event()->CopyFrom(key_event);
  if (!Send(mptr.release(), NULL))
    DLOG(ERROR) << "SendMessage failed";
}

void MockAppComponent::CompleteComposition(Typist* typist) {
  DCHECK_EQ(typist_, typist);
  scoped_ptr<ipc::proto::Message> mptr(
      NewMessage(ipc::MSG_COMPLETE_COMPOSITION, icid_, false));
  if (!Send(mptr.release(), NULL))
    DLOG(ERROR) << "SendMessage failed";
}

void MockAppComponent::CancelComposition(Typist* typist, bool commit) {
  DCHECK_EQ(typist_, typist);
  scoped_ptr<ipc::proto::Message> mptr(
      NewMessage(ipc::MSG_CANCEL_COMPOSITION, icid_, false));
  mptr->mutable_payload()->add_boolean(commit);
  if (!Send(mptr.release(), NULL))
    DLOG(ERROR) << "SendMessage failed";
}

void MockAppComponent::SelectCandidate(
    Typist* typist, uint32 candidate_index, bool commit) {
  DCHECK_EQ(typist_, typist);
  scoped_ptr<ipc::proto::Message> mptr(
      NewMessage(ipc::MSG_SELECT_CANDIDATE, icid_, false));
  mptr->mutable_payload()->add_uint32(0);
  mptr->mutable_payload()->add_uint32(candidate_index);
  mptr->mutable_payload()->add_boolean(commit);
  if (!Send(mptr.release(), NULL))
    DLOG(ERROR) << "SendMessage failed";
}

void MockAppComponent::CandidateListPageDown(Typist* typist) {
  DCHECK_EQ(typist_, typist);
  scoped_ptr<ipc::proto::Message> mptr(
      NewMessage(ipc::MSG_CANDIDATE_LIST_PAGE_DOWN, icid_, false));
  if (!Send(mptr.release(), NULL))
    DLOG(ERROR) << "SendMessage failed";
}

void MockAppComponent::CandidateListPageUp(Typist* typist) {
  DCHECK_EQ(typist_, typist);
  scoped_ptr<ipc::proto::Message> mptr(
      NewMessage(ipc::MSG_CANDIDATE_LIST_PAGE_UP,
                 icid_,
                 false));
  if (!Send(mptr.release(), NULL))
    DLOG(ERROR) << "SendMessage failed";
}

void MockAppComponent::CandidateListPageResize(Typist* typist, int size) {
  DCHECK_EQ(typist_, typist);
  scoped_ptr<ipc::proto::Message> mptr(
      NewMessage(ipc::MSG_CANDIDATE_LIST_PAGE_RESIZE, icid_, false));
  mptr->mutable_payload()->add_uint32(0);
  mptr->mutable_payload()->add_uint32(size);
  mptr->mutable_payload()->add_uint32(1);
  if (!Send(mptr.release(), NULL))
    DLOG(ERROR) << "SendMessage failed";
}

void MockAppComponent::FocusInputContext() {
  scoped_ptr<ipc::proto::Message> mptr(
      NewMessage(ipc::MSG_FOCUS_INPUT_CONTEXT, icid_, false));

  if (!Send(mptr.release(), NULL))
    DLOG(ERROR) << "Send error type = MSG_FOCUS_INPUT_CONTEXT";
}

void MockAppComponent::SwitchToKeyboardInput() {
  scoped_ptr<ipc::proto::Message> mptr(
      NewMessage(ipc::MSG_SWITCH_TO_INPUT_METHOD, icid_, false));
  mptr->mutable_payload()->add_string(kKeyboardInputComponentStringId);

  if (!Send(mptr.release(), NULL))
    DLOG(ERROR) << "Send error type = MSG_SWITCH_TO_INPUT_METHOD";
}

void MockAppComponent::UserComposite() {
  scoped_ptr<ipc::proto::Message> mptr(NewMessage(
      MSG_MOCK_APP_START_COMPOSITION,
      icid_,
      true));

  if (!Send(mptr.release(), NULL)) {
    DLOG(ERROR) << "Senderror: MSG_MOCK_APP_START_COMPOSITION";
    return;
  }
}

void MockAppComponent::set_typist(Typist* typist) {
  typist_ = typist;
}

void MockAppComponent::CreateInputContextInternal() {
  scoped_ptr<ipc::proto::Message> mptr(NewMessage(
      ipc::MSG_CREATE_INPUT_CONTEXT,
      ipc::kInputContextNone,
      true));

  ipc::proto::Message* reply = NULL;
  if (!SendWithReply(mptr.release(), -1, &reply)) {
    DLOG(ERROR) << "SendWithReply error: MSG_CREATE_INPUT_CONTEXT";
    return;
  }

  DCHECK(reply);
  icid_ = reply->icid();
  delete reply;

  RequestConsumerInternal();
}

void MockAppComponent::RequestConsumerInternal() {
  scoped_ptr<ipc::proto::Message> mptr(NewMessage(
      ipc::MSG_REQUEST_CONSUMER,
      icid_,
      false));

  for (int i = 0; i < arraysize(kRequestConsumerMessages); ++i)
    mptr->mutable_payload()->add_uint32(kRequestConsumerMessages[i]);

  if (!Send(mptr.release(), NULL))
    DLOG(ERROR) << "Send error: MSG_REQUEST_CONSUMER";
}

}  // namespace components
}  // namespace ime_goopy
