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

#ifndef GOOPY_IPC_HUB_COMPOSITION_MANAGER_H_
#define GOOPY_IPC_HUB_COMPOSITION_MANAGER_H_

#include <map>
#include <utility>

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "ipc/hub.h"
#include "ipc/protos/ipc.pb.h"

namespace ipc {
namespace hub {

class HubImpl;
class Component;
class InputContext;

// A built-in component for managing composition text and candidate list of all
// input contexts.
class HubCompositionManager : public Hub::Connector {
 public:
  explicit HubCompositionManager(HubImpl* hub);
  virtual ~HubCompositionManager();

  // Implementation of Hub::Connector interface
  virtual bool Send(proto::Message* message) OVERRIDE;

 private:
  // Key: input context id
  typedef std::map<uint32, proto::Composition> CompositionMap;

  // Key: input context id
  // Value:
  //  first: Toplevel CandidateList object,
  //  second: Id of the currently selected CandidateList object.
  typedef std::map<uint32, std::pair<proto::CandidateList, uint32> >
      CandidateListMap;

  // Message handlers.
  bool OnMsgAttachToInputContext(Component* source, proto::Message* message);
  bool OnMsgDetachedFromInputContext(Component* source,
                                     proto::Message* message);

  // Composition text related messages.
  bool OnMsgSetComposition(Component* source, proto::Message* message);
  bool OnMsgQueryComposition(Component* source, proto::Message* message);

  // Candidate list related messages.
  bool OnMsgSetCandidateList(Component* source, proto::Message* message);
  bool OnMsgSetSelectedCandidate(Component* source, proto::Message* message);
  bool OnMsgSetCandidateListVisibility(Component* source,
                                       proto::Message* message);
  bool OnMsgQueryCandidateList(Component* source, proto::Message* message);

  // Broadcast MSG_COMPOSITION_CHANGED message.
  // |composition| == NULL means that the composition was cleared.
  void BroadcastCompositionChanged(uint32 icid,
                                   const proto::Composition* composition);

  // Broadcast MSG_CANDIDATE_LIST_CHANGED message.
  // |candidates| == NULL means that the candidate list was cleared.
  void BroadcastCandidateListChanged(uint32 icid,
                                     const proto::CandidateList* candidates);

  // Broadcast MSG_SELECTED_CANDIDATE_CHANGED message.
  void BroadcastSelectedCandidateChanged(uint32 icid,
                                         uint32 candidate_list_id,
                                         uint32 candidate_id);

  // Broadcast MSG_CANDIDATE_LIST_VISIBILITY_CHANGED message.
  void BroadcastCandidateListVisibilityChanged(uint32 icid,
                                               uint32 candidate_list_id,
                                               bool visible);

  // Find the CandidateList object in a candidate list tree by given
  // candidate list id. Returns NULL if the candidate list cannot be found.
  proto::CandidateList* FindCandidateList(proto::CandidateList* top, uint32 id);

  // Sets the owner of a CandidateList object recursively.
  static void SetCandidateListOwner(uint32 owner,
                                    proto::CandidateList* candidates);

  // The Component object representing the command list manager itself.
  Component* self_;

  // Weak pointer to our owner.
  HubImpl* hub_;

  CompositionMap composition_map_;
  CandidateListMap candidate_list_map_;

  DISALLOW_COPY_AND_ASSIGN(HubCompositionManager);
};

}  // namespace hub
}  // namespace ipc

#endif  // GOOPY_IPC_HUB_COMPOSITION_MANAGER_H_
