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

#ifndef GOOPY_TSF_TEXT_SERVICE_H_
#define GOOPY_TSF_TEXT_SERVICE_H_

#include <atlbase.h>
#include <atlstr.h>
#include <atlcom.h>
#include <msctf.h>
#include <ctffunc.h>

#include "base/basictypes.h"
#include "base/scoped_ptr.h"
#include "common/smart_com_ptr.h"
#include "common/framework_interface.h"

namespace ime_goopy {
namespace tsf {
class ThreadManagerEventSink;
class DisplayAttributeManager;

class ATL_NO_VTABLE TextService
    : public CComObjectRootEx<CComSingleThreadModel>,
      public CComCoClass<TextService, &InputMethod::kTextServiceClsid>,
      public ITfTextInputProcessorEx,
      public ITfDisplayAttributeProvider,
      public ITfFnConfigure {
 public:
  DECLARE_REGISTRY_RESOURCEID(InputMethod::kRegistrarScriptId)
  DECLARE_NOT_AGGREGATABLE(TextService)
  BEGIN_COM_MAP(TextService)
    COM_INTERFACE_ENTRY(ITfTextInputProcessor)
    COM_INTERFACE_ENTRY(ITfTextInputProcessorEx)
    COM_INTERFACE_ENTRY(ITfDisplayAttributeProvider)
    COM_INTERFACE_ENTRY(ITfFunction)
    COM_INTERFACE_ENTRY(ITfFnConfigure)
  END_COM_MAP()
  DECLARE_PROTECT_FINAL_CONSTRUCT()

  TextService();
  ~TextService();

  // ITfTextInputProcessor
  STDMETHODIMP Activate(ITfThreadMgr* thread_manager, TfClientId client_id);
  STDMETHODIMP Deactivate();

  // ITfTextInputProcessorEx
  STDMETHODIMP ActivateEx(ITfThreadMgr* thread_manager,
                          TfClientId client_id,
                          DWORD flags);

  // ITfDisplayAttributeProvider
  STDMETHODIMP EnumDisplayAttributeInfo(
      IEnumTfDisplayAttributeInfo **enumerator);
  STDMETHODIMP GetDisplayAttributeInfo(
      REFGUID guid, ITfDisplayAttributeInfo **info);

  // ITfFunction
  STDMETHODIMP GetDisplayName(BSTR* name);

  // ITfFnConfigure
  STDMETHODIMP Show(HWND parent, LANGID langid, REFGUID profile);

 private:
  SmartComObjPtr<ThreadManagerEventSink> thread_manager_event_sink_;
  DISALLOW_EVIL_CONSTRUCTORS(TextService);
};
OBJECT_ENTRY_AUTO(InputMethod::kTextServiceClsid, TextService)
}  // namespace tsf
}  // namespace ime_goopy
#endif  // GOOPY_TSF_TEXT_SERVICE_H_
