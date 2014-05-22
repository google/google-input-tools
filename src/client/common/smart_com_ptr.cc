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
#include "base/logging.h"
#include "common/smart_com_ptr.h"

// Compare the "true" IUnknown objects for two IUnknown pointers.
INT_PTR SmartComPtrHelper::CompareUnknowns(const IUnknown *p1,
                                           const IUnknown *p2) {
  // If these are the same casted IUnknown, they are definitely the same
  // object.  Note that this considers two NULL pointers to be identical as
  // well.
  if ( p1 == p2 )
    return 0;

  // NOTE: because we need to call QueryInterface(), we need non-const
  // interface pointers.  But to protect the smart pointer (and make it easier
  // to use) we need const comparison operators.  The solution is to cast to
  // non-const in this function, since we know that it is safe (eg. nothing
  // in this function will cause an object to destruct, and QueryInterface()
  // should not modify the object in any real way beyond reference count).
  IUnknown *ncp1 = const_cast<IUnknown*>(p1);
  IUnknown *ncp2 = const_cast<IUnknown*>(p2);

  // NOTE: the code below queries for the "true" IUnknown but then promptly
  // releases it.  This is okay, because we are interested in the pointer's
  // raw integer value.
  IUnknown *u1 = NULL;
  if ( ncp1 != NULL ) {
    VERIFY(SUCCEEDED(ncp1->QueryInterface(IID_IUnknown,
                                          reinterpret_cast<void**>(&u1))));
    if ( u1 != NULL )
      u1->Release();
  }

  IUnknown *u2 = NULL;
  if ( ncp2 != NULL ) {
    VERIFY(SUCCEEDED(ncp2->QueryInterface(IID_IUnknown,
                                          reinterpret_cast<void**>(&u2))));
    if ( u2 != NULL )
      u2->Release();
  }

  return u1 - u2;
}

// Strict IUnknown to IUnknown assignment; skips the QueryInterface().
IUnknown *SmartComPtrHelper::Assign(IUnknown **dest, IUnknown *src) {
  DCHECK(dest != NULL);
  if (NULL == dest)
    return NULL;

  // Adding a reference to the source before releasing the current pointer
  // protects us when assigning the same pointer to itself.
  if ( src != NULL )
    src->AddRef();
  if ( *dest != NULL )
    (*dest)->Release();
  *dest = src;
  return *dest;
}

// Assign an IUnknown to an IUnknown but first QueryInterface() for the proper
// target interface.
IUnknown *SmartComPtrHelper::Assign(IUnknown **dest, IUnknown *src,
                                    const IID &iid) {
  DCHECK(dest != NULL);
  if (NULL == dest)
    return NULL;

  // QueryInterface() already adds a reference, so just hold on to the
  // current pointer until after we get a new pointer.
  IUnknown *temp = *dest;
  *dest = NULL;
  if ( src != NULL )
    src->QueryInterface(iid, reinterpret_cast<void**>(dest));
  if ( temp != NULL )
    temp->Release();
  return *dest;
}

// Assign an IUnknown to an IUnknown but first QueryInterface() for the proper
// target interface.
HRESULT SmartComPtrHelper::Query(IUnknown **dest, IUnknown *src,
                                 const IID &iid) {
  DCHECK(dest != NULL);
  if (NULL == dest)
    return E_POINTER;

  // QueryInterface() already adds a reference, so just hold on to the
  // current pointer until after we get a new pointer.
  IUnknown *temp = *dest;
  *dest = NULL;
  HRESULT hr = E_INVALIDARG;
  if ( src != NULL )
    hr = src->QueryInterface(iid, reinterpret_cast<void**>(dest));
  if ( temp != NULL )
    temp->Release();
  return hr;
}

// Assign an IUnknown from a VARIANT and QueryInterface() for the proper
// target interface.
IUnknown *SmartComPtrHelper::Assign(IUnknown **dest, const VARIANT &src,
                                    const IID &iid) {
  if ( V_VT(&src) == VT_DISPATCH )
    return Assign(dest, V_DISPATCH(&src), iid);
  if ( V_VT(&src) == VT_UNKNOWN )
    return Assign(dest, V_UNKNOWN(&src), iid);

  // The CRT smart pointer tries to change the VARIANT's type; because we may
  // have code that relies on this, that logic is duplicated here.
  CComVariant v;
  if ( SUCCEEDED(v.ChangeType(VT_DISPATCH, &src)) )
    return Assign(dest, V_DISPATCH(&v), iid);

  v.Clear();
  if ( SUCCEEDED(v.ChangeType(VT_UNKNOWN, &src)) )
    return Assign(dest, V_UNKNOWN(&v), iid);

  DCHECK(!"SmartComPtrHelper::Assign():"
          "can't convert variant to unknown or dispatch!");
  return Assign(dest, NULL);
}

// Attaches to an existing object without increasing its reference count.
IUnknown *SmartComPtrHelper::Attach(IUnknown **dest, IUnknown *src) {
  DCHECK(dest != NULL);
  if (NULL == dest)
    return NULL;

  // Adding a reference to the source before releasing the current pointer
  // protects us when attaching the same pointer to itself.  Release the
  // additional reference when done.
  if ( src != NULL )
    src->AddRef();
  if ( *dest != NULL )
    (*dest)->Release();
  *dest = src;
  if ( src != NULL )
    src->Release();
  return *dest;
}

// Detaches from an existing object without decreasing its reference count.
IUnknown *SmartComPtrHelper::Detach(IUnknown **dest) {
  DCHECK(dest != NULL);
  if (NULL == dest)
    return NULL;

  // Simply NULL the destination and return its original value.
  IUnknown *object = *dest;
  *dest = NULL;
  return object;
}

// Creates an instance of the object specified, queries for the interface
// specified, and stores the casted IUnknown for that interface.  Any previous
// pointer is unconditionally released.
HRESULT SmartComPtrHelper::Create(IUnknown **dest,
                                  const CLSID &clsid, const IID &iid,
                                  IUnknown *outer, DWORD context) {
  DCHECK(dest != NULL);
  if (NULL == dest)
    return E_POINTER;

  // Unconditionally release the current pointer, as we are creating a new
  // object anyways.
  if ( *dest != NULL ) {
    (*dest)->Release();
    *dest = NULL;
  }

  // NOTE: the following special case comes from CRT's _com_ptr_t.  It appears
  // (from searching Google Groups) that this may be necessary, though there
  // was nothing definitive.
  HRESULT hr;
  if ( context & (CLSCTX_LOCAL_SERVER | CLSCTX_REMOTE_SERVER) ) {
    IUnknown *unknown = NULL;
    hr = CoCreateInstance(clsid, outer, context, IID_IUnknown,
                          reinterpret_cast<void**>(&unknown));
    if ( SUCCEEDED(hr) && unknown != NULL ) {
      hr = OleRun(unknown);
      if ( SUCCEEDED(hr) )
        hr = unknown->QueryInterface(iid, reinterpret_cast<void**>(dest));
      unknown->Release();
    }
  } else {
    hr = CoCreateInstance(clsid, outer, context, iid,
                          reinterpret_cast<void**>(dest));
  }

  DCHECK(*dest != NULL || FAILED(hr));
  return hr;
}
