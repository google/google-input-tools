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

#include "tsf/external_candidate_ui.h"

#include "base/callback.h"
#include "base/logging.h"
#include "base/string_utils_win.h"
#include "common/debug.h"
#include "common/framework_interface.h"
#include "tsf/candidates.h"

namespace ime_goopy {
namespace tsf {
// {FCE65EEA-FF4F-4202-9FF2-F5E9A598D264}
static const GUID kCandidateListUIGuid = {
  0xfce65eea,
  0xff4f,
  0x4202,
  {0x9f, 0xf2, 0xf5, 0xe9, 0xa5, 0x98, 0xd2, 0x64}
};
static const wchar_t kDescription[] = L"ExternalCandidateUI";

ExternalCandidateUI::ExternalCandidateUI()
    : engine_(NULL),
      changed_flags_(0),
      ui_id_(TF_INVALID_UIELEMENTID),
      show_ui_(TRUE),
      owner_(NULL) {
}

ExternalCandidateUI::~ExternalCandidateUI() {
  EndUI();
  Uninitialize();
}

HRESULT ExternalCandidateUI::Initialize(ITfUIElementMgr* ui_element_mgr,
                                        ITfDocumentMgr* document_mgr,
                                        EngineInterface* engine,
                                        Candidates* owner) {
  DVLOG(1) << __FUNCTION__;
  assert(engine);

  // We don't care if we can get the ui_element_mgr interface. Because this
  // interface only exists from Windows Vista. On Windows XP or earlier
  // platform, the ui_manager_ will be NULL and ExternalCandidateUI will be
  // disabled. In this case, BeginUI will always set show_ui as true to enable
  // our own candidate UI.
  ui_element_mgr_ = ui_element_mgr;
  document_mgr_ = document_mgr;
  engine_ = engine;
  changed_flags_ = 0;
  ui_id_ = TF_INVALID_UIELEMENTID;
  owner_ = owner;
  return S_OK;
}

HRESULT ExternalCandidateUI::Uninitialize() {
  DVLOG(1) << __FUNCTION__;
  document_mgr_ = NULL;
  ui_element_mgr_ = NULL;
  engine_ = NULL;
  changed_flags_ = 0;
  ui_id_ = TF_INVALID_UIELEMENTID;
  owner_ = NULL;
  return S_OK;
}

HRESULT ExternalCandidateUI::BeginUI() {
  DVLOG(1) << __FUNCTION__;
  if (ui_id_ != TF_INVALID_UIELEMENTID) return E_UNEXPECTED;

  show_ui_ = TRUE;
  // If the current operating system doesn't support ITfUIElement interface,
  // we will show our own.
  if (!ui_element_mgr_) return S_OK;

  if (FAILED(ui_element_mgr_->BeginUIElement(this, &show_ui_, &ui_id_))) {
    show_ui_ = TRUE;
    ui_id_ = TF_INVALID_UIELEMENTID;
    return S_OK;
  }

  return S_OK;
}

bool ExternalCandidateUI::ShouldShow() {
  return show_ui_ == TRUE;
}

HRESULT ExternalCandidateUI::UpdateUI() {
  DVLOG(1) << __FUNCTION__;
  if (!ui_element_mgr_) return S_OK;
  if (ui_id_ == TF_INVALID_UIELEMENTID) return E_UNEXPECTED;
  changed_flags_= TF_CLUIE_DOCUMENTMGR |
                  TF_CLUIE_STRING |
                  TF_CLUIE_SELECTION |
                  TF_CLUIE_CURRENTPAGE |
                  TF_CLUIE_PAGEINDEX |
                  TF_CLUIE_COUNT;
  return ui_element_mgr_->UpdateUIElement(ui_id_);
}

HRESULT ExternalCandidateUI::EndUI() {
  DVLOG(1) << __FUNCTION__;
  if (ui_id_ == TF_INVALID_UIELEMENTID) return E_UNEXPECTED;
  if (ui_element_mgr_) {
    UpdateUI();
    ui_element_mgr_->EndUIElement(ui_id_);
  }
  ui_id_ = TF_INVALID_UIELEMENTID;
  show_ui_ = TRUE;
  return S_OK;
}

// ITfUIElement
HRESULT ExternalCandidateUI::GetDescription(BSTR* description) {
  DVLOG(1) << __FUNCTION__;
  if (!description) return E_INVALIDARG;

  *description = SysAllocString(kDescription);
  return *description ? S_OK : E_FAIL;
}

HRESULT ExternalCandidateUI::GetGUID(GUID *guid) {
  DVLOG(1) << __FUNCTION__;
  if (!guid) return E_INVALIDARG;

  *guid = kCandidateListUIGuid;
  return S_OK;
}

HRESULT ExternalCandidateUI::Show(BOOL show) {
  DVLOG(1) << __FUNCTION__;
  // I don't implement this function because I haven't find any application
  // which called this function. And it doesn't make sense to switch between
  // candidate UIs while the UI is alerady shown.
  return E_NOTIMPL;
}

// Returns true if the UI is currently shown by a text service.
HRESULT ExternalCandidateUI::IsShown(BOOL *show) {
  DVLOG(1) << __FUNCTION__;
  if (!show) return E_INVALIDARG;
  // If ui_id_ is valid, means we are showing a candidate ui. show_ui_ means we
  // should show the ui via text service's own window.
  *show = ((ui_id_ != TF_INVALID_UIELEMENTID) && show_ui_);
  return S_OK;
}


// ITfCandidateListUIElement
HRESULT ExternalCandidateUI::GetUpdatedFlags(DWORD *flags) {
  DVLOG(1) << __FUNCTION__;
  if (!flags) return E_INVALIDARG;

  *flags = changed_flags_;

  // TSF call GetUpdatedFlags to ask what is changed after the previous call.
  // We clear the flags after TSF got the changed information, so that the next
  // call to this function will return the changes from now on.
  changed_flags_ = 0;
  return S_OK;
}

HRESULT ExternalCandidateUI::GetDocumentMgr(ITfDocumentMgr** document_manager) {
  DVLOG(1) << __FUNCTION__;
  if (!document_manager) return E_INVALIDARG;

  *document_manager = document_mgr_;
  return S_OK;
}

HRESULT ExternalCandidateUI::GetCount(UINT *count) {
  DVLOG(1) << __FUNCTION__;
  if (!count) return E_INVALIDARG;
  assert(owner_);

  *count = owner_->candidate_list().candidate_size();

  return S_OK;
}

HRESULT ExternalCandidateUI::GetSelection(UINT *index) {
  DVLOG(1) << __FUNCTION__;
  if (!index) return E_INVALIDARG;
  assert(owner_);

  *index = owner_->candidate_list().selected_candidate();
  return S_OK;
}

HRESULT ExternalCandidateUI::GetString(UINT index, BSTR* text) {
  DVLOG(3) << __SHORT_FUNCTION__
           << L" index: " << index;
  if (!text) return E_INVALIDARG;
  assert(owner_);
  const ipc::proto::CandidateList& candidates = owner_->candidate_list();
  assert(index < candidates.candidate_size());

  *text = SysAllocString(
      Utf8ToWide(candidates.candidate(index).actual_text().text()).c_str());
  return *text ? S_OK : E_FAIL;
}

HRESULT ExternalCandidateUI::GetPageIndex(UINT* index,
                                         UINT size,
                                         UINT* page_count) {
  DVLOG(1) << __FUNCTION__;
  if (!page_count) return E_INVALIDARG;
  assert(owner_);

  // The paging logic is handled at engine, we send only one page to the system
  // at a time. So the page count here is always 1.
  *page_count = 1;
  if (!index) return S_OK;

  if (size >= 2) {
    index[0] = 0;
    index[1] = owner_->candidate_list().candidate_size();
  }
  return S_OK;
}

HRESULT ExternalCandidateUI::SetPageIndex(UINT* index, UINT page_count) {
  DVLOG(1) << __FUNCTION__;
  if (!index || !page_count) return E_INVALIDARG;
  if (page_count == 1) return S_OK;
  assert(engine_);
  engine_->ResizeCandidatePage(index[1]);
  return S_OK;
}

HRESULT ExternalCandidateUI::GetCurrentPage(UINT* current_page) {
  DVLOG(1) << __FUNCTION__;
  if (!current_page) return E_INVALIDARG;
  *current_page = 0;
  return S_OK;
}

// ITfCandidateListUIElementBehavior
HRESULT ExternalCandidateUI::SetSelection(UINT index) {
  DVLOG(1) << __FUNCTION__;
  assert(engine_);
  engine_->SelectCandidate(index, false);

  return S_OK;
}

HRESULT ExternalCandidateUI::Finalize() {
  DVLOG(1) << __FUNCTION__;
  assert(engine_);
  engine_->EndComposition(true);
  return S_OK;
}

HRESULT ExternalCandidateUI::Abort() {
  DVLOG(1) << __FUNCTION__;
  assert(engine_);
  engine_->EndComposition(false);
  return S_OK;
}
}  // namespace tsf
}  // namespace ime_goopy
