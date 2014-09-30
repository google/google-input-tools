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

#include "ipc/hub_composition_manager.h"

#include "base/logging.h"
#include "base/scoped_ptr.h"
#include "ipc/hub_impl.h"
#include "ipc/message_util.h"

namespace {

// Messages can be consumed by this built-in component.
const uint32 kConsumeMessages[] = {
  ipc::MSG_ATTACH_TO_INPUT_CONTEXT,
  ipc::MSG_DETACHED_FROM_INPUT_CONTEXT,

  ipc::MSG_SET_COMPOSITION,
  ipc::MSG_QUERY_COMPOSITION,

  ipc::MSG_SET_CANDIDATE_LIST,
  ipc::MSG_SET_SELECTED_CANDIDATE,
  ipc::MSG_SET_CANDIDATE_LIST_VISIBILITY,
  ipc::MSG_QUERY_CANDIDATE_LIST,
};

// Messages can be produced by this built-in component.
const uint32 kProduceMessages[] = {
  ipc::MSG_REQUEST_CONSUMER,
  ipc::MSG_COMPOSITION_CHANGED,
  ipc::MSG_CANDIDATE_LIST_CHANGED,
  ipc::MSG_SELECTED_CANDIDATE_CHANGED,
  ipc::MSG_CANDIDATE_LIST_VISIBILITY_CHANGED,
};

const char kStringID[] = "com.google.ime.goopy.ipc.hub.composition-manager";
const char kName[] = "Goopy IPC Hub Composition Manager";

}  // namespace

namespace ipc {
namespace hub {

HubCompositionManager::HubCompositionManager(HubImpl* hub)
    : self_(NULL),
      hub_(hub) {

  // Register this built-in component.
  hub->Attach(this);

  proto::ComponentInfo info;
  info.set_string_id(kStringID);
  info.set_name(kName);

  for (size_t i = 0; i < arraysize(kConsumeMessages); ++i)
    info.add_consume_message(kConsumeMessages[i]);

  for (size_t i = 0; i < arraysize(kProduceMessages); ++i)
    info.add_produce_message(kProduceMessages[i]);

  self_ = hub->CreateComponent(this, info, true);
  DCHECK(self_);
}

HubCompositionManager::~HubCompositionManager() {
  hub_->Detach(this);
  // |self_| will be deleted automatically when detaching from Hub.
}

bool HubCompositionManager::Send(proto::Message* message) {
  scoped_ptr<proto::Message> mptr(message);
  Component* source = hub_->GetComponent(mptr->source());
  DCHECK(source);
  switch (message->type()) {
    case MSG_ATTACH_TO_INPUT_CONTEXT:
      return OnMsgAttachToInputContext(source, mptr.release());
    case MSG_DETACHED_FROM_INPUT_CONTEXT:
      return OnMsgDetachedFromInputContext(source, mptr.release());
    case MSG_SET_COMPOSITION:
      return OnMsgSetComposition(source, mptr.release());
    case MSG_QUERY_COMPOSITION:
      return OnMsgQueryComposition(source, mptr.release());
    case MSG_SET_CANDIDATE_LIST:
      return OnMsgSetCandidateList(source, mptr.release());
    case MSG_SET_SELECTED_CANDIDATE:
      return OnMsgSetSelectedCandidate(source, mptr.release());
    case MSG_SET_CANDIDATE_LIST_VISIBILITY:
      return OnMsgSetCandidateListVisibility(source, mptr.release());
    case MSG_QUERY_CANDIDATE_LIST:
      return OnMsgQueryCandidateList(source, mptr.release());
    default:
      DLOG(ERROR) << "Unexpected message:" << GetMessageName(mptr->type());
      return false;
  }
}

bool HubCompositionManager::OnMsgAttachToInputContext(
    Component* source, proto::Message* message) {
  uint32 icid = message->icid();

  DCHECK_EQ(proto::Message::NEED_REPLY, message->reply_mode());
  hub_->ReplyTrue(source->connector(), message);

  if (icid != kInputContextNone) {
    message = NewMessage(
        MSG_REQUEST_CONSUMER,
        self_->id(), kComponentDefault, icid, false);

    // Although we only produce broadcast messages, we still need at least one
    // component to handle them to show the composition text and candidate list
    // to the user.
    for (size_t i = 0; i < arraysize(kProduceMessages); ++i)
      message->mutable_payload()->add_uint32(kProduceMessages[i]);
    hub_->Dispatch(this, message);
  }
  return true;
}

bool HubCompositionManager::OnMsgDetachedFromInputContext(
    Component* source, proto::Message* message) {
  DCHECK_EQ(proto::Message::NO_REPLY, message->reply_mode());
  scoped_ptr<proto::Message> mptr(message);

  // We no longer provide service to this input context, so just delete all
  // related data.
  const uint32 icid = mptr->icid();
  composition_map_.erase(icid);
  candidate_list_map_.erase(icid);
  return true;
}

bool HubCompositionManager::OnMsgSetComposition(Component* source,
                                                proto::Message* message) {
  if (!hub_->CheckMsgInputContextAndSourceAttached(source, message))
    return true;

  const uint32 icid = message->icid();
  bool changed = false;
  proto::Composition* composition = NULL;
  if (!message->has_payload() || !message->payload().has_composition()) {
    changed = (composition_map_.erase(icid) != 0);
  } else {
    changed = true;
    composition = &composition_map_[icid];
    composition->Swap(message->mutable_payload()->mutable_composition());
  }

  if (changed)
    BroadcastCompositionChanged(icid, composition);
  return hub_->ReplyTrue(source->connector(), message);
}

bool HubCompositionManager::OnMsgQueryComposition(Component* source,
                                                  proto::Message* message) {
  if (!hub_->CheckMsgNeedReply(source, message) ||
      !hub_->CheckMsgInputContext(source, message)) {
    return true;
  }

  const uint32 icid = message->icid();

  ConvertToReplyMessage(message);
  proto::MessagePayload* payload = message->mutable_payload();
  payload->Clear();

  CompositionMap::const_iterator iter = composition_map_.find(icid);
  if (iter != composition_map_.end())
    payload->mutable_composition()->CopyFrom(iter->second);
  source->connector()->Send(message);
  return true;
}

bool HubCompositionManager::OnMsgSetCandidateList(Component* source,
                                                  proto::Message* message) {
  if (!hub_->CheckMsgInputContextAndSourceAttached(source, message))
    return true;

  const uint32 icid = message->icid();
  bool changed = false;
  proto::CandidateList* candidate_list = NULL;
  if (!message->has_payload() || !message->payload().has_candidate_list()) {
    changed = (candidate_list_map_.erase(icid) != 0);
  } else {
    changed = true;
    std::pair<proto::CandidateList, uint32>* p = &candidate_list_map_[icid];
    candidate_list = &p->first;
    candidate_list->Swap(message->mutable_payload()->mutable_candidate_list());
    p->second = candidate_list->id();
    SetCandidateListOwner(source->id(), candidate_list);
  }

  if (changed)
    BroadcastCandidateListChanged(icid, candidate_list);
  return hub_->ReplyTrue(source->connector(), message);
}

bool HubCompositionManager::OnMsgSetSelectedCandidate(Component* source,
                                                      proto::Message* message) {
  if (!hub_->CheckMsgInputContextAndSourceAttached(source, message))
    return true;

  if (!message->has_payload() || message->payload().uint32_size() != 2) {
    return hub_->ReplyError(
        source->connector(), message, proto::Error::INVALID_PAYLOAD);
  }

  const uint32 icid = message->icid();
  CandidateListMap::iterator iter = candidate_list_map_.find(icid);
  if (iter == candidate_list_map_.end())
    return hub_->ReplyFalse(source->connector(), message);

  const uint32 candidate_list_id = message->payload().uint32(0);
  const uint32 candidate_id = message->payload().uint32(1);

  proto::CandidateList* candidate_list =
      FindCandidateList(&iter->second.first, candidate_list_id);

  // Only owner of the candidate list can change the selected candidate.
  if (!candidate_list || candidate_list->owner() != source->id())
    return hub_->ReplyFalse(source->connector(), message);

  bool changed = false;
  // Checks if the selected candidate list gets changed.
  if (iter->second.second != candidate_list_id) {
    changed = true;
    iter->second.second = candidate_list_id;
  }

  // Checks if the selected candidate gets changed.
  if (candidate_id < candidate_list->candidate_size() &&
      (!candidate_list->has_selected_candidate() ||
       candidate_list->selected_candidate() != candidate_id)) {
    changed = true;
    candidate_list->set_selected_candidate(candidate_id);
  } else if (candidate_list->has_selected_candidate() &&
             candidate_id >= candidate_list->candidate_size()) {
    changed = true;
    candidate_list->clear_selected_candidate();
  }

  if (changed)
    BroadcastSelectedCandidateChanged(icid, candidate_list_id, candidate_id);
  return hub_->ReplyTrue(source->connector(), message);
}

bool HubCompositionManager::OnMsgSetCandidateListVisibility(
    Component* source, proto::Message* message) {
  if (!hub_->CheckMsgInputContextAndSourceAttached(source, message))
    return true;

  if (!message->has_payload() || message->payload().uint32_size() != 1 ||
      message->payload().boolean_size() != 1) {
    return hub_->ReplyError(
        source->connector(), message, proto::Error::INVALID_PAYLOAD);
  }

  const uint32 icid = message->icid();
  CandidateListMap::iterator iter = candidate_list_map_.find(icid);
  if (iter == candidate_list_map_.end())
    return hub_->ReplyFalse(source->connector(), message);

  const uint32 candidate_list_id = message->payload().uint32(0);
  const bool visible = message->payload().boolean(0);

  proto::CandidateList* candidate_list =
      FindCandidateList(&iter->second.first, candidate_list_id);

  // Only owner of the candidate list can change the visibility.
  if (!candidate_list || candidate_list->owner() != source->id())
    return hub_->ReplyFalse(source->connector(), message);

  if (candidate_list->visible() != visible) {
    candidate_list->set_visible(visible);
    BroadcastCandidateListVisibilityChanged(icid, candidate_list_id, visible);
  }
  return hub_->ReplyTrue(source->connector(), message);
}

bool HubCompositionManager::OnMsgQueryCandidateList(Component* source,
                                                    proto::Message* message) {
  if (!hub_->CheckMsgNeedReply(source, message) ||
      !hub_->CheckMsgInputContext(source, message)) {
    return true;
  }

  Connector* connector = source->connector();
  const uint32 icid = message->icid();

  ConvertToReplyMessage(message);
  proto::MessagePayload* payload = message->mutable_payload();
  payload->Clear();

  CandidateListMap::const_iterator iter = candidate_list_map_.find(icid);
  if (iter != candidate_list_map_.end()) {
    payload->mutable_candidate_list()->CopyFrom(iter->second.first);
    payload->add_uint32(iter->second.second);
  }
  connector->Send(message);
  return true;
}

void HubCompositionManager::BroadcastCompositionChanged(
    uint32 icid,
    const proto::Composition* composition) {
  InputContext* ic = hub_->GetInputContext(icid);
  if (!ic || !ic->MayConsume(MSG_COMPOSITION_CHANGED, false))
    return;

  proto::Message* message = NewMessage(
      MSG_COMPOSITION_CHANGED,
      self_->id(), kComponentBroadcast, icid, false);

  if (composition)
    message->mutable_payload()->mutable_composition()->CopyFrom(*composition);
  hub_->Dispatch(this, message);
}

void HubCompositionManager::BroadcastCandidateListChanged(
    uint32 icid,
    const proto::CandidateList* candidates) {
  InputContext* ic = hub_->GetInputContext(icid);
  if (!ic || !ic->MayConsume(MSG_CANDIDATE_LIST_CHANGED, false))
    return;

  proto::Message* message = NewMessage(
      MSG_CANDIDATE_LIST_CHANGED,
      self_->id(), kComponentBroadcast, icid, false);

  if (candidates)
    message->mutable_payload()->mutable_candidate_list()->CopyFrom(*candidates);
  hub_->Dispatch(this, message);
}

void HubCompositionManager::BroadcastSelectedCandidateChanged(
    uint32 icid, uint32 candidate_list_id, uint32 candidate_id) {
  InputContext* ic = hub_->GetInputContext(icid);
  if (!ic || !ic->MayConsume(MSG_SELECTED_CANDIDATE_CHANGED, false))
    return;

  proto::Message* message = NewMessage(
      MSG_SELECTED_CANDIDATE_CHANGED,
      self_->id(), kComponentBroadcast, icid, false);

  proto::MessagePayload* payload = message->mutable_payload();
  payload->add_uint32(candidate_list_id);
  payload->add_uint32(candidate_id);
  hub_->Dispatch(this, message);
}

void HubCompositionManager::BroadcastCandidateListVisibilityChanged(
    uint32 icid, uint32 candidate_list_id, bool visible) {
  InputContext* ic = hub_->GetInputContext(icid);
  if (!ic || !ic->MayConsume(MSG_CANDIDATE_LIST_VISIBILITY_CHANGED, false))
    return;

  proto::Message* message = NewMessage(
      MSG_CANDIDATE_LIST_VISIBILITY_CHANGED,
      self_->id(), kComponentBroadcast, icid, false);

  proto::MessagePayload* payload = message->mutable_payload();
  payload->add_uint32(candidate_list_id);
  payload->add_boolean(visible);
  hub_->Dispatch(this, message);
}

proto::CandidateList* HubCompositionManager::FindCandidateList(
    proto::CandidateList* top, uint32 id) {
  if (top->id() == id)
    return top;

  // Find in sub candidate lists
  for (int i = 0; i < top->candidate_size(); ++i) {
    proto::Candidate* candidate = top->mutable_candidate(i);
    if (candidate->has_sub_candidates()) {
      proto::CandidateList* sub =
          FindCandidateList(candidate->mutable_sub_candidates(), id);
      if (sub)
        return sub;
    }
  }
  return NULL;
}

// static
void HubCompositionManager::SetCandidateListOwner(
    uint32 owner, proto::CandidateList* candidates) {
  candidates->set_owner(owner);
  for (int i = 0; i < candidates->candidate_size(); ++i) {
    proto::Candidate* candidate = candidates->mutable_candidate(i);
    if (candidate->has_sub_candidates())
      SetCandidateListOwner(owner, candidate->mutable_sub_candidates());
  }
}

}  // namespace hub
}  // namespace ipc
