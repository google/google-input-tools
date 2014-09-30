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

#include <atlbase.h>
#include <atlcom.h>
#include <atlcomcli.h>
#include <msctf.h>

#include "common/mock_engine.h"
#include "tsf/external_candidate_ui.h"
#include <gtest/gtest_prod.h>

namespace ime_goopy {
namespace tsf {
static const DWORD kUIID = 1;

class ATL_NO_VTABLE MockUIElementMgr
    : public CComObjectRootEx<CComSingleThreadModel>,
      public ITfUIElementMgr {
 public:
  DECLARE_NOT_AGGREGATABLE(MockUIElementMgr)
  BEGIN_COM_MAP(MockUIElementMgr)
    COM_INTERFACE_ENTRY(ITfUIElementMgr)
  END_COM_MAP()
  DECLARE_PROTECT_FINAL_CONSTRUCT()

  HRESULT STDMETHODCALLTYPE BeginUIElement(
      ITfUIElement* element,
      BOOL* show,
      DWORD* ui_id) {
    *show = FALSE;
    *ui_id = kUIID;
    return S_OK;
  }

  HRESULT STDMETHODCALLTYPE UpdateUIElement(DWORD ui_id) {
    return S_OK;
  }

  HRESULT STDMETHODCALLTYPE EndUIElement(DWORD ui_id) {
    return S_OK;
  }

  HRESULT STDMETHODCALLTYPE GetUIElement(DWORD ui_id, ITfUIElement **element) {
    return S_OK;
  }

  HRESULT STDMETHODCALLTYPE EnumUIElements(IEnumTfUIElements **enumerator) {
    return S_OK;
  }

 private:
};

TEST(ExternalCandidateUI, BeginEndUI) {
  CComObjectStackEx<ExternalCandidateUI> ui;
  BOOL is_shown = FALSE;

  // No UI element manager.
  EXPECT_TRUE(SUCCEEDED(ui.BeginUI()));
  EXPECT_TRUE(ui.ShouldShow());

  CComObjectStackEx<MockUIElementMgr> manager;
  MockEngine engine;
  ui.Initialize(&manager, NULL, &engine);

  // UI element manager available.
  EXPECT_TRUE(SUCCEEDED(ui.BeginUI()));
  EXPECT_FALSE(ui.ShouldShow());
  EXPECT_EQ(kUIID, ui.ui_id_);
  EXPECT_TRUE(SUCCEEDED(ui.IsShown(&is_shown)));
  EXPECT_FALSE(is_shown);

  EXPECT_TRUE(SUCCEEDED(ui.UpdateUI()));
  DWORD changed_flags = 0;
  EXPECT_TRUE(SUCCEEDED(ui.GetUpdatedFlags(&changed_flags)));
  EXPECT_NE(0, changed_flags);
  EXPECT_TRUE(SUCCEEDED(ui.GetUpdatedFlags(&changed_flags)));
  EXPECT_EQ(0, changed_flags);

  // End UI.
  EXPECT_TRUE(SUCCEEDED(ui.EndUI()));
  EXPECT_EQ(TF_INVALID_UIELEMENTID, ui.ui_id_);
  EXPECT_TRUE(SUCCEEDED(ui.IsShown(&is_shown)));
  EXPECT_FALSE(is_shown);

  ui.Uninitialize();
}

TEST(ExternalCandidateUI, ITfUIElement) {
  CComObjectStackEx<ExternalCandidateUI> ui;

  CComBSTR description;
  EXPECT_TRUE(SUCCEEDED(ui.GetDescription(&description)));
  EXPECT_NE(0, description.Length());

  GUID guid;
  EXPECT_EQ(E_INVALIDARG, ui.GetGUID(NULL));
  EXPECT_TRUE(SUCCEEDED(ui.GetGUID(&guid)));
}

TEST(ExternalCandidateUI, ITfCandidateListUIElement) {
  CComObjectStackEx<ExternalCandidateUI> ui;
  CComObjectStackEx<MockUIElementMgr> manager;
  MockEngine engine;
  CandidatePage* page = engine.GetCandidatePage();
  ui.Initialize(&manager, NULL, &engine);

  UINT count = 0;
  EXPECT_TRUE(SUCCEEDED(ui.GetCount(&count)));
  EXPECT_EQ(engine.GetCandidateCount(), count);

  UINT selection = 0;
  page->SetCurrentIndex(1);
  EXPECT_TRUE(SUCCEEDED(ui.GetSelection(&selection)));
  EXPECT_EQ(1, selection);

  CComBSTR candidate1, candidate2;
  EXPECT_TRUE(SUCCEEDED(ui.GetString(0, &candidate1)));
  EXPECT_STREQ(MockEngine::kTestCandidate1, candidate1);
  EXPECT_TRUE(SUCCEEDED(ui.GetString(1, &candidate2)));
  EXPECT_STREQ(MockEngine::kTestCandidate2, candidate2);
  EXPECT_EQ(E_INVALIDARG, ui.GetString(1, NULL));

  UINT page_count = 0;
  EXPECT_TRUE(SUCCEEDED(ui.GetPageIndex(NULL, 0, &page_count)));
  EXPECT_EQ(1, page_count);

  UINT pages[2] = {0, 0};
  EXPECT_TRUE(SUCCEEDED(ui.GetPageIndex(pages, 2, &page_count)));
  EXPECT_EQ(1, page_count);
  EXPECT_EQ(0, pages[0]);
  EXPECT_EQ(0, pages[1]);

  UINT new_pages[2] = {0, 1};
  EXPECT_TRUE(SUCCEEDED(ui.SetPageIndex(new_pages, 2)));
  EXPECT_EQ(2, page->GetPageCount());
  EXPECT_EQ(0, page->indices()[0]);
  EXPECT_EQ(1, page->indices()[1]);
  EXPECT_EQ(2, page->indices()[2]);

  UINT current_page = 0;
  EXPECT_TRUE(SUCCEEDED(ui.GetCurrentPage(&current_page)));
  EXPECT_EQ(1, current_page);

  ui.Uninitialize();
}

TEST(ExternalCandidateUI, ITfCandidateListUIElementBehavior) {
  CComObjectStackEx<ExternalCandidateUI> ui;
  CComObjectStackEx<MockUIElementMgr> manager;
  MockEngine engine;
  CandidatePage* page = engine.GetCandidatePage();
  ui.Initialize(&manager, NULL, &engine);

  page->Reset(2);
  EXPECT_TRUE(SUCCEEDED(ui.SetSelection(1)));
  EXPECT_EQ(1, page->GetCurrentIndex());

  EXPECT_TRUE(SUCCEEDED(ui.Finalize()));
  EXPECT_EQ(1L, engine.commands().size());
  EXPECT_TRUE(engine.commands()[0]->is<CommitCommand>());

  engine.Reset();
  EXPECT_TRUE(SUCCEEDED(ui.Abort()));
  EXPECT_EQ(1L, engine.commands().size());
  EXPECT_TRUE(engine.commands()[0]->is<CancelCommand>());

  ui.Uninitialize();
}
}  // namespace tsf
}  // namespace ime_goopy
