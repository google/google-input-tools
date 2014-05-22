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

#ifndef GOOPY_TSF_CONTEXT_EVENT_SINK_H_
#define GOOPY_TSF_CONTEXT_EVENT_SINK_H_

#include <atlbase.h>
#include <atlcom.h>
#include <msctf.h>
#include <ctffunc.h>

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "base/scoped_ptr.h"
#include "common/framework_interface.h"
#include "common/smart_com_ptr.h"
#include "tsf/candidates.h"
#include "tsf/sink_advisor.h"

namespace ime_goopy {
class EditCtrlTextRange;
class HtmlTextRange;
namespace tsf {
class Candidates;
class Compartment;
class CompositionEventSink;
template <class T> class EditSession;
class ExternalCandidateUI;
class KeyEventSink;
class ThreadManagerEventSink;

// A Context represent a typing environment in host application.
// ContextEventSink receives commands and events from the system,
// pass those messages to engine and notify the system about the
// changes of engine.
class ATL_NO_VTABLE ContextEventSink
    : public CComObjectRootEx<CComSingleThreadModel>,
      public ContextInterface,
      public ITfCleanupContextSink,
      public ITfTextLayoutSink,
      public ITfTextEditSink,
      public ITfCompartmentEventSink {
 public:
  DECLARE_NOT_AGGREGATABLE(ContextEventSink)
  BEGIN_COM_MAP(ContextEventSink)
    COM_INTERFACE_ENTRY(ITfCleanupContextSink)
    COM_INTERFACE_ENTRY(ITfTextLayoutSink)
    COM_INTERFACE_ENTRY(ITfTextEditSink)
    COM_INTERFACE_ENTRY(ITfCompartmentEventSink)
  END_COM_MAP()
  DECLARE_PROTECT_FINAL_CONSTRUCT()

  ContextEventSink();
  ~ContextEventSink();

  HRESULT Initialize(ThreadManagerEventSink *owner, ITfContext *context);
  HRESULT Uninitialize();
  KeyEventSink *key_event_sink();
  ITfContext *context() { return context_; }
  TfClientId client_id() { return client_id_; }
  TfEditCookie write_cookie() { return write_cookie_; }
  void set_write_cookie(TfEditCookie cookie) { write_cookie_ = cookie; }

  // ITfCleanupContextSink
  // Invoked when a context is about to remove.
  STDMETHODIMP OnCleanupContext(TfEditCookie cookie, ITfContext *context);

  // ITfTextLayoutSink
  // A layout change usually happens when the document window changes its
  // position or size. In this case we should recalculate the position of the
  // candidate window.
  STDMETHODIMP OnLayoutChange(ITfContext* context,
                              TfLayoutCode code,
                              ITfContextView* context_view);

  // ITfTextEditSink
  STDMETHODIMP OnEndEdit(ITfContext *context,
                         TfEditCookie readonly_cookie,
                         ITfEditRecord *edit_record);

  // ITfCompartmentEventSink
  STDMETHODIMP OnChange(REFGUID rguid);

  // ContextInterface Actions.
  virtual void UpdateComposition(const std::wstring& composition,
                                 int caret) OVERRIDE;
  virtual void CommitResult(const std::wstring& result) OVERRIDE;
  virtual void UpdateCandidates(
      bool is_compositing,
      const ipc::proto::CandidateList& candiate_list) OVERRIDE;
  virtual void UpdateStatus(
      bool native, bool full_shape, bool full_punct) OVERRIDE;

  // ContextInterface Information.
  Platform GetPlatform();
  EngineInterface* GetEngine();
  // Returns true if client rect of input context window is set in rect.
  virtual bool GetClientRect(RECT *client_rect) OVERRIDE;
  virtual bool GetCaretRectForCandidate(RECT* rect) OVERRIDE;
  virtual bool GetCaretRectForComposition(RECT* rect) OVERRIDE;
  virtual bool GetCandidatePos(POINT *point) OVERRIDE;
  virtual bool GetCompositionPos(POINT *point) OVERRIDE;
  virtual bool GetCompositionBoundary(RECT* rect) OVERRIDE;
  virtual bool GetCompositionFont(LOGFONT* font) OVERRIDE;
  TextRangeInterface *GetSelectionRange();
  TextRangeInterface *GetCompositionRange();
  virtual bool ShouldShow(UIComponent ui_type) OVERRIDE;
  virtual ContextId GetId();
  virtual void DetachEngine();

  void AttachEngine();

 private:
  friend class TextRange;
  // Callback functions when the edit session is acquired.

  // Gets the coordinates of the composition after OnLayoutChange.
  // So that we can update the new position of the candidate
  // window accordingly.
  HRESULT LayoutChangeSession(TfEditCookie cookie, bool by_layout_change);
  HRESULT DocumentChangedSession(TfEditCookie cookie, int change_flags);
  void FixHtmlCaretPosition();

  EngineInterface* engine_;
  ThreadManagerEventSink *owner_;
  UIManagerInterface* ui_manager_;
  TfClientId client_id_;
  // Current edit session cookie.
  TfEditCookie write_cookie_;

  SmartComPtr<ITfContext> context_;

  scoped_ptr<Candidates> candidates_;
  scoped_ptr<Compartment> empty_context_;
  scoped_ptr<Compartment> keyboard_disabled_;

  SmartComObjPtr<CompositionEventSink> composition_event_sink_;
  SmartComObjPtr<KeyEventSink> key_event_sink_;

  SingleSinkAdvisor<ITfCleanupContextSink> cleanup_context_sink_advisor;
  SinkAdvisor<ITfTextLayoutSink> text_layout_sink_advisor_;
  SinkAdvisor<ITfTextEditSink> text_edit_sink_advisor_;

  RECT caret_rect_;
  POINT composition_pos_;
  POINT candidate_pos_;
  DISALLOW_EVIL_CONSTRUCTORS(ContextEventSink);
};

}  // namespace tsf
}  // namespace ime_goopy
#endif  // GOOPY_TSF_CONTEXT_EVENT_SINK_H_
