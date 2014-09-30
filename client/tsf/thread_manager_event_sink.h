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

#ifndef GOOPY_TSF_THREAD_MANAGER_EVENT_SINK_H_
#define GOOPY_TSF_THREAD_MANAGER_EVENT_SINK_H_

#include <atlbase.h>
#include <atlcom.h>
#include <msctf.h>
#include <string>
#include "base/basictypes.h"
#include "base/scoped_ptr.h"
#include "common/smart_com_ptr.h"
#include "tsf/sink_advisor.h"

namespace ime_goopy {
class UIManagerInterface;
namespace tsf {
class ContextEventSink;
class ContextManager;
class Compartment;

// A thread manager represents an activated text service. An instance of the
// thread manager will be created when the text service is activated and will
// be destroyed when the user switches to another text service. Thread manager
// creates and mantains contexts, and handle some global sinks.
class ATL_NO_VTABLE ThreadManagerEventSink
    : public CComObjectRootEx<CComSingleThreadModel>,
      public ITfThreadMgrEventSink,
      public ITfThreadFocusSink,
      public ITfCompartmentEventSink,
      public ITfActiveLanguageProfileNotifySink {
 public:
  DECLARE_NOT_AGGREGATABLE(ThreadManagerEventSink)
  BEGIN_COM_MAP(ThreadManagerEventSink)
    COM_INTERFACE_ENTRY(ITfThreadMgrEventSink)
    COM_INTERFACE_ENTRY(ITfThreadFocusSink)
    COM_INTERFACE_ENTRY(ITfCompartmentEventSink)
    COM_INTERFACE_ENTRY(ITfActiveLanguageProfileNotifySink)
  END_COM_MAP()
  DECLARE_PROTECT_FINAL_CONSTRUCT()

  ThreadManagerEventSink();
  ~ThreadManagerEventSink();

  HRESULT Initialize(ITfThreadMgr* thread_manager,
                     TfClientId client_id);
  HRESULT Uninitialize();
  void SetEngineStatus();
  void FetchEngineStatus();
  void UpdateStatus(bool native, bool full_shape, bool full_punct);

  // Getters.
  UIManagerInterface *ui_manager() { return ui_manager_.get(); }
  ITfThreadMgr *thread_manager() { return thread_manager_; }
  TfClientId client_id() { return client_id_; }

  // ITfThreadMgrEventSink
  STDMETHODIMP OnInitDocumentMgr(ITfDocumentMgr* document_manager);
  STDMETHODIMP OnUninitDocumentMgr(ITfDocumentMgr* document_manager);
  STDMETHODIMP OnSetFocus(ITfDocumentMgr* document_manager,
                          ITfDocumentMgr* previous_document_manager);
  STDMETHODIMP OnPushContext(ITfContext* context);
  STDMETHODIMP OnPopContext(ITfContext* context);

  // ITfThreadFocusSink
  STDMETHODIMP OnSetThreadFocus();
  STDMETHODIMP OnKillThreadFocus();

  // ITfCompartmentEventSink
  STDMETHODIMP OnChange(REFGUID rguid);

  // ITfActiveLanguageProfileNotifySink
  STDMETHODIMP OnActivated(REFCLSID clsid, REFGUID guidProfile,
                           BOOL fActivated);

  void OnCleanUpContext(ContextEventSink* context);

 private:
  void SyncCompartmentStatus();
  HRESULT SwitchToActiveContext();
  HRESULT SwitchToActiveContextForDocumentManager(
      ITfDocumentMgr *document_manager);
  HRESULT SwitchContext(ITfContext *context);
  bool GetImeOpenStatus();
  bool SwitchToEnglish();
  bool SwitchToPreviousLanguage();
  bool status_initialized_;
  bool clean_up_;
  bool initialized_;
  DWORD previous_language_;
  std::wstring previous_profile_;
  SmartComPtr<ITfThreadMgr> thread_manager_;
  TfClientId client_id_;

  scoped_ptr<ContextManager> context_manager_;
  scoped_ptr<UIManagerInterface> ui_manager_;

  SmartComPtr<ITfContext> active_context_;
  SmartComObjPtr<ContextEventSink> active_context_event_sink_;

  // Compartments.
  scoped_ptr<Compartment> conversion_status_;
  scoped_ptr<Compartment> keyboard_opened_;

  // Event Sink.
  SinkAdvisor<ITfThreadMgrEventSink> thread_manager_event_sink_advisor_;
  SinkAdvisor<ITfThreadFocusSink> thread_focus_sink_advisor_;
  SinkAdvisor<ITfActiveLanguageProfileNotifySink>
      active_language_profile_notify_sink_advisor_;

  DISALLOW_EVIL_CONSTRUCTORS(ThreadManagerEventSink);
};
}  // namespace tsf
}  // namespace ime_goopy
#endif  // GOOPY_TSF_THREAD_MANAGER_EVENT_SINK_H_
