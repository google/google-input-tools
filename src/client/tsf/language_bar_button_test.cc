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

#include "base/scoped_ptr.h"
#include "tsf/language_bar_button.h"
#include <gtest/gunit.h>

namespace ime_goopy {
namespace tsf {
// {586AA362-E773-4361-A3B4-2EACB7DDC670}
static const CLSID kTestClsid =
{0x586aa362, 0xe773, 0x4361, {0xa3, 0xb4, 0x2e, 0xac, 0xb7, 0xdd, 0xc6, 0x70}};
// {772F1B78-EBB4-4d8d-BA90-CEE02848CF57}
static const GUID kTestGuid =
{0x772f1b78, 0xebb4, 0x4d8d, {0xba, 0x90, 0xce, 0xe0, 0x28, 0x48, 0xcf, 0x57}};
static const DWORD kTestStyle = TF_LBI_STYLE_BTN_MENU;
static const ULONG kTestSort = 1;
static const wchar_t kTestName[] = L"TestName";

TEST(LanguageBarButton, GetInfo) {
  CComObjectStackEx<LanguageBarButton> button;
  button.Initialize(kTestClsid, NULL, kTestGuid, kTestStyle, kTestSort);
  button.SetName(kTestName);
  EXPECT_EQ(E_INVALIDARG, button.GetInfo(NULL));

  TF_LANGBARITEMINFO info;
  EXPECT_TRUE(SUCCEEDED(button.GetInfo(&info)));

  EXPECT_TRUE(kTestClsid == info.clsidService);
  EXPECT_TRUE(kTestGuid == info.guidItem);
  EXPECT_TRUE(kTestStyle == info.dwStyle);
  EXPECT_TRUE(kTestSort == info.ulSort);
  EXPECT_STREQ(kTestName, info.szDescription);

  button.Uninitialize();
}

TEST(LanguageBarButton, GetStatus) {
  CComObjectStackEx<LanguageBarButton> button;

  DWORD status = 0;
  EXPECT_EQ(E_INVALIDARG, button.GetStatus(NULL));

  EXPECT_TRUE(SUCCEEDED(button.GetStatus(&status)));
  EXPECT_EQ(0, status);

  button.SetVisible(false);
  EXPECT_TRUE(SUCCEEDED(button.GetStatus(&status)));
  EXPECT_EQ(TF_LBI_STATUS_HIDDEN, status);

  button.SetEnabled(false);
  EXPECT_TRUE(SUCCEEDED(button.GetStatus(&status)));
  EXPECT_EQ(TF_LBI_STATUS_HIDDEN | TF_LBI_STATUS_DISABLED, status);

  button.SetPressed(true);
  EXPECT_TRUE(SUCCEEDED(button.GetStatus(&status)));
  EXPECT_EQ(TF_LBI_STATUS_HIDDEN |
            TF_LBI_STATUS_DISABLED |
            TF_LBI_STATUS_BTN_TOGGLED, status);
}

TEST(LanguageBarButton, Name) {
  CComObjectStackEx<LanguageBarButton> button;
  button.SetName(kTestName);

  EXPECT_EQ(E_INVALIDARG, button.GetTooltipString(NULL));

  CComBSTR tooltip;
  EXPECT_TRUE(SUCCEEDED(button.GetTooltipString(&tooltip)));
  EXPECT_STREQ(kTestName, tooltip);

  EXPECT_EQ(E_INVALIDARG, button.GetText(NULL));

  CComBSTR text;
  EXPECT_TRUE(SUCCEEDED(button.GetText(&text)));
  EXPECT_STREQ(kTestName, text);
}

static bool _clicked = false;
static void TestClicked() {
  _clicked = true;
}

TEST(LanguageBarButton, OnClick) {
  CComObjectStackEx<LanguageBarButton> button;
  button.SetClickCallback(NewPermanentCallback(&TestClicked));
  POINT pt = {0, 0};
  RECT rect = {0, 0, 0, 0};
  button.OnClick(TF_LBI_CLK_LEFT, pt, &rect);
  EXPECT_TRUE(_clicked);
}

static const DWORD kTestMenuId = 1;
static DWORD _menu_clicked = 0;
static void TestMenuClicked(DWORD id) {
  _menu_clicked = id;
}

TEST(LanguageBarButton, OnMenuSelect) {
  CComObjectStackEx<LanguageBarButton> button;
  button.SetMenuCallback(NewPermanentCallback(&TestMenuClicked));
  button.OnMenuSelect(kTestMenuId);
  EXPECT_EQ(kTestMenuId, _menu_clicked);
}

class ATL_NO_VTABLE MockTSFMenu
    : public CComObjectRootEx<CComSingleThreadModel>,
      public ITfMenu {
 public:
  DECLARE_NOT_AGGREGATABLE(MockTSFMenu)
  BEGIN_COM_MAP(MockTSFMenu)
    COM_INTERFACE_ENTRY(ITfMenu)
  END_COM_MAP()
  DECLARE_PROTECT_FINAL_CONSTRUCT()

  HRESULT STDMETHODCALLTYPE AddMenuItem(
      UINT id,
      DWORD flags,
      HBITMAP bitmap,
      HBITMAP bitmap_mask,
      const WCHAR *pch,
      ULONG cch,
      ITfMenu **menu) {
    ids_.push_back(id);
    return S_OK;
  }

  const vector<int>& ids() { return ids_; }

 private:
  vector<int> ids_;
};

static const int kTestItemId = 1;

TEST(LanguageBarButton, InitMenu) {
  CComObjectStackEx<LanguageBarButton> button;
  CComObjectStackEx<MockTSFMenu> tsf_menu;

  MENUITEMINFO iteminfo;
  ZeroMemory(&iteminfo, sizeof(iteminfo));
  iteminfo.cbSize = sizeof(iteminfo);
  iteminfo.fMask = MIIM_ID;
  iteminfo.wID = kTestItemId;

  HMENU hmenu = CreateMenu();
  EXPECT_TRUE(hmenu);
  EXPECT_TRUE(InsertMenuItem(hmenu, 0, TRUE, &iteminfo));

  button.SetMenu(hmenu);
  button.InitMenu(&tsf_menu);
  EXPECT_EQ(1, tsf_menu.ids().size());
  EXPECT_EQ(kTestItemId, tsf_menu.ids()[0]);

  DestroyMenu(hmenu);
}

class ATL_NO_VTABLE MockLangBarItemSink
    : public CComObjectRootEx<CComSingleThreadModel>,
      public ITfLangBarItemSink {
 public:
  DECLARE_NOT_AGGREGATABLE(MockLangBarItemSink)
  BEGIN_COM_MAP(MockLangBarItemSink)
    COM_INTERFACE_ENTRY(ITfLangBarItemSink)
  END_COM_MAP()
  DECLARE_PROTECT_FINAL_CONSTRUCT()

  HRESULT STDMETHODCALLTYPE OnUpdate(DWORD flags) {
    flags_.push_back(flags);
    return S_OK;
  }

  const vector<DWORD>& flags() { return flags_; }

 private:
  vector<DWORD> flags_;
};

TEST(LanguageBarButton, Sink) {
  CComObjectStackEx<LanguageBarButton> button;
  CComObjectStackEx<MockLangBarItemSink> sink;
  DWORD cookie = 0;
  EXPECT_TRUE(SUCCEEDED(button.AdviseSink(IID_ITfLangBarItemSink,
                                          &sink,
                                          &cookie)));
  button.SetName(kTestName);
  EXPECT_EQ(1, sink.flags().size());
  EXPECT_EQ(TF_LBI_TEXT, sink.flags()[0]);

  EXPECT_EQ(CONNECT_E_NOCONNECTION, button.UnadviseSink(0));
  EXPECT_TRUE(SUCCEEDED(button.UnadviseSink(cookie)));
}

}  // namespace tsf
}  // namespace ime_goopy
