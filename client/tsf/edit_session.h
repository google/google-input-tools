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

// Usage:
// RequestEditSession(context, client_id, callback_with_x_args,
//                    arg1, arg2, ..., argX, flag);
// The function would request an edit session, and the callback
// would be invoked with the provided args when the session is
// acquired.

#ifndef GOOPY_TSF_EDIT_SESSION_H_
#define GOOPY_TSF_EDIT_SESSION_H_

#include <atlbase.h>
#include <atlcom.h>
#include <msctf.h>

#include "base/basictypes.h"
#include "base/callback.h"
#include "base/scoped_ptr.h"
#include "common/smart_com_ptr.h"

namespace ime_goopy {
namespace tsf {

// EditSessionX is the adapter from a X-parameter callback object to an
// ITfEditSession callback. You don't want to use them directly.
// Use the helper function RequestEditSession, which would take care of
// the creation of the adapter.
// We adds reference to the callback_owner, to prevent it from being
// deleted until the EditSession is executed. If this is not necessary
// a NULL owner could be passed in.
class ATL_NO_VTABLE EditSession0
    : public CComObjectRootEx<CComSingleThreadModel>,
      public ITfEditSession {
 public:
  typedef ResultCallback1<HRESULT, TfEditCookie> SessionCallback;

  DECLARE_NOT_AGGREGATABLE(EditSession0)
  BEGIN_COM_MAP(EditSession0)
    COM_INTERFACE_ENTRY(ITfEditSession)
  END_COM_MAP()
  DECLARE_PROTECT_FINAL_CONSTRUCT()

  EditSession0() {}
  ~EditSession0() {}

  void Initialize(IUnknown *callback_owner, SessionCallback* callback) {
    callback_owner_ = callback_owner;
    callback_.reset(callback);
  }

 private:
  // ITfEditSession
  STDMETHODIMP DoEditSession(TfEditCookie cookie) {
    HRESULT hr = callback_->Run(cookie);
    callback_owner_ = NULL;
    return hr;
  }

  SmartComPtr<IUnknown> callback_owner_;
  scoped_ptr<SessionCallback> callback_;
  DISALLOW_EVIL_CONSTRUCTORS(EditSession0);
};

// T is the type of the callback argument
template <class T>
class ATL_NO_VTABLE EditSession1
    : public CComObjectRootEx<CComSingleThreadModel>,
      public ITfEditSession {
 public:
  typedef ResultCallback2<HRESULT, TfEditCookie, T> SessionCallback;

  DECLARE_NOT_AGGREGATABLE(EditSession1)
  BEGIN_COM_MAP(EditSession1)
    COM_INTERFACE_ENTRY(ITfEditSession)
  END_COM_MAP()
  DECLARE_PROTECT_FINAL_CONSTRUCT()

  EditSession1() {}
  ~EditSession1() {}

  void Initialize(IUnknown *callback_owner, SessionCallback* callback,
                  T callback_arg) {
    callback_owner_ = callback_owner;
    callback_.reset(callback);
    callback_arg_ = callback_arg;
  }

 private:
  // ITfEditSession
  STDMETHODIMP DoEditSession(TfEditCookie cookie) {
    HRESULT hr = callback_->Run(cookie, callback_arg_);
    callback_owner_ = NULL;
    return hr;
  }

  SmartComPtr<IUnknown> callback_owner_;
  scoped_ptr<SessionCallback> callback_;
  T callback_arg_;
  DISALLOW_EVIL_CONSTRUCTORS(EditSession1);
};

// T, U are the types of the two callback arguments
template <class T, class U>
class ATL_NO_VTABLE EditSession2
    : public CComObjectRootEx<CComSingleThreadModel>,
      public ITfEditSession {
 public:
  typedef ResultCallback3<HRESULT, TfEditCookie, T, U> SessionCallback;

  DECLARE_NOT_AGGREGATABLE(EditSession2)
  BEGIN_COM_MAP(EditSession2)
    COM_INTERFACE_ENTRY(ITfEditSession)
  END_COM_MAP()
  DECLARE_PROTECT_FINAL_CONSTRUCT()

  EditSession2() {}
  ~EditSession2() {}

  void Initialize(IUnknown *callback_owner, SessionCallback* callback,
                  T callback_arg1, U callback_arg2) {
    callback_owner_ = callback_owner;
    callback_.reset(callback);
    callback_arg1_ = callback_arg1;
    callback_arg2_ = callback_arg2;
  }

 private:
  // ITfEditSession
  STDMETHODIMP DoEditSession(TfEditCookie cookie) {
    HRESULT hr = callback_->Run(cookie, callback_arg1_, callback_arg2_);
    callback_owner_ = NULL;
    return hr;
  }

  SmartComPtr<IUnknown> callback_owner_;
  scoped_ptr<SessionCallback> callback_;
  T callback_arg1_;
  U callback_arg2_;
  DISALLOW_EVIL_CONSTRUCTORS(EditSession2);
};

// T, U, V are the types of the three callback arguments
template <class T, class U, class V>
class ATL_NO_VTABLE EditSession3
    : public CComObjectRootEx<CComSingleThreadModel>,
      public ITfEditSession {
 public:
  typedef ResultCallback4<HRESULT, TfEditCookie, T, U, V> SessionCallback;

  DECLARE_NOT_AGGREGATABLE(EditSession3)
  BEGIN_COM_MAP(EditSession3)
    COM_INTERFACE_ENTRY(ITfEditSession)
  END_COM_MAP()
  DECLARE_PROTECT_FINAL_CONSTRUCT()

  EditSession3() {}
  ~EditSession3() {}

  void Initialize(IUnknown *callback_owner, SessionCallback* callback,
                  T callback_arg1, U callback_arg2, V callback_arg3) {
    callback_owner_ = callback_owner;
    callback_.reset(callback);
    callback_arg1_ = callback_arg1;
    callback_arg2_ = callback_arg2;
    callback_arg3_ = callback_arg3;
  }

 private:
  // ITfEditSession
  STDMETHODIMP DoEditSession(TfEditCookie cookie) {
    HRESULT hr = callback_->Run(
        cookie, callback_arg1_, callback_arg2_, callback_arg3_);
    callback_owner_ = NULL;
    return hr;
  }

  SmartComPtr<IUnknown> callback_owner_;
  scoped_ptr<SessionCallback> callback_;
  T callback_arg1_;
  T callback_arg2_;
  T callback_arg3_;
  DISALLOW_EVIL_CONSTRUCTORS(EditSession3);
};

inline HRESULT RequestEditSession(
    ITfContext *context,
    TfClientId client_id,
    IUnknown *callback_owner,
    ResultCallback1<HRESULT, TfEditCookie> *callback,
    DWORD flags) {
  SmartComObjPtr<EditSession0> edit_session;
  HRESULT hr = edit_session.CreateInstance();
  if (FAILED(hr))
    return hr;

  edit_session->Initialize(callback_owner, callback);
  HRESULT hr2 = context->RequestEditSession(
      client_id, edit_session, flags, &hr);
  if (FAILED(hr) || FAILED(hr2))
    return E_FAIL;

  return S_OK;
}

template <class T>
HRESULT RequestEditSession(
    ITfContext *context,
    TfClientId client_id,
    IUnknown *callback_owner,
    ResultCallback2<HRESULT, TfEditCookie, T> *callback,
    T callback_arg,
    DWORD flags) {
  SmartComObjPtr<EditSession1<T> > edit_session;
  HRESULT hr = edit_session.CreateInstance();
  if (FAILED(hr))
    return hr;

  edit_session->Initialize(callback_owner, callback, callback_arg);
  HRESULT hr2 = context->RequestEditSession(
      client_id, edit_session, flags, &hr);
  if (FAILED(hr) || FAILED(hr2))
    return E_FAIL;

  return S_OK;
}

template <class T, class U>
HRESULT RequestEditSession(
    ITfContext *context,
    TfClientId client_id,
    IUnknown *callback_owner,
    ResultCallback3<HRESULT, TfEditCookie, T, U> *callback,
    T callback_arg1,
    U callback_arg2,
    DWORD flags) {
  SmartComObjPtr<EditSession2<T, U> > edit_session;
  HRESULT hr = edit_session.CreateInstance();
  if (FAILED(hr))
    return hr;

  edit_session->Initialize(callback_owner, callback,
                           callback_arg1, callback_arg2);
  HRESULT hr2 = context->RequestEditSession(
      client_id, edit_session, flags, &hr);
  if (FAILED(hr) || FAILED(hr2))
    return E_FAIL;

  return S_OK;
}

template <class T, class U, class V>
HRESULT RequestEditSession(
    ITfContext *context,
    TfClientId client_id,
    IUnknown *callback_owner,
    ResultCallback4<HRESULT, TfEditCookie, T, U, V> *callback,
    T callback_arg1,
    U callback_arg2,
    V callback_arg3,
    DWORD flags) {
  SmartComObjPtr<EditSession3<T, U, V> > edit_session;
  HRESULT hr = edit_session.CreateInstance();
  if (FAILED(hr))
    return hr;

  edit_session->Initialize(callback_owner, callback,
                           callback_arg1, callback_arg2, callback_arg3);
  HRESULT hr2 = context->RequestEditSession(
      client_id, edit_session, flags, &hr);
  if (FAILED(hr) || FAILED(hr2))
    return E_FAIL;

  return S_OK;
}

}  // namespace tsf
}  // namespace ime_goopy

#endif  // GOOPY_TSF_EDIT_SESSION_H_
