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

#include "tsf/text_service.h"

#include "base/logging.h"
#include "common/framework_interface.h"
#include "common/singleton.h"
#include "tsf/debug.h"
#include "tsf/display_attribute_manager.h"
#include "tsf/thread_manager_event_sink.h"

namespace ime_goopy {
namespace tsf {
TextService::TextService() {
}

TextService::~TextService() {
}

// ITfTextInputProcessor
// Called when the text service is activated.
// The TSF manager ignores the return value from this method.
HRESULT TextService::Activate(ITfThreadMgr* thread_manager,
                              TfClientId client_id) {
  DVLOG(1) << __FUNCTION__
           << L" thread_manager: " << thread_manager
           << L" client_id: " << client_id;
  HRESULT hr = E_FAIL;
  hr = thread_manager_event_sink_.CreateInstance();
  if (FAILED(hr)) {
    Deactivate();
    return hr;
  }

  hr = thread_manager_event_sink_->Initialize(thread_manager, client_id);
  if (FAILED(hr)) {
    Deactivate();
    return hr;
  }

  return S_OK;
}

// Called when the text service is deactivated.
// The TSF manager ignores the return value from this method.
HRESULT TextService::Deactivate() {
  DVLOG(1) << __FUNCTION__;

  if (thread_manager_event_sink_) {
    thread_manager_event_sink_->Uninitialize();
    thread_manager_event_sink_.Release();
  }
  Singleton<DisplayAttributeManager>::ClearInstance();
  return S_OK;
}

// ITfTextInputProcessorEx
HRESULT TextService::ActivateEx(ITfThreadMgr* thread_manager,
                                TfClientId client_id,
                                DWORD flags) {
  DVLOG(1) << __FUNCTION__
           << L" thread_manager: " << thread_manager
           << L" client_id: " << client_id
           << L" flag: " << Debug::TMAE_String(flags);
  return Activate(thread_manager, client_id);
}

// ITfDisplayAttributeProvider
HRESULT TextService::EnumDisplayAttributeInfo(
    IEnumTfDisplayAttributeInfo **enumerator) {
  DVLOG(1) << __FUNCTION__;
  DisplayAttributeManager *display_attribute =
      Singleton<DisplayAttributeManager>::GetInstance();
  if (!display_attribute) {
    return E_FAIL;
  }
  return display_attribute->GetEnumerator(enumerator);
}

HRESULT TextService::GetDisplayAttributeInfo(REFGUID guid,
                                             ITfDisplayAttributeInfo** info) {
  DVLOG(3) << __FUNCTION__;
  DisplayAttributeManager *display_attribute =
      Singleton<DisplayAttributeManager>::GetInstance();
  if (!display_attribute) {
    return E_FAIL;
  }
  return display_attribute->GetAttribute(guid, info);
}

// ITfFunction
HRESULT TextService::GetDisplayName(BSTR* name) {
  DVLOG(1) << __FUNCTION__;
  if (!name) return E_INVALIDARG;
  *name = SysAllocString(InputMethod::kDisplayName);
  return *name ? S_OK : E_FAIL;
}

// ITfFnConfigure
HRESULT TextService::Show(HWND parent, LANGID langid, REFGUID profile) {
  DVLOG(1) << __FUNCTION__;
  InputMethod::ShowConfigureWindow(parent);
  return S_OK;
}

}  // namespace tsf
}  // namespace ime_goopy
