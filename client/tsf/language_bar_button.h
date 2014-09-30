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

#ifndef GOOPY_TSF_LANGUAGE_BAR_BUTTON_H_
#define GOOPY_TSF_LANGUAGE_BAR_BUTTON_H_

#include <atlbase.h>
#include <atlstr.h>
#include <atlcom.h>
#include <msctf.h>

#include "base/basictypes.h"
#include "base/callback.h"
#include "base/scoped_ptr.h"
#include "common/smart_com_ptr.h"

namespace ime_goopy {
namespace tsf {
class ATL_NO_VTABLE LanguageBarButton
    : public CComObjectRootEx<CComSingleThreadModel>,
      public ITfLangBarItemButton,
      public ITfSource {
 public:
  typedef Callback1<DWORD> MenuCallback;
  typedef Closure ClickCallback;

  DECLARE_NOT_AGGREGATABLE(LanguageBarButton)
  BEGIN_COM_MAP(LanguageBarButton)
    COM_INTERFACE_ENTRY(ITfLangBarItemButton)
    COM_INTERFACE_ENTRY(ITfLangBarItem)
    COM_INTERFACE_ENTRY(ITfSource)
  END_COM_MAP()
  DECLARE_PROTECT_FINAL_CONSTRUCT()

  LanguageBarButton();
  ~LanguageBarButton();

  // See memeber variable comments for the meaning of each parameter.
  HRESULT Initialize(const CLSID& clsid,
                     HINSTANCE instance,
                     const GUID& guid,
                     DWORD style,
                     ULONG sort);
  HRESULT Uninitialize();

  // The text, description and tooltip for this button are all the same, which
  // can be modified by SetName().
  void SetName(const wstring& name);
  void SetVisible(bool value);
  void SetEnabled(bool value);
  void SetPressed(bool value);
  void SetIconId(LPCWSTR icon_id);
  void SetMenu(HMENU menu);
  void SetClickCallback(ClickCallback* callback) {
    click_callback_.reset(callback);
  }
  void SetMenuCallback(MenuCallback* callback) {
    menu_callback_.reset(callback);
  }

  // ITfLangBarItem
  // Obtains information about the language bar item.
  STDMETHODIMP GetInfo(TF_LANGBARITEMINFO *info);
  // Obtains the status of a language bar item.
  STDMETHODIMP GetStatus(DWORD *status);
  // Called to show or hide the language bar item.
  STDMETHODIMP Show(BOOL show);
  // Obtains the text to be displayed in the tooltip for the language bar item.
  // tooltip will be deleted by the caller.
  STDMETHODIMP GetTooltipString(BSTR *tooltip);

  // ITfLangBarItemButton
  STDMETHODIMP OnClick(TfLBIClick click, POINT pt, const RECT *area);
  STDMETHODIMP InitMenu(ITfMenu *menu);
  STDMETHODIMP OnMenuSelect(UINT id);
  STDMETHODIMP GetIcon(HICON *icon);
  // tooltip will be deleted by the caller.
  STDMETHODIMP GetText(BSTR *text);

  // ITfSource
  STDMETHODIMP AdviseSink(REFIID riid, IUnknown *punk, DWORD *cookie);
  STDMETHODIMP UnadviseSink(DWORD cookie);

 private:
  // Convert Win32's HMENU to TSF's ITfMenu.
  HRESULT BuildTsfMenu(HMENU hmenu, ITfMenu* menu);
  ITfLangBarItemMgr* CreateLangBarItemMgr();

  const CLSID* clsid_;  // CLSID for the text service.
  const GUID* guid_;    // GUID for this button.
  DWORD style_;   // Combination of TF_LBI_STYLE_* constants.
  ULONG sort_;    // Expected position in language bar.
  LPCWSTR icon_id_;
  HMENU menu_;
  bool visible_;
  bool enabled_;
  bool pressed_;
  wstring name_;  // Button text, description and tooltip.
  // Callback when the button is clicked.
  scoped_ptr<ClickCallback> click_callback_;
  // Callback when any menu item is clicked.
  scoped_ptr<MenuCallback> menu_callback_;
  SmartComPtr<ITfLangBarItemSink> sink_;  // Sink used to notify TSF.
  HINSTANCE instance_;
  HMODULE msctf_;
};
}  // namespace tsf
}  // namespace ime_goopy
#endif  // GOOPY_TSF_LANGUAGE_BAR_BUTTON_H_
