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

#include "tsf/language_bar_button.h"

namespace ime_goopy {
namespace tsf {
// Just a random number to make sure it is our cookie.
static const DWORD kSinkCookie = 0x34a2cf36;

// LanguageBarButton
LanguageBarButton::LanguageBarButton()
    : clsid_(NULL),
      guid_(NULL),
      style_(0),
      sort_(0),
      icon_id_(0),
      menu_(NULL),
      visible_(true),
      enabled_(true),
      pressed_(false),
      msctf_(LoadLibrary(L"msctf.dll")) {
}

LanguageBarButton::~LanguageBarButton() {
  if (msctf_) FreeLibrary(msctf_);
}

HRESULT LanguageBarButton::Initialize(const CLSID& clsid,
                                      HINSTANCE instance,
                                      const GUID& guid,
                                      DWORD style,
                                      ULONG sort) {
  clsid_ = &clsid;
  instance_ = instance;
  guid_ = &guid;
  style_ = style;
  sort_ = sort;

  SmartComPtr<ITfLangBarItemMgr> manager;
  manager.Attach(CreateLangBarItemMgr());
  if (manager.p() == NULL) return E_FAIL;

  HRESULT hr = manager->AddItem(this);
  if (FAILED(hr)) return hr;

  return S_OK;
}

HRESULT LanguageBarButton::Uninitialize() {
  SmartComPtr<ITfLangBarItemMgr> manager;
  manager.Attach(CreateLangBarItemMgr());
  if (manager.p() == NULL) return E_FAIL;

  HRESULT hr = manager->RemoveItem(this);
  if (FAILED(hr)) return hr;

  return S_OK;
}

void LanguageBarButton::SetName(const wstring& text) {
  if (name_ == text) return;
  name_ = text;
  if (sink_) sink_->OnUpdate(TF_LBI_TEXT);
}

void LanguageBarButton::SetVisible(bool value) {
  if (visible_ == value) return;
  visible_ = value;
  if (sink_) sink_->OnUpdate(TF_LBI_STATUS);
}

void LanguageBarButton::SetEnabled(bool value) {
  if (enabled_ == value) return;
  enabled_ = value;
  if (sink_) sink_->OnUpdate(TF_LBI_STATUS);
}

void LanguageBarButton::SetPressed(bool value) {
  if (pressed_ == value) return;
  pressed_ = value;
  if (sink_) sink_->OnUpdate(TF_LBI_STATUS);
}

void LanguageBarButton::SetIconId(LPCWSTR icon_id) {
  if (icon_id_ == icon_id) return;
  icon_id_ = icon_id;
  if (sink_) sink_->OnUpdate(TF_LBI_ICON);
}

void LanguageBarButton::SetMenu(HMENU menu) {
  menu_ = menu;
}

// ITfLangBarItem
HRESULT LanguageBarButton::GetInfo(TF_LANGBARITEMINFO *info) {
  if (!info) return E_INVALIDARG;
  assert(clsid_);
  assert(guid_);

  info->clsidService = *clsid_;
  info->guidItem = *guid_;
  info->dwStyle = style_;
  info->ulSort = sort_;
  wcsncpy(info->szDescription,
          name_.c_str(),
          arraysize(info->szDescription));
  return S_OK;
}

// ITfLangBarItem
HRESULT LanguageBarButton::GetStatus(DWORD *status) {
  if (!status) return E_INVALIDARG;
  DWORD new_status = 0;
  if (!visible_) new_status |= TF_LBI_STATUS_HIDDEN;
  if (!enabled_) new_status |= TF_LBI_STATUS_DISABLED;
  if (pressed_) new_status |= TF_LBI_STATUS_BTN_TOGGLED;
  *status = new_status;
  return S_OK;
}

// ITfLangBarItem
HRESULT LanguageBarButton::Show(BOOL show) {
  SetVisible(show);
  return S_OK;
}

// ITfLangBarItem
HRESULT LanguageBarButton::GetTooltipString(BSTR* tooltip) {
  if (!tooltip) return E_INVALIDARG;
  // The caller will free this buffer.
  *tooltip = SysAllocString(name_.c_str());
  return (*tooltip == NULL) ? E_OUTOFMEMORY : S_OK;
}

// ITfLangBarItemButton
HRESULT LanguageBarButton::OnClick(TfLBIClick click,
                                  POINT pt,
                                  const RECT* area) {
  if (!area) return E_INVALIDARG;
  if (click != TF_LBI_CLK_LEFT) return S_OK;
  if (click_callback_.get()) click_callback_->Run();
  return S_OK;
}

// ITfLangBarItemButton
HRESULT LanguageBarButton::InitMenu(ITfMenu* menu) {
  if (!menu) return E_INVALIDARG;
  if (click_callback_.get()) click_callback_->Run();
  if (menu_) BuildTsfMenu(menu_, menu);
  return S_OK;
}

// ITfLangBarItemButton
HRESULT LanguageBarButton::OnMenuSelect(UINT id) {
  if (menu_callback_.get()) menu_callback_->Run(id);
  return S_OK;
}

// ITfLangBarItemButton
HRESULT LanguageBarButton::GetIcon(HICON* icon) {
  if (!icon) return E_INVALIDARG;
  if (icon_id_) {
    // The icon will be destroyed by the caller.
    *icon = static_cast<HICON>(LoadImage(
        instance_, icon_id_, IMAGE_ICON, 0, 0, 0));
  } else {
    *icon = NULL;
  }
  return S_OK;
}

// ITfLangBarItemButton
HRESULT LanguageBarButton::GetText(BSTR *text) {
  if (!text) return E_INVALIDARG;
  // Caller will free the buffer.
  *text = SysAllocString(name_.c_str());

  return (*text == NULL) ? E_OUTOFMEMORY : S_OK;
}

// ITfSource
HRESULT LanguageBarButton::AdviseSink(REFIID riid,
                                      IUnknown *punk,
                                      DWORD *cookie) {
  if (!IsEqualIID(IID_ITfLangBarItemSink, riid))
    return CONNECT_E_CANNOTCONNECT;

  // We support only one sink.
  if (sink_ != NULL) return CONNECT_E_ADVISELIMIT;

  HRESULT hr = punk->QueryInterface(IID_ITfLangBarItemSink,
                                    reinterpret_cast<void**>(&sink_));
  if (FAILED(hr)) {
    sink_ = NULL;
    return E_NOINTERFACE;
  }

  *cookie = kSinkCookie;
  return S_OK;
}

// ITfSource
HRESULT LanguageBarButton::UnadviseSink(DWORD cookie) {
  if (cookie != kSinkCookie) return CONNECT_E_NOCONNECTION;
  if (sink_ == NULL) return CONNECT_E_NOCONNECTION;

  sink_ = NULL;
  return S_OK;
}

HRESULT LanguageBarButton::BuildTsfMenu(HMENU hmenu, ITfMenu* menu) {
  if (!hmenu || !menu) return E_INVALIDARG;

  int count = GetMenuItemCount(hmenu);
  if (count == 0) return S_OK;

  wchar_t name[MAX_PATH] = {0};
  MENUITEMINFO info;
  info.cbSize = sizeof(info);
  info.fMask = MIIM_ID | MIIM_STATE | MIIM_STRING | MIIM_SUBMENU | MIIM_FTYPE;
  info.dwTypeData = name;

  for (int i = 0; i < count; i++) {
    info.cch = MAX_PATH;
    if (!GetMenuItemInfo(hmenu, i, TRUE, &info)) continue;

    DWORD flags = 0;
    if (info.fType & MFT_SEPARATOR) flags = TF_LBMENUF_SEPARATOR;
    if (info.hSubMenu) flags = TF_LBMENUF_SUBMENU;
    if (info.fState & MFS_CHECKED) flags |= TF_LBMENUF_CHECKED;
    SmartComPtr<ITfMenu> submenu;
    menu->AddMenuItem(info.wID,
                      flags,
                      NULL,
                      NULL,
                      name,
                      static_cast<ULONG>(wcslen(name)),
                      submenu.GetAddress());

    if (submenu) BuildTsfMenu(info.hSubMenu, submenu);
  }
  return S_OK;
}

// When there is no COM support in the host application, we must use the dll's
// export function to create the manager. This piece of code also works when
// COM is available. The dll should not be freed here, because it's needed by
// the created object.
ITfLangBarItemMgr* LanguageBarButton::CreateLangBarItemMgr() {
  typedef HRESULT (WINAPI *CreateLangBarItemMgrProc)(ITfLangBarItemMgr**);
  if (msctf_ == NULL) return NULL;

  ITfLangBarItemMgr* manager = NULL;

  CreateLangBarItemMgrProc proc = reinterpret_cast<CreateLangBarItemMgrProc>(
      GetProcAddress(msctf_, "TF_CreateLangBarItemMgr"));
  if (!proc) return NULL;
  if (FAILED((*proc)(&manager))) return NULL;
  return manager;
}

}  // namespace tsf
}  // namespace ime_goopy
