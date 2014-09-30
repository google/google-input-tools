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

// Compartment is a part of Text Service Framework, which is used to share
// information between applications and text services. As a keyboard text
// service, we monitor the following compartment:
//   GUID_COMPARTMENT_KEYBOARD_OPENCLOSE
//   GUID_COMPARTMENT_KEYBOARD_DISABLED
//   GUID_COMPARTMENT_EMPTYCONTEXT
// The first one is associated with a thread manager, and the following ones
// are associated with a context.
// We also set the following compartment to let TSF knows our state:
//   GUID_COMPARTMENT_KEYBOARD_INPUTMODE_CONVERSION
//   GUID_COMPARTMENT_KEYBOARD_INPUTMODE_SENTENCE

#ifndef GOOPY_TSF_COMPARTMENT_H_
#define GOOPY_TSF_COMPARTMENT_H_

#include <atlbase.h>
#include <atlcom.h>
#include <msctf.h>

#include "base/basictypes.h"
#include "base/scoped_ptr.h"
#include "common/smart_com_ptr.h"
#include "tsf/sink_advisor.h"

namespace ime_goopy {
namespace tsf {
class Compartment {
 public:
  Compartment(TfClientId client_id,
              IUnknown* source,
              const GUID& guid,
              ITfCompartmentEventSink* sink);
  ~Compartment();

  bool Ready();
  bool Advised();

  HRESULT GetInteger(DWORD* value);
  HRESULT SetInteger(DWORD value);

 private:
  TfClientId client_id_;
  SmartComPtr<ITfCompartment> compartment_;
  SinkAdvisor<ITfCompartmentEventSink> compartment_event_sink_advisor_;
};
}  // namespace tsf
}  // namespace ime_goopy
#endif  // GOOPY_TSF_COMPARTMENT_H_
