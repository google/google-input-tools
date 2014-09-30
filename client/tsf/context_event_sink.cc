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

#include "tsf/context_event_sink.h"

#include "base/logging.h"
#include "common/debug.h"
#include "common/delayed_execute.h"
#include "text_range/html_text_range.h"
#include "tsf/candidates.h"
#include "tsf/compartment.h"
#include "tsf/composition_event_sink.h"
#include "tsf/debug.h"
#include "tsf/edit_session.h"
#include "tsf/external_candidate_ui.h"
#include "tsf/key_event_sink.h"
#include "tsf/text_range.h"
#include "tsf/thread_manager_event_sink.h"

namespace ime_goopy {
namespace tsf {

ContextEventSink::ContextEventSink()
    : owner_(NULL),
      client_id_(TF_CLIENTID_NULL),
      ui_manager_(NULL),
      write_cookie_(TF_INVALID_COOKIE) {
  memset(&caret_rect_, 0, sizeof(caret_rect_));
  memset(&composition_pos_, 0, sizeof(composition_pos_));
  memset(&candidate_pos_, 0, sizeof(candidate_pos_));
}

ContextEventSink::~ContextEventSink() {
}

HRESULT ContextEventSink::Initialize(ThreadManagerEventSink *owner,
                                     ITfContext *context) {
  DVLOG(1) << __SHORT_FUNCTION__;

  owner_ = owner;
  context_ = context;
  client_id_ = owner->client_id();
  ui_manager_ = owner->ui_manager();
  DCHECK(write_cookie_ == TF_INVALID_COOKIE);
  if (!client_id_ || !ui_manager_) {
    DCHECK(false);
    return E_UNEXPECTED;
  }

  engine_ = InputMethod::CreateEngine(this);
  if (!engine_) {
    DVLOG(1) << "engine is null.";
    return E_FAIL;
  }
  engine_->SetContext(this);
  HRESULT hr = cleanup_context_sink_advisor.Advise(context_, client_id_, this);
  if (FAILED(hr)) {
    DVLOG(2) << L"Can't advise cleanup context sink.";
    return hr;
  }

  // Sinks.
  hr = text_layout_sink_advisor_.Advise(context_, this);
  if (FAILED(hr)) {
    DVLOG(2) << L"Can't advise text layout sink.";
    return hr;
  }

  hr = text_edit_sink_advisor_.Advise(context_, this);
  if (FAILED(hr)) {
    DVLOG(2) << L"Can't advise text edit sink.";
    return hr;
  }

  // CompositionEventSink.
  hr = composition_event_sink_.CreateInstance();
  if (FAILED(hr)) {
    DVLOG(2) << L"Can't create composition.";
    return hr;
  }
  hr = composition_event_sink_->Initialize(this);
  if (FAILED(hr)) {
    DVLOG(2) << L"Can't create composition.";
    return hr;
  }

  // Candidates.
  candidates_.reset(new Candidates(owner_->thread_manager(),
                                   engine_,
                                   ui_manager_));

  // KeyEventSink.
  key_event_sink_.CreateInstance();
  hr = key_event_sink_->Initialize(this);
  if (FAILED(hr)) {
    DVLOG(2) << L"Can't create key_event_sink.";
    return hr;
  }

  // Status.
  empty_context_.reset(new Compartment(
      client_id_,
      context_,
      GUID_COMPARTMENT_EMPTYCONTEXT,
      this));
  keyboard_disabled_.reset(new Compartment(
      client_id_,
      context_,
      GUID_COMPARTMENT_KEYBOARD_DISABLED,
      this));

  return S_OK;
}

HRESULT ContextEventSink::Uninitialize() {
  DVLOG(1) << __FUNCTION__;
  if (key_event_sink_) {
    key_event_sink_->Uninitialize();
    key_event_sink_.Release();
  }
  if (composition_event_sink_) {
    composition_event_sink_->Uninitialize();
    composition_event_sink_.Release();
  }

  InputMethod::DestroyEngineOfContext(this);

  // Status.
  empty_context_.reset(NULL);
  keyboard_disabled_.reset(NULL);

  candidates_.reset(NULL);

  // Sinks.
  cleanup_context_sink_advisor.Unadvise();
  text_layout_sink_advisor_.Unadvise();
  text_edit_sink_advisor_.Unadvise();

  // Clear internal status.
  context_.Release();
  ui_manager_ = NULL;
  client_id_ = TF_CLIENTID_NULL;
  owner_ = NULL;

  return S_OK;
}

KeyEventSink *ContextEventSink::key_event_sink() {
  return key_event_sink_.p();
}

// ITfCleanupContextSink
STDMETHODIMP ContextEventSink::OnCleanupContext(
    TfEditCookie cookie, ITfContext *context) {
  DCHECK(write_cookie_ == TF_INVALID_COOKIE);
  if (engine_) {
    write_cookie_ = cookie;
    UpdateComposition(L"", 0);
    write_cookie_ = TF_INVALID_COOKIE;
  }
  owner_->OnCleanUpContext(this);
  return S_OK;
}

// ITfTextLayoutSink
HRESULT ContextEventSink::OnLayoutChange(ITfContext* context,
                                         TfLayoutCode code,
                                         ITfContextView* context_view) {
  DVLOG(4) << __FUNCTION__
           << L" layout code: " << code;

  HRESULT hr = RequestEditSession(
      context_, client_id_, this->GetUnknown(),
      NewPermanentCallback(this, &ContextEventSink::LayoutChangeSession),
      true,
      TF_ES_ASYNCDONTCARE | TF_ES_READ);
  if (FAILED(hr)) {
    DVLOG(1) << "RequestEditSession failed in " << __SHORT_FUNCTION__;
    return hr;
  }
  return S_OK;
}

// ContextInterface
void ContextEventSink::UpdateComposition(const std::wstring& composition,
                                         int caret) {
  if (write_cookie_ == TF_INVALID_COOKIE) {
    // TODO(zengjian): This is a quick fix to 2976834. Better to find a
    // more standard way.
    RequestEditSession(
        context_, client_id_, this->GetUnknown(),
        NewPermanentCallback(
            composition_event_sink_.p(),
            &CompositionEventSink::UpdateCallback),
        composition,
        caret,
        static_cast<DWORD>(TF_ES_ASYNCDONTCARE | TF_ES_READWRITE));
  } else {
    composition_event_sink_->Update(write_cookie_, composition, caret);
  }
  ui_manager_->Update(COMPONENT_COMPOSITION);
}

void ContextEventSink::CommitResult(const std::wstring& result) {
  if (write_cookie_ == TF_INVALID_COOKIE) {
    // TODO(zengjian): This is a quick fix to 2976834. Better to find a
    // more standard way.
    RequestEditSession(
        context_, client_id_, this->GetUnknown(),
        NewPermanentCallback(composition_event_sink_.p(),
                             &CompositionEventSink::CommitResultForCallback),
        result,
        static_cast<DWORD>(TF_ES_ASYNCDONTCARE | TF_ES_READWRITE));
  } else {
    composition_event_sink_->CommitResult(write_cookie_, result);
  }
}

void ContextEventSink::UpdateCandidates(
    bool is_compositing, const ipc::proto::CandidateList& candidate_list) {
  candidates_->Update(candidate_list);
  ui_manager_->Update(COMPONENT_CANDIDATES);
}

void ContextEventSink::UpdateStatus(bool native,
                                    bool full_shape,
                                    bool full_punct) {
  owner_->UpdateStatus(native, full_shape, full_punct);
  ui_manager_->Update(COMPONENT_STATUS);
}

// ContextInterface
ContextInterface::Platform ContextEventSink::GetPlatform() {
  return PLATFORM_WINDOWS_TSF;
}

EngineInterface* ContextEventSink::GetEngine() {
  return engine_;
}

// ContextInterface
HRESULT ContextEventSink::OnEndEdit(ITfContext *context,
                                    TfEditCookie readonly_cookie,
                                    ITfEditRecord *edit_record) {
  LayoutChangeSession(readonly_cookie, false);
  return S_OK;
}

// ITfCompartmentEventSink
STDAPI ContextEventSink::OnChange(REFGUID rguid) {
  DVLOG(1) << __FUNCTION__;
  DWORD value = 0;
  if (IsEqualGUID(rguid,
                  GUID_COMPARTMENT_EMPTYCONTEXT)) {
    if (empty_context_->GetInteger(&value) == S_OK) {
      ui_manager_->SetContext(value ? this : NULL);
      key_event_sink_->set_enabled(value);
    }
  } else if (IsEqualGUID(rguid,
                         GUID_COMPARTMENT_KEYBOARD_DISABLED)) {
    if (keyboard_disabled_->GetInteger(&value) == S_OK) {
      ui_manager_->SetContext(value ? this : NULL);
      key_event_sink_->set_enabled(value);
    }
  }
  return S_OK;
}

bool ContextEventSink::GetClientRect(RECT* client_rect) {
  return false;
}

bool ContextEventSink::GetCaretRectForCandidate(RECT* rect) {
  *rect = caret_rect_;
  return true;
}

bool ContextEventSink::GetCaretRectForComposition(RECT* rect) {
  *rect = caret_rect_;
  return true;
}

bool ContextEventSink::ShouldShow(UIComponent component) {
  switch (component) {
    case UI_COMPONENT_COMPOSITION:
      return false;
      break;
    case UI_COMPONENT_CANDIDATES:
      return candidates_->ShouldShow();
      break;
    case UI_COMPONENT_STATUS:
      return true;
      break;
  }
  return true;
}

TextRangeInterface *ContextEventSink::GetSelectionRange() {
  if (write_cookie_ == TF_INVALID_COOKIE) {
    DCHECK(false);
    return NULL;
  }
  SmartComPtr<ITfContextView> view;
  // IE doesn't provide text stores, so we have to get the contents
  // by native HTML methods.
  HWND window = NULL;
  if (SUCCEEDED(context_->GetActiveView(view.GetAddress())) &&
      SUCCEEDED(view->GetWnd(&window))) {
    HtmlTextRange *html_text_range =
        HtmlTextRange::CreateFromSelection(NULL, window);
    if (html_text_range)
      return html_text_range;
  }
  TF_SELECTION selection;
  ULONG select_count;
  HRESULT hr = context_->GetSelection(
      write_cookie_, TF_DEFAULT_SELECTION, 1, &selection, &select_count);
  if (FAILED(hr) || select_count != 1) {
    DVLOG(1) << "ITfContext::GetSelection failed in " << __SHORT_FUNCTION__;
    return NULL;
  }
  TextRangeInterface *range = new TextRange(this, selection.range);
  selection.range->Release();
  return range;
}

TextRangeInterface *ContextEventSink::GetCompositionRange() {
  ITfRange *range = composition_event_sink_->range();
  if (!range)
    return NULL;
  return new TextRange(this, range);
}

HRESULT ContextEventSink::LayoutChangeSession(TfEditCookie cookie,
                                              bool by_layout_change) {
  // Return directly when not in composing.
  if (composition_event_sink_ == NULL) return S_OK;
  if (composition_event_sink_->range() == NULL) return S_OK;
  BOOL empty = FALSE;
  composition_event_sink_->range()->IsEmpty(cookie, &empty);
  if (empty) return S_OK;

  HRESULT hr = E_FAIL;
  // Get context view.
  SmartComPtr<ITfContextView> context_view;
  hr = context_->GetActiveView(context_view.GetAddress());
  if (FAILED(hr)) return hr;

  SmartComPtr<ITfRange> start_range, end_range;
  composition_event_sink_->range()->Clone(start_range.GetAddress());
  composition_event_sink_->range()->Clone(end_range.GetAddress());
  start_range->Collapse(cookie, TF_ANCHOR_START);
  end_range->Collapse(cookie, TF_ANCHOR_END);

  // Get caret rect.
  BOOL clipped = TRUE;
  RECT start_rect = {0}, end_rect = {0}, composition_rect = {0};
  hr = context_view->GetTextExt(cookie,
                                composition_event_sink_->range(),
                                &composition_rect,
                                &clipped);
  RECT document_rect = {0};
  context_view->GetScreenExt(&document_rect);
  // Ignore invalid positions.
  // composition rect may be bigger than document rect, so we only check
  // if they intersects.
  RECT intersect_rect = {0};
  ::IntersectRect(&intersect_rect, &composition_rect, &document_rect);
  if (IsRectEmpty(&intersect_rect)) {
    return S_OK;
  }
  if (IsRectEmpty(&composition_rect) && composition_rect.left == 0)
    return S_OK;

  hr = context_view->GetTextExt(cookie,
                                start_range,
                                &start_rect,
                                &clipped);
  if (FAILED(hr)) return hr;

  hr = context_view->GetTextExt(cookie,
                                end_range,
                                &end_rect,
                                &clipped);
  if (FAILED(hr)) return hr;

  // The end rect is not always in composition rect, so we just check if they
  // overlap with each other.
  if (!by_layout_change &&
      (start_rect.left < composition_rect.left ||
      start_rect.right > composition_rect.right ||
      end_rect.right < composition_rect.left ||
      end_rect.left > composition_rect.right)) {
    return S_OK;
  }

  composition_pos_.x = start_rect.left;
  composition_pos_.y = start_rect.top;

  caret_rect_.left = caret_rect_.right = start_rect.left;
  caret_rect_.top = end_rect.top;
  caret_rect_.bottom = end_rect.bottom;

  // When composition string is wrapped.
  if (start_rect.bottom != end_rect.bottom) {
    caret_rect_.left = caret_rect_.right = document_rect.left;
  }
  candidate_pos_.x = caret_rect_.left;
  candidate_pos_.y = caret_rect_.bottom;
  DVLOG(3) << __SHORT_FUNCTION__
           << L" left: " << caret_rect_.left
           << L" top: " << caret_rect_.top
           << L" right: " << caret_rect_.right
           << L" bottom: " << caret_rect_.bottom
           << L" clip: " << clipped;

  owner_->ui_manager()->LayoutChanged();
  return S_OK;
}

HRESULT ContextEventSink::DocumentChangedSession(
    TfEditCookie cookie, int change_flags) {
  DCHECK(write_cookie_ == TF_INVALID_COOKIE);
  if (engine_) {
    write_cookie_ = cookie;
      engine_->DocumentChanged(change_flags);
    write_cookie_ = TF_INVALID_COOKIE;
  }
  return S_OK;
}

bool ContextEventSink::GetCandidatePos(POINT *point) {
  *point = candidate_pos_;
  return true;
}

bool ContextEventSink::GetCompositionPos(POINT *point) {
  *point = composition_pos_;
  return true;
}

bool ContextEventSink::GetCompositionBoundary(RECT* rect) {
  return false;
}

bool ContextEventSink::GetCompositionFont(LOGFONT* font) {
  return false;
}

ContextInterface::ContextId ContextEventSink::GetId() {
  // Return NULL to let the frontend be destroyed immediately when the context
  // is destroyed. Because the tsf context will not changed when switching
  // between two languages in the same text service.
  return NULL;
}

void ContextEventSink::DetachEngine() {
  engine_ = NULL;
}

void ContextEventSink::AttachEngine() {
  if (engine_)
    engine_->SetContext(this);
}

}  // namespace tsf
}  // namespace ime_goopy
