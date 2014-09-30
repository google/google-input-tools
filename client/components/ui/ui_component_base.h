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

#ifndef GOOPY_COMPONENTS_UI_UI_COMPONENT_BASE_H_
#define GOOPY_COMPONENTS_UI_UI_COMPONENT_BASE_H_

#include <map>
#include <string>
#include <vector>

#include "base/scoped_ptr.h"
#include "ipc/component_base.h"
#include "ipc/protos/ipc.pb.h"

namespace google {
namespace protobuf {
template <class T> class RepeatedPtrField;
}  // namespace protobuf
}  // namespace google

namespace ipc {
class ComponentHost;
}
namespace ime_goopy {
namespace components {

// Base class of platform independent UI component implementations. It will
// handle all the IPC message related work.
class UIComponentBase : public ipc::ComponentBase {
 public:
  UIComponentBase();
  virtual ~UIComponentBase();

  // Overridden from Component: derived classes should not override this two
  // message anymore.
  virtual void GetInfo(ipc::proto::ComponentInfo* info) OVERRIDE;
  virtual void Handle(ipc::proto::Message* message) OVERRIDE;

 protected:
  typedef google::protobuf::RepeatedPtrField<ipc::proto::CommandList>
          CommandLists;
  typedef google::protobuf::RepeatedPtrField<ipc::proto::ComponentInfo>
          ComponentInfos;
  // Methods that will be called by derived classes.
  bool DoCommand(int owner, int icid, const std::string& id);
  bool DoCandidateCommand(
      int owner,
      int icid,
      int candidate_list_id,
      int candidate_index,
      const std::string& id);
  void SelectCandidate(
      int owner,
      int candidate_list_id,
      int candidate_index,
      bool commit);
  // Callback function when a input method is selected through toolbar UI.
  void SelectInputMethod(int new_input_method_id);
  // Callback functions of candidate page up/down button.
  void CandidateListPageDown(int owner, int candidate_list_id);
  void CandidateListPageUp(int owner, int candidate_list_id);
  // Callback functions to send related messages.
  void CandidateListShown(int owner, int candidate_list_id);
  void CandidateListHidden(int owner, int candidate_list_id);
  // Queries the information of the focused input context and refresh the UI.
  void RefreshUI();
  // Sends message to win menu component to show the menu in application's
  // process.
  std::string ShowMenu(const ipc::proto::Rect& location_hint,
                       const ipc::proto::CommandList& command_list);
  int GetFocusedIcid() const {
    return focused_icid_;
  }

  // Virtual methods need to be implemented by derived classes.

  // Returns component name.
  virtual std::string GetComponentName() = 0;
  // Returns component string id.
  virtual std::string GetComponentStringID() = 0;
  // Set the current composition text to show.
  // Calling with |composition| set to NULL will clear current candidate list.
  virtual void SetComposition(const ipc::proto::Composition* composition) = 0;
  // Sets the current candidate list.
  // Calling with |list| set to NULL will clear current candidate list.
  virtual void SetCandidateList(const ipc::proto::CandidateList* list) = 0;
  // Set current selected candidate.
  virtual void SetSelectedCandidate(int candidate_list_id,
                                    int candidate_index) = 0;
  // Shows or hides the composition UI.
  virtual void SetCompositionVisibility(bool visible) = 0;
  // Shows or hides the candidate list UI.
  virtual void SetCandidateListVisibility(bool visible) = 0;
  // Shows or hides the toolbar list UI.
  virtual void SetToolbarVisibility(bool visible) = 0;
  // Sets the current command list (usually to the toolbar UI).
  virtual void SetCommandList(const CommandLists& command_lists) = 0;
  // Changes the visibility of the candidate list identified by |id|.
  // See the definition of MSG_CANDIDATE_LIST_VISIBILITIES CHANGED for detail.
  virtual void ChangeCandidateListVisibility(int id, bool visible) = 0;
  // Sets the current input method list (usually to toolbar UI).
  virtual void SetInputMethods(const ComponentInfos& components) = 0;
  // Sets the current active input method.
  virtual void SetActiveInputMethod(
      const ipc::proto::ComponentInfo& component) = 0;
  // Set the information of current input caret. The derived class will use
  // this information to determine the position of composition and candidate
  // list UI.
  virtual void SetInputCaret(const ipc::proto::InputCaret& caret) = 0;

 private:
  void OnMsgAttachToInputContext(ipc::proto::Message* message);
  void OnMsgDetachedFromInputContext(ipc::proto::Message* message);
  void OnMsgInputContextDeleted(ipc::proto::Message* message);
  void OnMsgInputContextGotFocus(ipc::proto::Message* message);
  void OnMsgInputContextLostFocus(ipc::proto::Message* message);
  void OnMsgCompositionChanged(ipc::proto::Message* message);
  void OnMsgCandidateListChanged(ipc::proto::Message* message);
  void OnMsgUpdateInputCaret(ipc::proto::Message* message);
  void OnMsgCommandListChanged(ipc::proto::Message* message);
  void OnMsgInputMethodActivated(ipc::proto::Message* message);
  void OnMsgShowCompositionUI(ipc::proto::Message* message);
  void OnMsgHideCompositionUI(ipc::proto::Message* message);
  void OnMsgShowCandidateListUI(ipc::proto::Message* message);
  void OnMsgHideCandidateListUI(ipc::proto::Message* message);
  void OnMsgShowToolBarUI(ipc::proto::Message* message);
  void OnMsgHideToolBarUI(ipc::proto::Message* message);
  void OnMsgCandidateListVisibilityChanged(ipc::proto::Message* message);
  void OnMsgSelectedCandidateChanged(ipc::proto::Message* message);
  // Returns true if the message is for the active input context.
  bool IsActiveICMessage(const ipc::proto::Message* message) const;

  int focused_icid_;
  DISALLOW_COPY_AND_ASSIGN(UIComponentBase);
};

}  // namespace components
}  // namespace ime_goopy

#endif  // GOOPY_COMPONENTS_UI_UI_COMPONENT_BASE_H_
