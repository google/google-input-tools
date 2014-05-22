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

#include <atlbase.h>
#include <atlcom.h>
#include <atlcomcli.h>
#include <msctf.h>

#include "base/scoped_ptr.h"
#include "tsf/sink_advisor.h"
#include <gtest/gtest_prod.h>

namespace ime_goopy {
namespace tsf {

static const int kTestCookie = 5;
class ATL_NO_VTABLE MockSinkSource
    : public CComObjectRootEx<CComSingleThreadModel>,
      public ITfSource {
 public:
  DECLARE_NOT_AGGREGATABLE(MockSinkSource)
  BEGIN_COM_MAP(MockSinkSource)
    COM_INTERFACE_ENTRY(ITfSource)
  END_COM_MAP()
  DECLARE_PROTECT_FINAL_CONSTRUCT()

  MockSinkSource()
      : advise_called_(0),
        unadvise_called_(0),
        cookie_(kTestCookie) {
  }

  ~MockSinkSource() {
  }

  STDMETHODIMP AdviseSink(REFIID riid, IUnknown *punk, DWORD *cookie) {
    advise_called_++;
    *cookie = cookie_;
    sink_ = punk;
    return S_OK;
  }

  STDMETHODIMP UnadviseSink(DWORD cookie) {
    unadvise_called_++;
    sink_.Release();
    return S_OK;
  }

  void CallSink() {
    ASSERT_TRUE(sink_);
    sink_->OnSetThreadFocus();
  }

  int advise_called() { return advise_called_; }
  int unadvise_called() { return unadvise_called_; }

 private:
  int advise_called_;
  int unadvise_called_;
  DWORD cookie_;
  SmartComPtr<ITfThreadFocusSink> sink_;
  DISALLOW_EVIL_CONSTRUCTORS(MockSinkSource);
};

class ATL_NO_VTABLE MockSinkTarget
    : public CComObjectRootEx<CComSingleThreadModel>,
      public ITfThreadFocusSink {
 public:
  DECLARE_NOT_AGGREGATABLE(MockSinkTarget)
  BEGIN_COM_MAP(MockSinkTarget)
    COM_INTERFACE_ENTRY(ITfThreadFocusSink)
  END_COM_MAP()
  DECLARE_PROTECT_FINAL_CONSTRUCT()

  MockSinkTarget() : on_set_focus_called_(0) {
  }

  ~MockSinkTarget() {
  }

  // ITfThreadFocusSink
  STDMETHODIMP OnSetThreadFocus() {
    on_set_focus_called_++;
    return S_OK;
  }

  STDMETHODIMP OnKillThreadFocus() {
    return S_OK;
  }

  int set_focus_called() { return on_set_focus_called_; }

 private:
  int on_set_focus_called_;
  DISALLOW_EVIL_CONSTRUCTORS(MockSinkTarget);
};

TEST(SinkAdvisor, Test) {
  SmartComObjPtr<MockSinkSource> source;
  source.CreateInstance();

  SmartComObjPtr<MockSinkTarget> target;
  target.CreateInstance();

  SinkAdvisor<ITfThreadFocusSink> advisor;
  EXPECT_FALSE(advisor.IsAdvised());

  advisor.Advise(source, target);
  EXPECT_EQ(1, source->advise_called());
  EXPECT_TRUE(advisor.IsAdvised());

  source->CallSink();
  EXPECT_EQ(1, target->set_focus_called());

  advisor.Unadvise();
  EXPECT_EQ(1, source->unadvise_called());
  EXPECT_FALSE(advisor.IsAdvised());
}

static const int kTestClientId = 10;

class ATL_NO_VTABLE MockSingleSinkSource
    : public CComObjectRootEx<CComSingleThreadModel>,
      public ITfSourceSingle {
 public:
  DECLARE_NOT_AGGREGATABLE(MockSingleSinkSource)
  BEGIN_COM_MAP(MockSingleSinkSource)
    COM_INTERFACE_ENTRY(ITfSourceSingle)
  END_COM_MAP()
  DECLARE_PROTECT_FINAL_CONSTRUCT()

  MockSingleSinkSource()
      : advise_called_(0),
        unadvise_called_(0),
        cookie_(kTestCookie) {
  }

  ~MockSingleSinkSource() {
  }

  STDMETHODIMP AdviseSingleSink(TfClientId client_id,
                                REFIID riid,
                                IUnknown *punk) {
    advise_called_++;
    sink_ = punk;
    return S_OK;
  }

  STDMETHODIMP UnadviseSingleSink(TfClientId client_id, REFIID riid) {
    unadvise_called_++;
    sink_.Release();
    return S_OK;
  }

  void CallSink() {
    ASSERT_TRUE(sink_);
    sink_->OnSetThreadFocus();
  }

  int advise_called() { return advise_called_; }
  int unadvise_called() { return unadvise_called_; }

 private:
  int advise_called_;
  int unadvise_called_;
  DWORD cookie_;
  SmartComPtr<ITfThreadFocusSink> sink_;
  DISALLOW_EVIL_CONSTRUCTORS(MockSingleSinkSource);
};

class ATL_NO_VTABLE MockSingleSinkTarget
    : public CComObjectRootEx<CComSingleThreadModel>,
      public ITfThreadFocusSink {
 public:
  DECLARE_NOT_AGGREGATABLE(MockSingleSinkTarget)
  BEGIN_COM_MAP(MockSingleSinkTarget)
    COM_INTERFACE_ENTRY(ITfThreadFocusSink)
  END_COM_MAP()
  DECLARE_PROTECT_FINAL_CONSTRUCT()

  MockSingleSinkTarget() : on_set_focus_called_(0) {
  }

  ~MockSingleSinkTarget() {
  }

  // ITfThreadFocusSink
  STDMETHODIMP OnSetThreadFocus() {
    on_set_focus_called_++;
    return S_OK;
  }

  STDMETHODIMP OnKillThreadFocus() {
    return S_OK;
  }

  int set_focus_called() { return on_set_focus_called_; }

 private:
  int on_set_focus_called_;
  DISALLOW_EVIL_CONSTRUCTORS(MockSingleSinkTarget);
};

TEST(SingleSinkAdvisor, Test) {
  SmartComObjPtr<MockSingleSinkSource> source;
  source.CreateInstance();

  SmartComObjPtr<MockSingleSinkTarget> target;
  target.CreateInstance();

  SingleSinkAdvisor<ITfThreadFocusSink> advisor;

  EXPECT_FALSE(advisor.IsAdvised());

  advisor.Advise(source, kTestClientId, target);
  EXPECT_EQ(1, source->advise_called());
  EXPECT_TRUE(advisor.IsAdvised());

  source->CallSink();
  EXPECT_EQ(1, target->set_focus_called());

  advisor.Unadvise();
  EXPECT_EQ(1, source->unadvise_called());
  EXPECT_FALSE(advisor.IsAdvised());
}

}  // namespace tsf
}  // namespace ime_goopy
