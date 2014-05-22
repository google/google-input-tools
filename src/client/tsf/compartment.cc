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

#include "tsf/compartment.h"

namespace ime_goopy {
namespace tsf {
Compartment::Compartment(TfClientId client_id,
                         IUnknown* source,
                         const GUID& guid,
                         ITfCompartmentEventSink* sink) {
  SmartComPtr<ITfCompartmentMgr> compartment_manager(source);
  if (!compartment_manager) return;

  compartment_manager->GetCompartment(
      guid, compartment_.GetAddress());
  client_id_ = client_id;

  if (sink) compartment_event_sink_advisor_.Advise(compartment_, sink);
}

Compartment::~Compartment() {
}

bool Compartment::Ready() {
  return compartment_ != NULL;
}

HRESULT Compartment::GetInteger(DWORD* value) {
  if (!value) return E_INVALIDARG;
  if (!Ready()) return E_UNEXPECTED;

  VARIANT var;
  HRESULT hr = compartment_->GetValue(&var);
  // hr may be S_FALSE, means the value is empty.
  if (hr != S_OK) return hr;

  if (var.vt == VT_I4) {
    *value = var.lVal;
    return S_OK;
  } else {
    return E_UNEXPECTED;
  }
}

HRESULT Compartment::SetInteger(DWORD value) {
  if (!Ready()) return E_UNEXPECTED;

  VARIANT var;
  var.vt = VT_I4;
  var.lVal = value;
  return compartment_->SetValue(client_id_, &var);
}

}  // namespace tsf
}  // namespace ime_goopy
