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

#ifndef GOOPY_TSF_EXTERNAL_CANDIDATE_UI_H_
#define GOOPY_TSF_EXTERNAL_CANDIDATE_UI_H_

#include <atlbase.h>
#include <atlcom.h>
#include <msctf.h>
#include <vector>

#include "base/basictypes.h"
#include "base/scoped_ptr.h"
#include "common/smart_com_ptr.h"
#include <gtest/gtest_prod.h>

namespace ime_goopy {
class EngineInterface;
namespace tsf {
class Candidates;
// This class implemented UI Less mode in TSF. In this mode, applications like
// games and command prompt draw the candidate UI themselves. We should provide
// data to them to support their UI. This mode is supported in TSF after
// Windows Vista, so in earlier version of windows, TSF text service can not be
// used in those applications.
class ATL_NO_VTABLE ExternalCandidateUI
    : public CComObjectRootEx<CComSingleThreadModel>,
      public ITfCandidateListUIElementBehavior {
 public:
  DECLARE_NOT_AGGREGATABLE(ExternalCandidateUI)
  BEGIN_COM_MAP(ExternalCandidateUI)
    COM_INTERFACE_ENTRY(ITfUIElement)
    COM_INTERFACE_ENTRY(ITfCandidateListUIElement)
    COM_INTERFACE_ENTRY(ITfCandidateListUIElementBehavior)
  END_COM_MAP()
  DECLARE_PROTECT_FINAL_CONSTRUCT()

  ExternalCandidateUI();
  ~ExternalCandidateUI();

  HRESULT Initialize(ITfUIElementMgr* ui_element_mgr,
                     ITfDocumentMgr* document_mgr,
                     EngineInterface* engine,
                     Candidates* owner);
  HRESULT Uninitialize();

  // We must call BeginUI before showing the candidate list and EndUI after
  // hiding it. This will tell the system what happened to the UI. If show_ui
  // changed to FALSE after BeginUI, we should not show our own UI and use
  // ExternalCandidateUI as the current UI.
  HRESULT BeginUI();
  bool ShouldShow();  // If we should show own own UI.
  HRESULT UpdateUI();
  HRESULT EndUI();

  // ITfUIElement
  STDMETHODIMP GetDescription(BSTR* description);
  STDMETHODIMP GetGUID(GUID *guid);
  STDMETHODIMP Show(BOOL show);
  STDMETHODIMP IsShown(BOOL *show);

  // ITfCandidateListUIElement
  STDMETHODIMP GetUpdatedFlags(DWORD *flags);
  STDMETHODIMP GetDocumentMgr(ITfDocumentMgr **document_manager);
  STDMETHODIMP GetCount(UINT *count);
  STDMETHODIMP GetSelection(UINT *index);
  STDMETHODIMP GetString(UINT index, BSTR *text);
  STDMETHODIMP GetPageIndex(UINT* index, UINT size, UINT* page_count);
  STDMETHODIMP SetPageIndex(UINT* index, UINT page_count);
  STDMETHODIMP GetCurrentPage(UINT* current_page);

  // ITfCandidateListUIElementBehavior
  STDMETHODIMP SetSelection(UINT index);
  STDMETHODIMP Finalize();
  STDMETHODIMP Abort();

  FRIEND_TEST(ExternalCandidateUI, BeginEndUI);

 private:
  SmartComPtr<ITfUIElementMgr> ui_element_mgr_;
  SmartComPtr<ITfDocumentMgr> document_mgr_;
  EngineInterface* engine_;
  DWORD changed_flags_;
  DWORD ui_id_;
  BOOL show_ui_;
  Candidates* owner_;
};

}  // namespace tsf
}  // namespace ime_goopy
#endif  // GOOPY_TSF_EXTERNAL_CANDIDATE_UI_H_
