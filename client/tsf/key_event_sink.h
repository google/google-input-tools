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

#ifndef GOOPY_TSF_KEY_EVENT_SINK_H_
#define GOOPY_TSF_KEY_EVENT_SINK_H_

#include <atlbase.h>
#include <atlcom.h>
#include <msctf.h>

#include "base/basictypes.h"
#include "common/smart_com_ptr.h"
#include "ipc/protos/ipc.pb.h"
#include "tsf/sink_advisor.h"
#include <gtest/gtest_prod.h>

namespace ime_goopy {
class EngineInterface;
namespace tsf {
class ContextEventSink;
// Receive keyboard events, recognize the key and send them to engine and UI.
class ATL_NO_VTABLE KeyEventSink
    : public CComObjectRootEx<CComSingleThreadModel>,
      public ITfContextKeyEventSink {
 public:
  DECLARE_NOT_AGGREGATABLE(KeyEventSink)
  BEGIN_COM_MAP(KeyEventSink)
    COM_INTERFACE_ENTRY(ITfContextKeyEventSink)
  END_COM_MAP()
  DECLARE_PROTECT_FINAL_CONSTRUCT()

  KeyEventSink();
  ~KeyEventSink();

  HRESULT Initialize(ContextEventSink *context_event_sink);
  HRESULT Uninitialize();
  void set_enabled(bool value) { enabled_ = value; }

  // ITfContextKeyEventSink
  STDMETHODIMP OnKeyDown(WPARAM wparam, LPARAM lparam, BOOL *eaten);
  STDMETHODIMP OnKeyUp(WPARAM wparam, LPARAM lparam, BOOL *eaten);
  STDMETHODIMP OnTestKeyDown(WPARAM wparam, LPARAM lparam, BOOL *eaten);
  STDMETHODIMP OnTestKeyUp(WPARAM wparam, LPARAM lparam, BOOL *eaten);

  FRIEND_TEST(KeyEventSink, TestKey);
  FRIEND_TEST(KeyEventSink, Key);

 private:
   HRESULT ProcessKeySession(TfEditCookie cookie, ipc::proto::KeyEvent key);
  SmartComPtr<ITfContext> context_;
  TfClientId client_id_;
  ContextEventSink *context_event_sink_;

  EngineInterface* engine_;
  bool enabled_;
  bool engine_eaten_;
  static const HKL english_hkl_;
  SinkAdvisor<ITfContextKeyEventSink> key_event_sink_advisor_;
};
}  // namespace tsf
}  // namespace ime_goopy
#endif  // GOOPY_TSF_KEY_EVENT_SINK_H_
