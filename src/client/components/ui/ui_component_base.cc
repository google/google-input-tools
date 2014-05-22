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
#include "common/debug.h"
#include "ipc/component_host.h"
#include "ipc/constants.h"
#include "ipc/message_types.h"
#include "ipc/protos/ipc.pb.h"

namespace ime_goopy {
namespace components {
namespace {

const int kProduceMessages[] = {
  ipc::MSG_REGISTER_COMPONENT,
  ipc::MSG_DEREGISTER_COMPONENT,
  ipc::MSG_CANDIDATE_LIST_SHOWN,
  ipc::MSG_CANDIDATE_LIST_HIDDEN,
  ipc::MSG_CANDIDATE_LIST_PAGE_DOWN,
  ipc::MSG_CANDIDATE_LIST_PAGE_UP,
  ipc::MSG_CANDIDATE_LIST_SCROLL_TO,
  ipc::MSG_CANDIDATE_LIST_PAGE_RESIZE,
  ipc::MSG_SELECT_CANDIDATE,
  ipc::MSG_DO_CANDIDATE_COMMAND,
  ipc::MSG_QUERY_INPUT_CARET,
  ipc::MSG_QUERY_COMMAND_LIST,
  ipc::MSG_DO_COMMAND,
  ipc::MSG_LIST_INPUT_METHODS,
  ipc::MSG_SWITCH_TO_INPUT_METHOD,
  ipc::MSG_QUERY_ACTIVE_INPUT_METHOD,
  ipc::MSG_QUERY_COMPOSITION,
  ipc::MSG_QUERY_CANDIDATE_LIST,
  ipc::MSG_QUERY_ACTIVE_CONSUMER,
  ipc::MSG_SHOW_MENU,
  ipc::MSG_REQUEST_CONSUMER,
};

const int kConsumeMessages[] = {
  ipc::MSG_ATTACH_TO_INPUT_CONTEXT,
  ipc::MSG_DETACHED_FROM_INPUT_CONTEXT,
  ipc::MSG_INPUT_CONTEXT_DELETED,
  ipc::MSG_INPUT_CONTEXT_GOT_FOCUS,
  ipc::MSG_INPUT_CONTEXT_LOST_FOCUS,
  ipc::MSG_COMPOSITION_CHANGED,
  ipc::MSG_CANDIDATE_LIST_CHANGED,
  ipc::MSG_CANDIDATE_LIST_VISIBILITY_CHANGED,
  ipc::MSG_SELECTED_CANDIDATE_CHANGED,
  ipc::MSG_UPDATE_INPUT_CARET,
  ipc::MSG_COMMAND_LIST_CHANGED,
  ipc::MSG_INPUT_METHOD_ACTIVATED,
  ipc::MSG_SHOW_COMPOSITION_UI,
  ipc::MSG_HIDE_COMPOSITION_UI,
  ipc::MSG_SHOW_CANDIDATE_LIST_UI,
  ipc::MSG_HIDE_CANDIDATE_LIST_UI,
  ipc::MSG_SHOW_TOOLBAR_UI,
  ipc::MSG_HIDE_TOOLBAR_UI,
};

}  // namespace

UIComponentBase::UIComponentBase()
  : focused_icid_(ipc::kInputContextNone) {
}

UIComponentBase::~UIComponentBase() {
}

void UIComponentBase::GetInfo(ipc::proto::ComponentInfo* info) {
  info->set_string_id(GetComponentStringID());
  info->set_name(GetComponentName());
  for (size_t i = 0; i < arraysize(kProduceMessages) ; ++i)
    info->add_produce_message(kProduceMessages[i]);
  for (size_t i = 0; i < arraysize(kConsumeMessages); ++i)
    info->add_consume_message(kConsumeMessages[i]);
  GetSubComponentsInfo(info);
}

bool UIComponentBase::DoCommand(int owner, int icid, const std::string& id) {
  scoped_ptr<ipc::proto::Message> mptr(
      NewMessage(ipc::MSG_DO_COMMAND, icid, false));
  mptr->set_target(owner);
  mptr->mutable_payload()->add_string(id);
  return Send(mptr.release(), NULL);
}

bool UIComponentBase::DoCandidateCommand(
    int owner,
    int icid,
    int candidate_list_id,
    int candidate_index,
    const std::string& id) {
  scoped_ptr<ipc::proto::Message> mptr(
      NewMessage(ipc::MSG_DO_CANDIDATE_COMMAND, icid, false));
  mptr->set_target(owner);
  mptr->mutable_payload()->add_uint32(candidate_list_id);
  mptr->mutable_payload()->add_uint32(candidate_index);
  mptr->mutable_payload()->add_string(id);
  return Send(mptr.release(), NULL);
}

void UIComponentBase::SelectCandidate(
    int owner,
    int candidata_list_id,
    int candidate_index,
    bool commit) {
  scoped_ptr<ipc::proto::Message> mptr(
      NewMessage(ipc::MSG_SELECT_CANDIDATE, focused_icid_, false));
  mptr->set_target(owner);
  mptr->mutable_payload()->add_uint32(candidata_list_id);
  mptr->mutable_payload()->add_uint32(candidate_index);
  mptr->mutable_payload()->add_boolean(commit);
  Send(mptr.release(), NULL);
}

void UIComponentBase::SelectInputMethod(int new_input_method_id) {
  scoped_ptr<ipc::proto::Message> mptr(
    NewMessage(ipc::MSG_SWITCH_TO_INPUT_METHOD, focused_icid_, false));
  mptr->mutable_payload()->add_uint32(new_input_method_id);
  Send(mptr.release(), NULL);
}

void UIComponentBase::Handle(ipc::proto::Message* message) {
  if (HandleMessageBySubComponents(message))
    return;
  switch (message->type()) {
    case ipc::MSG_ATTACH_TO_INPUT_CONTEXT:
      OnMsgAttachToInputContext(message);
      break;
    case ipc::MSG_DETACHED_FROM_INPUT_CONTEXT:
      OnMsgDetachedFromInputContext(message);
      break;
      case ipc::MSG_INPUT_CONTEXT_DELETED:
      OnMsgInputContextDeleted(message);
      break;
    case ipc::MSG_INPUT_CONTEXT_GOT_FOCUS:
      OnMsgInputContextGotFocus(message);
      break;
    case ipc::MSG_INPUT_CONTEXT_LOST_FOCUS:
      OnMsgInputContextLostFocus(message);
      break;
    case ipc::MSG_CANDIDATE_LIST_VISIBILITY_CHANGED:
      OnMsgCandidateListVisibilityChanged(message);
      break;
    case ipc::MSG_UPDATE_INPUT_CARET:
      OnMsgUpdateInputCaret(message);
      break;
    case ipc::MSG_COMMAND_LIST_CHANGED:
      OnMsgCommandListChanged(message);
      break;
    case ipc::MSG_INPUT_METHOD_ACTIVATED:
      OnMsgInputMethodActivated(message);
      break;
    case ipc::MSG_SHOW_CANDIDATE_LIST_UI:
      OnMsgShowCandidateListUI(message);
      break;
    case ipc::MSG_SHOW_COMPOSITION_UI:
      OnMsgShowCompositionUI(message);
      break;
    case ipc::MSG_SHOW_TOOLBAR_UI:
      OnMsgShowToolBarUI(message);
      break;
    case ipc::MSG_HIDE_CANDIDATE_LIST_UI:
      OnMsgHideCandidateListUI(message);
      break;
    case ipc::MSG_HIDE_COMPOSITION_UI:
      OnMsgHideCompositionUI(message);
      break;
    case ipc::MSG_HIDE_TOOLBAR_UI:
      OnMsgHideToolBarUI(message);
      break;
    case ipc::MSG_CANDIDATE_LIST_CHANGED:
      OnMsgCandidateListChanged(message);
      break;
    case ipc::MSG_COMPOSITION_CHANGED:
      OnMsgCompositionChanged(message);
      break;
    case ipc::MSG_SELECTED_CANDIDATE_CHANGED:
      OnMsgSelectedCandidateChanged(message);
      break;
    default:
      int icid = message->icid();
      DLOG(ERROR) << L"Unexpected message received:"
        << L"type = " << message->type()
        << L"icid = " << message->icid();
      ReplyError(message, ipc::proto::Error::INVALID_MESSAGE,
                 "unknown type");
      break;
  }
}

void UIComponentBase::OnMsgAttachToInputContext(
    ipc::proto::Message* message) {
  scoped_ptr<ipc::proto::Message> mptr(
      NewMessage(ipc::MSG_REQUEST_CONSUMER, message->icid(), false));
  mptr->mutable_payload()->add_uint32(ipc::MSG_SHOW_MENU);
  if (!Send(mptr.release(), NULL))
    DLOG(ERROR) << L"Send ipc::MSG_REQUEST_CONSUMER failed";
  ReplyTrue(message);
}

void UIComponentBase::OnMsgDetachedFromInputContext(
    ipc::proto::Message* message) {
  if (message->payload().has_input_context_info()) {
    int icid = message->payload().input_context_info().id();
    if (icid == focused_icid_) {
      focused_icid_ = ipc::kInputContextNone;
      SetCandidateListVisibility(false);
      SetCompositionVisibility(false);
      SetToolbarVisibility(false);
    }
  }
  ReplyTrue(message);
}

void UIComponentBase::OnMsgInputContextDeleted(ipc::proto::Message* message) {
  if (message->has_payload() && message->payload().uint32_size() &&
        message->payload().uint32(0) == focused_icid_) {
    SetCandidateList(NULL);
    SetComposition(NULL);
    SetCandidateListVisibility(false);
    SetCompositionVisibility(false);
    SetToolbarVisibility(false);
    focused_icid_ = ipc::kInputContextNone;
  }
  ReplyTrue(message);
}

void UIComponentBase::OnMsgInputContextLostFocus(ipc::proto::Message* message) {
  if (!IsActiveICMessage(message)) {
    ReplyFalse(message);
    return;
  }
  SetCandidateListVisibility(false);
  SetCompositionVisibility(false);
  SetToolbarVisibility(false);
  focused_icid_ = ipc::kInputContextNone;
  ReplyTrue(message);
}

void UIComponentBase::RefreshUI() {
  host()->PauseMessageHandling(this);
  ipc::proto::Message* reply = NULL;
  scoped_ptr<ipc::proto::Message> mptr;
  mptr.reset(NewMessage(ipc::MSG_QUERY_CANDIDATE_LIST, focused_icid_, true));
  if (!SendWithReply(mptr.release(), -1, &reply)) {
    DLOG(ERROR) << L"SendWithReply failed";
  } else {
    // TODO(synch): change active selected candidate list when we support
    // cascaded candidate list.
    if (reply->has_payload() && reply->payload().has_candidate_list())
      SetCandidateList(&(reply->payload().candidate_list()));
    else
      SetCandidateList(NULL);
    delete reply;
  }
  mptr.reset(NewMessage(ipc::MSG_QUERY_COMPOSITION, focused_icid_, true));
  if (!SendWithReply(mptr.release(), -1, &reply)) {
    DLOG(ERROR) << L"SendWithReply failed";
  } else {
    if (reply->has_payload() && reply->payload().has_composition())
      SetComposition(&(reply->payload().composition()));
    else
      SetComposition(NULL);
    delete reply;
  }
  mptr.reset(NewMessage(ipc::MSG_LIST_INPUT_METHODS, focused_icid_, true));
  if (!SendWithReply(mptr.release(), -1, &reply)) {
    DLOG(ERROR) << L"SendWithReply failed";
  } else {
    if (reply->has_payload() && reply->payload().component_info_size())
      SetInputMethods(reply->payload().component_info());
    delete reply;
  }
  mptr.reset(NewMessage(
      ipc::MSG_QUERY_ACTIVE_INPUT_METHOD, focused_icid_, true));
  if (!SendWithReply(mptr.release(), -1, &reply)) {
    DLOG(ERROR) << L"SendWithReply failed";
  } else {
    if (reply->has_payload() && reply->payload().component_info_size() == 1)
      SetActiveInputMethod(reply->payload().component_info(0));
    delete reply;
  }
  mptr.reset(NewMessage(ipc::MSG_QUERY_COMMAND_LIST, focused_icid_, true));
  if (!SendWithReply(mptr.release(), -1, &reply)) {
    DLOG(ERROR) << L"SendWithReply failed";
  } else {
    SetCommandList(reply->payload().command_list());
    delete reply;
  }
  mptr.reset(NewMessage(ipc::MSG_QUERY_INPUT_CARET, focused_icid_, true));
  if (!SendWithReply(mptr.release(), -1, &reply)) {
    DLOG(ERROR) << L"SendWithReply failed";
  } else {
    if (reply->has_payload() && reply->payload().has_input_caret())
      SetInputCaret(reply->payload().input_caret());
    delete reply;
  }
  host()->ResumeMessageHandling(this);
}

void UIComponentBase::OnMsgInputContextGotFocus(ipc::proto::Message* message) {
  focused_icid_ = message->icid();

  SetCandidateListVisibility(false);
  SetCompositionVisibility(false);
  SetToolbarVisibility(false);
  RefreshUI();
  ReplyTrue(message);
}

void UIComponentBase::OnMsgCandidateListChanged(ipc::proto::Message* message) {
  if (!IsActiveICMessage(message)) {
    ReplyFalse(message);
    return;
  }
  if (message->has_payload() && message->payload().has_candidate_list())
    SetCandidateList(&(message->payload().candidate_list()));
  else
    SetCandidateList(NULL);
  ReplyTrue(message);
}

void UIComponentBase::OnMsgInputMethodActivated(ipc::proto::Message* message) {
  if (!IsActiveICMessage(message)) {
    ReplyFalse(message);
    return;
  }
  if (message->has_payload() && message->payload().component_info_size() == 1)
    SetActiveInputMethod(message->payload().component_info(0));
  ReplyTrue(message);
}

void UIComponentBase::OnMsgUpdateInputCaret(ipc::proto::Message* message) {
  if (!IsActiveICMessage(message)) {
    ReplyFalse(message);
    return;
  }
  if (message->payload().has_input_caret())
    SetInputCaret(message->payload().input_caret());
  ReplyTrue(message);
}

void UIComponentBase::OnMsgCommandListChanged(ipc::proto::Message* message) {
  if (!IsActiveICMessage(message)) {
    ReplyFalse(message);
    return;
  }
  SetCommandList(message->payload().command_list());
  ReplyTrue(message);
}

void UIComponentBase::OnMsgCompositionChanged(ipc::proto::Message* message) {
  if (!IsActiveICMessage(message)) {
    ReplyFalse(message);
    return;
  }
  if (message->has_payload() && message->payload().has_composition())
    SetComposition(&(message->payload().composition()));
  else
    SetComposition(NULL);
  ReplyTrue(message);
}

void UIComponentBase::OnMsgShowCompositionUI(ipc::proto::Message* message) {
  if (!IsActiveICMessage(message)) {
    ReplyFalse(message);
    return;
  }
  SetCompositionVisibility(true);
  ReplyTrue(message);
}

void UIComponentBase::OnMsgHideCompositionUI(ipc::proto::Message* message) {
  if (!IsActiveICMessage(message)) {
    ReplyFalse(message);
    return;
  }
  SetCompositionVisibility(false);
  ReplyTrue(message);
}

void UIComponentBase::OnMsgShowCandidateListUI(ipc::proto::Message* message) {
  if (!IsActiveICMessage(message)) {
    ReplyFalse(message);
    return;
  }
  SetCandidateListVisibility(true);
  ReplyTrue(message);
}

void UIComponentBase::OnMsgHideCandidateListUI(ipc::proto::Message* message) {
  if (!IsActiveICMessage(message)) {
    ReplyFalse(message);
    return;
  }
  SetCandidateListVisibility(false);
  ReplyTrue(message);
}

void UIComponentBase::OnMsgShowToolBarUI(ipc::proto::Message* message) {
  if (!IsActiveICMessage(message)) {
    ReplyFalse(message);
    return;
  }
  SetToolbarVisibility(true);
  ReplyTrue(message);
}

void UIComponentBase::OnMsgHideToolBarUI(ipc::proto::Message* message) {
  if (!IsActiveICMessage(message)) {
    ReplyFalse(message);
    return;
  }
  SetToolbarVisibility(false);
  ReplyTrue(message);
}

void UIComponentBase::OnMsgCandidateListVisibilityChanged(
    ipc::proto::Message* message) {
  if (!IsActiveICMessage(message)) {
    ReplyFalse(message);
    return;
  }
  if (message->has_payload() &&
      message->payload().boolean_size() == 1 &&
      message->payload().uint32_size() == 1) {
    ChangeCandidateListVisibility(message->payload().uint32(0),
                                  message->payload().boolean(0));
  }
  ReplyTrue(message);
}

void UIComponentBase::OnMsgSelectedCandidateChanged(
    ipc::proto::Message* message) {
  if (!IsActiveICMessage(message)) {
    ReplyFalse(message);
    return;
  }
  if (message->has_payload() && message->payload().uint32_size() == 2) {
    SetSelectedCandidate(message->payload().uint32(0),
                         message->payload().uint32(1));
  }
  ReplyTrue(message);
}

void UIComponentBase::CandidateListPageDown(int owner, int candidate_list_id) {
  scoped_ptr<ipc::proto::Message> mptr(NewMessage(
      ipc::MSG_CANDIDATE_LIST_PAGE_DOWN, focused_icid_, false));
  mptr->set_target(owner);
  mptr->mutable_payload()->add_uint32(candidate_list_id);
  Send(mptr.release(), NULL);
}

void UIComponentBase::CandidateListPageUp(int owner, int candidate_list_id) {
  scoped_ptr<ipc::proto::Message> mptr(NewMessage(
      ipc::MSG_CANDIDATE_LIST_PAGE_UP, focused_icid_, false));
  mptr->set_target(owner);
  mptr->mutable_payload()->add_uint32(candidate_list_id);
  Send(mptr.release(), NULL);
}

void UIComponentBase::CandidateListShown(int owner, int candidate_list_id) {
  scoped_ptr<ipc::proto::Message> mptr(
      NewMessage(ipc::MSG_CANDIDATE_LIST_SHOWN, focused_icid_, false));
  mptr->set_target(owner);
  mptr->mutable_payload()->add_uint32(candidate_list_id);
  Send(mptr.release(), NULL);
}

void UIComponentBase::CandidateListHidden(int owner, int candidate_list_id) {
  scoped_ptr<ipc::proto::Message> mptr(
      NewMessage(ipc::MSG_CANDIDATE_LIST_HIDDEN, focused_icid_, false));
  mptr->set_target(owner);
  mptr->mutable_payload()->add_uint32(candidate_list_id);
  Send(mptr.release(), NULL);
}

std::string UIComponentBase::ShowMenu(
    const ipc::proto::Rect& location_hint,
    const ipc::proto::CommandList& command_list) {
  scoped_ptr<ipc::proto::Message> mptr(
      NewMessage(ipc::MSG_QUERY_ACTIVE_CONSUMER, focused_icid_, true));
  mptr->mutable_payload()->add_uint32(ipc::MSG_SHOW_MENU);
  ipc::proto::Message* reply = NULL;
  if (!SendWithReplyNonRecursive(mptr.release(), -1, &reply)) {
    DLOG(ERROR) << __SHORT_FUNCTION__ << "SendWithReplyNonRecursive failed";
    return "";
  }
  scoped_ptr<ipc::proto::Message> reply_mptr(reply);
  if (!reply_mptr.get() || !reply_mptr->has_payload() ||
      !reply_mptr->payload().uint32_size()) {
    DLOG(ERROR) << __SHORT_FUNCTION__
                << "Error reply message for MSG_QUERY_ACTIVE_CONSUMER";
    return "";
  }
  int menu_component = reply->payload().uint32(0);
  DCHECK_NE(menu_component, ipc::kComponentBroadcast);
  mptr.reset(NewMessage(ipc::MSG_SHOW_MENU, focused_icid_, true));
  mptr->set_target(menu_component);
  mptr->mutable_payload()->mutable_input_caret()->mutable_rect()->CopyFrom(
      location_hint);
  mptr->mutable_payload()->add_command_list()->CopyFrom(command_list);
  reply = NULL;
  if (!SendWithReplyNonRecursive(mptr.release(), -1, &reply)) {
    DLOG(ERROR) << __SHORT_FUNCTION__ << "SendWithReplyNonRecursive failed";
  }
  reply_mptr.reset(reply);
  if (!reply_mptr.get() || !reply_mptr->has_payload() ||
      !reply_mptr->payload().string_size()) {
    DLOG(ERROR) << __SHORT_FUNCTION__
        << "Error reply message for MSG_SHOW_MENU";
    return "";
  }
  return reply_mptr->payload().string(0);
}

bool UIComponentBase::IsActiveICMessage(
    const ipc::proto::Message* message) const {
  return message->icid() == focused_icid_;
}

}  // namespace components
}  // namespace ime_goopy
