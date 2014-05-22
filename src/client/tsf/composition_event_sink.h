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

#ifndef GOOPY_TSF_COMPOSITION_EVENT_SINK_H_
#define GOOPY_TSF_COMPOSITION_EVENT_SINK_H_

#include <atlbase.h>
#include <atlcom.h>
#include <msctf.h>

#include <string>
#include "base/basictypes.h"
#include "base/scoped_ptr.h"
#include "common/smart_com_ptr.h"

namespace ime_goopy {
class EngineInterface;
namespace tsf {
class ContextEventSink;
class ATL_NO_VTABLE CompositionEventSink
    : public CComObjectRootEx<CComSingleThreadModel>,
      public ITfCompositionSink,
      public ITfMouseSink {
 public:
  DECLARE_NOT_AGGREGATABLE(CompositionEventSink)
  BEGIN_COM_MAP(CompositionEventSink)
    COM_INTERFACE_ENTRY(ITfCompositionSink)
    COM_INTERFACE_ENTRY(ITfMouseSink)
  END_COM_MAP()
  DECLARE_PROTECT_FINAL_CONSTRUCT()

  CompositionEventSink();
  ~CompositionEventSink();

  HRESULT Initialize(ContextEventSink* context_event_sink);
  HRESULT Uninitialize();

  // Invoked when an edit session is acquired.
  HRESULT Update(TfEditCookie cookie,
                 const std::wstring& composition,
                 int caret);
  HRESULT UpdateCallback(TfEditCookie cookie,
                         std::wstring composition,
                         int caret);
  HRESULT CommitResult(TfEditCookie cookie, const std::wstring& result);
  HRESULT CommitResultForCallback(TfEditCookie cookie, std::wstring result);
  // Invoked when an edit session is acquired. This will reconvert a
  // section of text into composition.
  HRESULT Reconvert(TfEditCookie cookie, ITfRange *range);

  // ITfCompositionSink
  STDMETHODIMP OnCompositionTerminated(TfEditCookie cookie,
                                       ITfComposition *composition);

  // ITfMouseSink
  STDMETHODIMP OnMouseEvent(ULONG edge,
                            ULONG quadrant,
                            DWORD button_status,
                            BOOL* eaten);
  ITfRange* range() { return composition_range_; }

 private:
  // Retreives the inserting point and starts composition.
  HRESULT StartComposition(TfEditCookie cookie);
  HRESULT StartCompositionAt(TfEditCookie cookie, ITfRange *range);
  // Updates the composition area with the latest contents,
  // and updates the caret position.
  HRESULT UpdateComposition(TfEditCookie cookie,
                            const std::wstring& composition,
                            int caret);
  // Updates the composition area with the input result from engine,
  // and ends the composition.
  HRESULT EndComposition(TfEditCookie cookie, const std::wstring& result);

  HRESULT ApplyInputAttribute(TfEditCookie cookie);
  HRESULT ClearInputAttribute(TfEditCookie cookie);
  HRESULT SetCaretPosition(TfEditCookie cookie, int position);
  HRESULT AdviseMouseSink(ITfRange* range);
  HRESULT ProcessMouseEventSession(
      TfEditCookie cookie, DWORD button_status, int offset);

  bool Ready() { return context_.p() != NULL; }
  bool Composing() { return composition_range_.p() != NULL; }

  ContextEventSink *context_event_sink_;
  SmartComPtr<ITfContext> context_;
  TfClientId client_id_;
  EngineInterface* engine_;

  // The following members are only available when composing. These variables
  // may become invalid if the context and our CompositionEventSink class are
  // out of sync. This will never happen if we process the events of context
  // properly. If there are bugs about this in the future, we should not cache
  // these variables.
  SmartComPtr<ITfComposition> composition_;
  SmartComPtr<ITfRange> composition_range_;
  DWORD mouse_sink_cookie_;
  DISALLOW_EVIL_CONSTRUCTORS(CompositionEventSink);
};
}  // namespace tsf
}  // namespace ime_goopy
#endif  // GOOPY_TSF_COMPOSITION_EVENT_SINK_H_
