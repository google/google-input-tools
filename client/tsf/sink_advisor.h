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

#ifndef GOOPY_TSF_SINK_ADVISOR_H_
#define GOOPY_TSF_SINK_ADVISOR_H_

#include <atlbase.h>
#include <atlcomcli.h>
#include <msctf.h>
#include <string>
#include "base/basictypes.h"
#include "base/logging.h"
#include "common/smart_com_ptr.h"

namespace ime_goopy {
namespace tsf {

// Advise and unadvise TSF sink.
template <class SinkInterface>
class SinkAdvisor {
 public:
  SinkAdvisor() : cookie_(TF_INVALID_COOKIE) {
  }

  virtual ~SinkAdvisor() {
    if (IsAdvised()) Unadvise();
  }

  bool IsAdvised() {
    return cookie_ != TF_INVALID_COOKIE;
  }

  HRESULT Advise(IUnknown* source, SinkInterface* sink) {
    if (!source || !sink) return E_INVALIDARG;
    if (IsAdvised()) return E_UNEXPECTED;

    SmartComPtr<ITfSource> src(source);
    if (!src) return E_NOINTERFACE;

    HRESULT hr = src->AdviseSink(__uuidof(SinkInterface),
                                 sink,
                                 &cookie_);
    if (FAILED(hr)) {
      cookie_ = TF_INVALID_COOKIE;
      return hr;
    }
    source_ = src;
    return hr;
  }

  HRESULT Unadvise() {
    if (!IsAdvised()) return E_UNEXPECTED;

    HRESULT hr = source_->UnadviseSink(cookie_);
    cookie_ = TF_INVALID_COOKIE;
    source_.Release();
    return hr;
  }

 private:
  DWORD cookie_;
  SmartComPtr<ITfSource> source_;
  DISALLOW_EVIL_CONSTRUCTORS(SinkAdvisor);
};

// Advise and unadvise TSF single sink.
template <class SinkInterface>
class SingleSinkAdvisor {
 public:
  SingleSinkAdvisor() : client_id_(TF_CLIENTID_NULL) {
  }

  virtual ~SingleSinkAdvisor() {
    if (IsAdvised()) Unadvise();
  }

  bool IsAdvised() {
    return client_id_ != TF_CLIENTID_NULL;
  }

  HRESULT Advise(IUnknown* source, TfClientId client_id, SinkInterface *sink) {
    if (!source || !client_id || !sink)
      return E_INVALIDARG;
    if (IsAdvised()) return E_UNEXPECTED;

    SmartComPtr<ITfSourceSingle> src(source);
    if (!src)
      return E_NOINTERFACE;

    HRESULT hr = src->AdviseSingleSink(
        client_id, __uuidof(SinkInterface), sink);
    if (FAILED(hr)) return hr;

    source_ = src;
    client_id_ = client_id;
    return hr;
  }

  HRESULT Unadvise() {
    if (!IsAdvised()) return E_UNEXPECTED;

    HRESULT hr = source_->UnadviseSingleSink(
        client_id_, __uuidof(SinkInterface));
    source_.Release();
    client_id_ = TF_CLIENTID_NULL;
    return hr;
  }

 private:
  SmartComPtr<ITfSourceSingle> source_;
  TfClientId client_id_;
  DISALLOW_EVIL_CONSTRUCTORS(SingleSinkAdvisor);
};

}  // namespace tsf
}  // namespace ime_goopy
#endif  // GOOPY_TSF_SINK_ADVISOR_H_

