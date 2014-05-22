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

#include "tsf/candidates.h"

#include "base/logging.h"
#include "common/debug.h"
#include "common/framework_interface.h"
#include "common/smart_com_ptr.h"
#include "tsf/external_candidate_ui.h"

namespace ime_goopy {
namespace tsf {
Candidates::Candidates(ITfThreadMgr* thread_manager,
                       EngineInterface* engine,
                       UIManagerInterface* ui_manager)
    : engine_(engine), ui_manager_(ui_manager), status_(STATUS_HIDDEN) {
  assert(engine_);
  assert(ui_manager);

  HRESULT hr = E_FAIL;
  // External Candidate UI. This component is supported since Windows Vista.
  // In earlier version of Windows, the creation will fail and it's OK.
  hr = external_candidate_ui_.CreateInstance();
  if (FAILED(hr)) {
    DVLOG(1) << __SHORT_FUNCTION__
             << L" Can't create ExternalCandidateUI."
             << L" hr: " << hr;
    return;
  }

  // Get ITfUIElementMgr.
  SmartComPtr<ITfUIElementMgr> ui_element_mgr(thread_manager);
  if (!ui_element_mgr) {
    DVLOG(1) << __SHORT_FUNCTION__
             << L" Can't get ITfUIElementMgr.";
    return;
  }

  // Get ITfDocumentMgr.
  SmartComPtr<ITfDocumentMgr> document_manager;
  hr = thread_manager->GetFocus(document_manager.GetAddress());
  if (FAILED(hr) || !document_manager) {
    DVLOG(1) << __SHORT_FUNCTION__
             << L" Can't get ITfDocumentMgr."
             << L" hr: " << hr;
  }

  // Initialize.
  hr = external_candidate_ui_->Initialize(ui_element_mgr,
                                          document_manager,
                                          engine,
                                          this);
  if (FAILED(hr)) {
    external_candidate_ui_->Uninitialize();
    external_candidate_ui_.Release();
  }
}

Candidates::~Candidates() {
  // External candidate UI.
  if (external_candidate_ui_) {
    external_candidate_ui_->Uninitialize();
    external_candidate_ui_.Release();
  }
}

HRESULT Candidates::Update(const ipc::proto::CandidateList& candidate_list) {
  assert(engine_);
  assert(ui_manager_);
  candidate_list_.CopyFrom(candidate_list);
  switch (status_) {
    case STATUS_HIDDEN:
      if (!IsEmpty()) {
        if (external_candidate_ui_) {
          external_candidate_ui_->BeginUI();
          if (external_candidate_ui_->ShouldShow()) {
            status_ = STATUS_SHOWN_UI_MANAGER;
          } else {
            external_candidate_ui_->UpdateUI();
            status_ = STATUS_SHOWN_EXTERNAL;
          }
        } else {
          status_ = STATUS_SHOWN_UI_MANAGER;
        }
      }
      break;
    case STATUS_SHOWN_EXTERNAL:
      external_candidate_ui_->UpdateUI();
      if (IsEmpty()) {
        external_candidate_ui_->EndUI();
        status_ = STATUS_HIDDEN;
      }
      break;
    case STATUS_SHOWN_UI_MANAGER:
      if (IsEmpty()) {
        if (external_candidate_ui_) external_candidate_ui_->EndUI();
        status_ = STATUS_HIDDEN;
      }
      break;
  }
  return S_OK;
}

bool Candidates::IsEmpty() {
  return candidate_list_.candidate_size() == 0;
}

}  // namespace tsf
}  // namespace ime_goopy
