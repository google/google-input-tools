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

#include "tsf/thread_manager_event_sink.h"

#include "base/logging.h"
#include "common/language_utils.h"
#include "tsf/compartment.h"
#include "tsf/context_event_sink.h"
#include "tsf/context_manager.h"
#include "tsf/key_event_sink.h"
#include "tsf/sink_advisor.h"
#include "tsf/tsf_utils.h"

namespace ime_goopy {
namespace tsf {
ThreadManagerEventSink::ThreadManagerEventSink()
    : client_id_(TF_CLIENTID_NULL),
      status_initialized_(false),
      clean_up_(false),
      initialized_(false),
      previous_language_(0) {
}

ThreadManagerEventSink::~ThreadManagerEventSink() {
}

HRESULT ThreadManagerEventSink::Initialize(ITfThreadMgr* thread_manager,
                                           TfClientId client_id) {
  assert(thread_manager);
  assert(client_id != TF_CLIENTID_NULL);

  thread_manager_ = thread_manager;
  client_id_ = client_id;

  HRESULT hr = E_FAIL;

  // Context manager.
  context_manager_.reset(new ContextManager(this));
  if (!context_manager_.get()) return E_FAIL;

  // UI Manager.
  ui_manager_.reset(InputMethod::CreateUIManager(NULL));
  if (!ui_manager_.get()) return E_FAIL;

  // Sinks.
  hr = thread_manager_event_sink_advisor_.Advise(thread_manager, this);
  if (FAILED(hr)) return hr;

  hr = thread_focus_sink_advisor_.Advise(thread_manager, this);
  if (FAILED(hr)) return hr;
  hr = active_language_profile_notify_sink_advisor_.Advise(thread_manager,
                                                           this);
  if (FAILED(hr)) return hr;
  // Status.
  conversion_status_.reset(new Compartment(
      client_id,
      thread_manager,
      GUID_COMPARTMENT_KEYBOARD_INPUTMODE_CONVERSION,
      this));
  keyboard_opened_.reset(new Compartment(
      client_id,
      thread_manager,
      GUID_COMPARTMENT_KEYBOARD_OPENCLOSE,
      this));

  SwitchToActiveContext();
  initialized_ = true;
  return S_OK;
}

void ThreadManagerEventSink::SetEngineStatus() {
  if (active_context_event_sink_ == NULL)
    return;
  EngineInterface *engine = active_context_event_sink_->GetEngine();
  if (engine == NULL)
    return;
  DWORD conversion = 0, sentense = 0;
  if (conversion_status_.get() &&
      SUCCEEDED(conversion_status_->GetInteger(&conversion))) {
    engine->NotifyConversionModeChange(conversion);
  }
}

void ThreadManagerEventSink::FetchEngineStatus() {
  if (active_context_event_sink_ == NULL)
    return;
  EngineInterface *engine = active_context_event_sink_->GetEngine();
  if (engine == NULL)
    return;
  int conversion_mode = engine->GetConversionMode();
  if (conversion_status_.get())
    conversion_status_->SetInteger(conversion_mode);
}

void ThreadManagerEventSink::UpdateStatus(bool native,
                                          bool full_shape,
                                          bool full_punct) {
  if (conversion_status_.get()) {
    uint32 conversion = 0;
    conversion |= native ? EngineInterface::CONVERSION_MODE_CHINESE : 0;
    conversion |= full_shape ? EngineInterface::CONVERSION_MODE_FULL_SHAPE : 0;
    conversion |= full_punct ? EngineInterface::CONVERSION_MODE_FULL_PUNCT : 0;
    conversion_status_->SetInteger(conversion);
  }
}

HRESULT ThreadManagerEventSink::Uninitialize() {
  // Status.
  conversion_status_.reset(NULL);
  keyboard_opened_.reset(NULL);

  // Sinks.
  thread_focus_sink_advisor_.Unadvise();
  thread_manager_event_sink_advisor_.Unadvise();
  active_language_profile_notify_sink_advisor_.Unadvise();
  ui_manager_->SetContext(NULL);
  context_manager_->RemoveAll();
  context_manager_.reset(NULL);
  thread_manager_.Release();
  ui_manager_.reset(NULL);
  client_id_ = NULL;
  return S_OK;
}

// ITfThreadMgrEventSink
// Called just before the first context is pushed onto a document.
HRESULT ThreadManagerEventSink::OnInitDocumentMgr(
    ITfDocumentMgr* document_manager) {
  DVLOG(1) << __FUNCTION__
           << L" doc: " << document_manager;
  return SwitchToActiveContext();
}

// ITfThreadMgrEventSink
// Called just after the last context is popped off a document.
HRESULT ThreadManagerEventSink::OnUninitDocumentMgr(
    ITfDocumentMgr* document_manager) {
  DVLOG(1) << __FUNCTION__
           << L" doc: " << document_manager;
  return SwitchToActiveContext();
}

// ITfThreadMgrEventSink
// Sink called by the framework when focus changes from one document to
// another. Either document may be NULL, meaning previously there was no
// focus document, or now no document holds the input focus.
HRESULT ThreadManagerEventSink::OnSetFocus(
    ITfDocumentMgr* document_manager,
    ITfDocumentMgr* previous_document_manager) {
  DVLOG(1) << __FUNCTION__
           << L" from_doc: " << previous_document_manager
           << L" to_doc: " << document_manager;
  return SwitchToActiveContextForDocumentManager(document_manager);
}

// ITfThreadFocusSink
HRESULT ThreadManagerEventSink::OnSetThreadFocus() {
  DVLOG(1) << __FUNCTION__;
  return SwitchToActiveContext();
}

HRESULT ThreadManagerEventSink::OnKillThreadFocus() {
  DVLOG(1) << __FUNCTION__;
  return SwitchContext(NULL);
}

// ITfThreadMgrEventSink
// Sink called by the framework when a context is pushed.
HRESULT ThreadManagerEventSink::OnPushContext(ITfContext *context) {
  DVLOG(1) << __FUNCTION__
           << L" context: " << context;
  return SwitchToActiveContext();
}

// ITfThreadMgrEventSink
// Sink called by the framework when a context is popped.
HRESULT ThreadManagerEventSink::OnPopContext(ITfContext *context) {
  DVLOG(1) << __FUNCTION__
           << L" context: " << context;
  return SwitchToActiveContext();
}

// ITfCompartmentEventSink
HRESULT ThreadManagerEventSink::OnChange(REFGUID rguid) {
  DVLOG(1) << __FUNCTION__;
  if (!status_initialized_) {
    return S_OK;
  }
  if (active_context_event_sink_ == NULL) {
    return E_UNEXPECTED;
  }
  EngineInterface *engine = active_context_event_sink_->GetEngine();
  if (engine == NULL) {
    return S_OK;
  }
  DWORD value = 0;
  if (IsEqualGUID(rguid, GUID_COMPARTMENT_KEYBOARD_INPUTMODE_CONVERSION)) {
    if (conversion_status_->GetInteger(&value) == S_OK) {
      engine->NotifyConversionModeChange(value);
    }
  } else if (IsEqualGUID(rguid,
                         GUID_COMPARTMENT_KEYBOARD_OPENCLOSE)) {
    if (keyboard_opened_->GetInteger(&value) == S_OK) {
      ui_manager_->SetContext(value ? active_context_event_sink_.p() : NULL);
      if (active_context_event_sink_)
        active_context_event_sink_->key_event_sink()->set_enabled(value);
    }
  }
  return S_OK;
}

void ThreadManagerEventSink::SyncCompartmentStatus() {
  if (active_context_event_sink_ == NULL)
    return;
  EngineInterface *engine = active_context_event_sink_->GetEngine();
  if (status_initialized_) {
    SetEngineStatus();
  } else {
    FetchEngineStatus();
    keyboard_opened_->SetInteger(TRUE);
    status_initialized_ = true;
  }
}

HRESULT ThreadManagerEventSink::SwitchToActiveContext() {
  SmartComPtr<ITfDocumentMgr> document_manager;
  HRESULT hr = thread_manager_->GetFocus(document_manager.GetAddress());
  if (FAILED(hr)) {
    DVLOG(1) << L"Can't get focus.";
    return SwitchContext(NULL);
  }
  return SwitchToActiveContextForDocumentManager(document_manager);
}

HRESULT ThreadManagerEventSink::SwitchToActiveContextForDocumentManager(
    ITfDocumentMgr *document_manager) {
  if (!document_manager)
    return SwitchContext(NULL);

  SmartComPtr<ITfContext> context;
  HRESULT hr = document_manager->GetTop(context.GetAddress());
  if (FAILED(hr)) {
    DVLOG(1) << L"Can't get top context.";
    return SwitchContext(NULL);
  }
  if (!context) {
    DVLOG(3) << L"No top context.";
    return SwitchContext(NULL);
  }
  return SwitchContext(context);
}

HRESULT ThreadManagerEventSink::SwitchContext(ITfContext *context) {
  if (active_context_ == context && initialized_)
    return S_OK;
  if (context_manager_ == NULL) {
    DCHECK(false);
    return E_UNEXPECTED;
  }
  active_context_ = context;
  if (context) {
    // active_context_event_sink_ would be NULL if the context doesn't
    // accept text input.
    context_manager_->GetOrCreate(
        context, active_context_event_sink_.GetAddress());
    ui_manager_->SetContext(active_context_event_sink_);
    SyncCompartmentStatus();
    ui_manager_->SetToolbarStatus(active_context_event_sink_ != NULL);
  } else {
    active_context_event_sink_ = NULL;
    ui_manager_->SetContext(false);
    ui_manager_->SetToolbarStatus(false);
  }
  if (clean_up_) return S_OK;

  if (!GetImeOpenStatus()) {
    SwitchToEnglish();
  } else {
    SwitchToPreviousLanguage();
  }
  return S_OK;
}

void ThreadManagerEventSink::OnCleanUpContext(ContextEventSink* context) {
  clean_up_ = true;
  SwitchContext(NULL);
}

HRESULT ThreadManagerEventSink::OnActivated(REFCLSID clsid, REFGUID guidProfile,
                                            BOOL fActivated) {
  if (::IsEqualGUID(clsid, InputMethod::kTextServiceClsid) &&
      fActivated &&
      clean_up_) {
    // Switch between different languages in the same text service.
    clean_up_ = false;
    status_initialized_ = false;
    initialized_ = false;
    SwitchToActiveContext();
    initialized_ = true;
    if (active_context_event_sink_)
      active_context_event_sink_->AttachEngine();
  }
  return S_OK;
}

bool ThreadManagerEventSink::GetImeOpenStatus() {
  if (!active_context_event_sink_) return false;
  DWORD value = 0;
  if (keyboard_opened_->GetInteger(&value) != S_OK) return false;
  return value;
}

bool ThreadManagerEventSink::SwitchToEnglish() {
  DWORD lang = TSFUtils::GetCurrentLanguageId();
  if (lang != kEnglishInfo.id) {
    previous_language_ = lang;
    previous_profile_ = TSFUtils::GetCurrentLanguageProfile();
    TSFUtils::SwitchToTIP(kEnglishInfo.id, kEnglishInfo.guid);
    return true;
  }
  return false;
}

bool ThreadManagerEventSink::SwitchToPreviousLanguage() {
  DWORD lang = TSFUtils::GetCurrentLanguageId();
  if (previous_language_ && previous_language_ != lang) {
    TSFUtils::SwitchToTIP(previous_language_, previous_profile_);
    previous_language_ = 0;
    return true;
  }
  return false;
}

}  // namespace tsf
}  // namespace ime_goopy
