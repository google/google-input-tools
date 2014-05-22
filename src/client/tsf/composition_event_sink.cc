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

#include "tsf/composition_event_sink.h"

#include "base/callback.h"
#include "base/logging.h"
#include "common/debug.h"
#include "common/framework_interface.h"
#include "common/singleton.h"
#include "common/smart_com_ptr.h"
#include "tsf/context_event_sink.h"
#include "tsf/edit_session.h"
#include "tsf/display_attribute_manager.h"

namespace ime_goopy {
namespace tsf {
CompositionEventSink::CompositionEventSink()
    : context_event_sink_(NULL),
      client_id_(TF_CLIENTID_NULL),
      engine_(NULL),
      mouse_sink_cookie_(TF_INVALID_COOKIE) {
}

CompositionEventSink::~CompositionEventSink() {
  if (Ready()) Uninitialize();
}

HRESULT CompositionEventSink::Initialize(ContextEventSink* context_event_sink) {
  DVLOG(1) << __FUNCTION__;
  DCHECK(context_event_sink);

  context_event_sink_ = context_event_sink;
  context_ = context_event_sink->context();
  client_id_ = context_event_sink->client_id();
  engine_ = context_event_sink->GetEngine();
  return S_OK;
}

HRESULT CompositionEventSink::Uninitialize() {
  DVLOG(1) << __FUNCTION__;

  DCHECK(!Composing());
  AdviseMouseSink(NULL);
  engine_ = NULL;
  client_id_ = TF_CLIENTID_NULL;
  context_.Release();
  context_event_sink_ = NULL;
  return S_OK;
}

HRESULT CompositionEventSink::OnCompositionTerminated(
    TfEditCookie cookie, ITfComposition *composition) {
  DVLOG(1) << __FUNCTION__;
  // This step requires a write cookie.
  DCHECK(context_event_sink_->write_cookie() == TF_INVALID_COOKIE);
  context_event_sink_->set_write_cookie(cookie);
  engine_->EndComposition(true);
  context_event_sink_->set_write_cookie(TF_INVALID_COOKIE);
  return S_OK;
}

HRESULT CompositionEventSink::UpdateComposition(TfEditCookie cookie,
                                                const std::wstring& composition,
                                                int caret) {
  if (!Composing())
    return S_OK;
  composition_range_->SetText(
      cookie,
      0,
      composition.c_str(),
      static_cast<int>(composition.size()));
  SetCaretPosition(cookie, caret);
  ApplyInputAttribute(cookie);
  AdviseMouseSink(composition_range_);
  return S_OK;
}

HRESULT CompositionEventSink::StartComposition(TfEditCookie cookie) {
  DVLOG(3) << __FUNCTION__;
  // Get selection range.
  SmartComPtr<ITfInsertAtSelection> insert(context_);
  if (!insert) return E_NOINTERFACE;

  SmartComPtr<ITfRange> insert_range;
  HRESULT hr = insert->InsertTextAtSelection(
      cookie, TF_IAS_QUERYONLY, NULL, 0, insert_range.GetAddress());
  if (FAILED(hr)) return hr;

  return StartCompositionAt(cookie, insert_range);
}

HRESULT CompositionEventSink::StartCompositionAt(TfEditCookie cookie,
                                                 ITfRange *range) {
  DVLOG(3) << __FUNCTION__;
  // Start composition.
  SmartComPtr<ITfContextComposition> context_composition(context_);
  if (!context_composition) return E_NOINTERFACE;
  HRESULT hr = context_composition->StartComposition(
      cookie, range, this, composition_.GetAddress());
  if (FAILED(hr)) return hr;

  // Save composition range.
  hr = composition_->GetRange(composition_range_.GetAddress());
  if (FAILED(hr)) return hr;

  hr = ApplyInputAttribute(cookie);
  if (FAILED(hr)) return hr;

  return S_OK;
}

HRESULT CompositionEventSink::EndComposition(TfEditCookie cookie,
                                             const std::wstring& result) {
  DVLOG(3) << __FUNCTION__;
  if (!Composing())
    return S_OK;
  composition_range_->SetText(
      cookie,
      0,
      result.c_str(),
      static_cast<int>(result.size()));
  SetCaretPosition(cookie, result.size());

  ClearInputAttribute(cookie);
  AdviseMouseSink(NULL);

  composition_->EndComposition(cookie);

  composition_ = NULL;
  composition_range_ = NULL;

  return S_OK;
}

HRESULT CompositionEventSink::ApplyInputAttribute(TfEditCookie cookie) {
  DisplayAttributeManager *display_attribute_manager =
      Singleton<DisplayAttributeManager>::GetInstance();
  if (!display_attribute_manager) {
    DVLOG(1) << "Getting DisplayAttributeManager instance failed in "
             << __SHORT_FUNCTION__;
    return E_FAIL;
  }
  return display_attribute_manager->ApplyInputAttribute(
      context_, composition_range_, cookie,
      engine_->GetTextStyleIndex(TEXTSTATE_COMPOSING));
}

HRESULT CompositionEventSink::ClearInputAttribute(TfEditCookie cookie) {
  DisplayAttributeManager *display_attribute_manager =
      Singleton<DisplayAttributeManager>::GetInstance();
  if (!display_attribute_manager) {
    DVLOG(1) << "Getting DisplayAttributeManager instance failed in "
             << __SHORT_FUNCTION__;
    return E_FAIL;
  }
  return display_attribute_manager->ClearAttribute(
      context_, composition_range_, cookie);
}

HRESULT CompositionEventSink::SetCaretPosition(TfEditCookie cookie,
                                               int position) {
  DVLOG(3) << __FUNCTION__;
  // Clone composition range.
  SmartComPtr<ITfRange> range;
  HRESULT hr = composition_range_->Clone(range.GetAddress());
  if (FAILED(hr)) return hr;

  // Collapse composition range to caret position.
  LONG shifted = 0;
  range->ShiftStart(cookie, position, &shifted, NULL);
  range->Collapse(cookie, TF_ANCHOR_START);

  // Set caret position.
  TF_SELECTION selection;
  selection.range = range;
  selection.style.ase = TF_AE_NONE;
  selection.style.fInterimChar = FALSE;
  context_->SetSelection(cookie, 1, &selection);

  return S_OK;
}

HRESULT CompositionEventSink::OnMouseEvent(ULONG edge,
                                           ULONG quadrant,
                                           DWORD button_status,
                                           BOOL* eaten) {
  DVLOG(3) << __FUNCTION__;
  *eaten = TRUE;
  HRESULT hr = RequestEditSession(
      context_, client_id_, this->GetUnknown(),
      NewPermanentCallback(
          this, &CompositionEventSink::ProcessMouseEventSession),
      button_status, static_cast<int>(edge),
      TF_ES_ASYNCDONTCARE | TF_ES_READWRITE);
  if (FAILED(hr)) {
    DVLOG(1) << "RequestEditSession failed in " << __SHORT_FUNCTION__;
    return hr;
  }
  return S_OK;
}

HRESULT CompositionEventSink::AdviseMouseSink(ITfRange* range) {
  DVLOG(3) << __FUNCTION__;
  SmartComPtr<ITfMouseTracker> tracker(context_);
  if (!tracker) return E_NOINTERFACE;

  // Unadvise existing sink first.
  if (mouse_sink_cookie_ != TF_INVALID_COOKIE) {
    tracker->UnadviseMouseSink(mouse_sink_cookie_);
    mouse_sink_cookie_ = TF_INVALID_COOKIE;
  }

  // Advise new range if requested.
  if (range) {
    HRESULT hr = tracker->AdviseMouseSink(range, this, &mouse_sink_cookie_);
    if (FAILED(hr)) {
      mouse_sink_cookie_ = TF_INVALID_COOKIE;
      return hr;
    }
  }
  return S_OK;
}

HRESULT CompositionEventSink::Update(TfEditCookie cookie,
                                     const std::wstring& composition,
                                     int caret) {
  // Sync the TSF states with the engine, like text and caret position.
  // Since this function is asynchronous, it is possible the engine has
  // exits the composition, while TSF hasn't, or vice versa.
  if (Composing()) {
    if (composition.size()) {
      // Both the engine and TSF are in composition. Good.
      // Simply sync the text and caret position.
      UpdateComposition(cookie, composition, caret);
    } else {
      // The TSF is in composition while the engine not.
      // So let TSF out.
      EndComposition(cookie, L"");
    }
  } else {
    if (composition.size()) {
      // The engine is in composition while TSF not. So let TSF in.
      StartComposition(cookie);
      UpdateComposition(cookie, composition, caret);
    }
  }
  return S_OK;
}

HRESULT CompositionEventSink::UpdateCallback(TfEditCookie cookie,
                                             std::wstring composition,
                                             int caret) {
  return Update(cookie, composition, caret);
}

HRESULT CompositionEventSink::CommitResult(TfEditCookie cookie,
                                           const std::wstring& result) {
  if (result.size() == 0)
    return S_OK;
  if (!Composing()) {
    StartComposition(cookie);
  }
  UpdateComposition(cookie, result, result.size());
  EndComposition(cookie, result);
  return S_OK;
}

HRESULT CompositionEventSink::CommitResultForCallback(TfEditCookie cookie,
                                                      std::wstring result) {
  return CommitResult(cookie, result);
}

HRESULT CompositionEventSink::Reconvert(TfEditCookie cookie, ITfRange *range) {
  DCHECK(!Composing());
  if (Composing())
    return E_UNEXPECTED;

  HRESULT hr = StartCompositionAt(cookie, range);
  if (FAILED(hr))
    return E_FAIL;
  return S_OK;
}

HRESULT CompositionEventSink::ProcessMouseEventSession(
    TfEditCookie cookie, DWORD button_status, int offset) {
  if (!engine_) {
    DCHECK(false);
    return E_UNEXPECTED;
  }
  engine_->ProcessMouseEvent(button_status, offset);
  return S_OK;
}

}  // namespace tsf
}  // namespace ime_goopy
