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

//
// SmartComPtr: the Google COM smart pointer class.
// Why does this exist?  This class mixes the good of both ATL's CComPtr and
// CRT's _com_ptr_t classes.
//
// WARNING: SmartComPtr pointers are not thread safe. It's the callers
// responsibility to not use global or thread shared SmartComPtr pointers.

#ifndef GOOPY_COMMON_SMART_COM_PTR_H_
#define GOOPY_COMMON_SMART_COM_PTR_H_

#include <atlbase.h>
#include <atlcom.h>
#include <algorithm>
#include "base/logging.h"

// Helper functions for SmartComPtr; reduces template bloat.
class SmartComPtrHelper {
 public:
  // Compare the "true" IUnknown objects for two IUnknown pointers.
  static INT_PTR CompareUnknowns(const IUnknown *p1, const IUnknown *p2);

  // Strict IUnknown to IUnknown assignment; skips the QueryInterface().
  // Be careful, Assign will call Release on the object inside dest.
  static IUnknown *Assign(IUnknown **dest, IUnknown *src);

  // Assign an IUnknown to an IUnknown but first QueryInterface() for the
  // proper target interface. Be careful, Assign will call Release on the
  // object inside dest.
  static IUnknown *Assign(IUnknown **dest, IUnknown *src, const IID &iid);

  // Assign an IUnknown from a VARIANT and QueryInterface() for the proper
  // target interface. Be careful, Assign will call Release on the
  // object inside dest.
  static IUnknown *Assign(IUnknown **dest, const VARIANT &src,
                          const IID &iid);

  // Query an IUnknown to for iid and store the result in *dest.
  //  Be careful, Query will call Release on the object inside dest.
  static HRESULT Query(IUnknown **dest, IUnknown *src, const IID &iid);

  // Attaches to an existing object without increasing its reference count.
  // Be careful, Attach will call Release on the object inside dest.
  static IUnknown *Attach(IUnknown **dest, IUnknown *src);

  // Detaches from an existing object without decreasing its reference count.
  // Detach will set the object pointed in dest to NULL.
  static IUnknown *Detach(IUnknown **dest);

  // Creates an instance of the object specified, queries for the interface
  // specified, and stores the casted IUnknown for that interface. Any
  // previous pointer is unconditionally released.
  // Be careful, Create will call Release on the object inside dest.
  static HRESULT Create(IUnknown **dest, const CLSID &clsid, const IID &iid,
                        IUnknown *outer, DWORD context);
};

// Wraps an interface pointer to hide the AddRef() and Release() methods. This
// is done to prevent a caller from accidentally releasing an object directly,
// instead of releasing it through the smart pointer.
template<class T>
class SmartComPtrContainer: public T {
 private:
  STDMETHOD_(ULONG, AddRef)() = 0;
  STDMETHOD_(ULONG, Release)() = 0;
};

// Wraps the CComObject<T> objects, simplifying the creation and life
// management.
template <class T>
class SmartComObjPtr {
 public:
  SmartComObjPtr() : p_(NULL) {}
  explicit SmartComObjPtr(CComObject<T> *src) : p_(NULL) {
    operator=(src);
  }
  SmartComObjPtr(const SmartComObjPtr &src) : p_(NULL) {
    operator=(src);
  }
  ~SmartComObjPtr() {
    Release();
  }

  HRESULT CreateInstance() {
    Release();
    HRESULT hr = CComObject<T>::CreateInstance(&p_);
    if (FAILED(hr))
      return hr;
    p_->AddRef();
    return S_OK;
  }
  void Release() {
    operator=(NULL);
  }

  SmartComObjPtr &operator=(CComObject<T> *src) {
    if (src) src->AddRef();
    if (p_) p_->Release();
    p_ = src;
    return *this;
  }
  SmartComObjPtr &operator=(const SmartComObjPtr<T> &src) {
    return operator=(src.p_);
  }
  SmartComObjPtr &operator=(INT32 null) {
    DCHECK(null == NULL);
    return operator=(reinterpret_cast<CComObject<T>*>(NULL));
  }

  CComObject<T> **GetAddress() {
    Release();
    return &p_;
  }
  CComObject<T> *p() { return p_; }
  operator CComObject<T> *() { return p_; }
  CComObject<T> *operator->() { return p_; }
 private:
  CComObject<T> *p_;
};

// COM smart pointer.
template <class T>
class SmartComPtr {
 public:
  // Default constructor.
  SmartComPtr(): p_(NULL) {}

  // Destructor releases any held interface pointer.
  ~SmartComPtr() {
    DCHECK(this != NULL);
    if (this != NULL && p_ != NULL)
      p_->Release();
  }

  // Creates a smart pointer from any other smart pointer.
  template<class Q>
  explicit SmartComPtr(const SmartComPtr<Q> &q): p_(NULL) {
    operator=(q.p());
  }

  // Creates a smart pointer from any IUnknown-based interface pointer.
  template<class Q>
  explicit SmartComPtr(Q *q): p_(NULL) {
    operator=(q);
  }

  // Creates a smart pointer from a VARIANT.
  explicit SmartComPtr(const VARIANT &v): p_(NULL) {
    operator=(v);
  }

  // Create a smart pointer from NULL.
  explicit SmartComPtr(int null): p_(NULL) {
    operator=(null);
  }

  // Copy constructor; necessary for returning SmartComPtr types.
  explicit SmartComPtr(const SmartComPtr &q): p_(NULL) {
    operator=(q.p_);
  }

  // Creates a smart pointer by creating an object of the specified class ID
  // and querying for the specified interface type.
  SmartComPtr(const CLSID &clsid, IUnknown *outer, DWORD context): p_(NULL) {
    SmartComPtrHelper::Create(reinterpret_cast<IUnknown**>(&p_),
                              clsid, GetIID(), outer, context);
  }

  // Create new instance class instance.
  HRESULT CoCreate(const CLSID &clsid, IUnknown *outer, DWORD context) {
    return SmartComPtrHelper::Create(reinterpret_cast<IUnknown**>(&p_), clsid,
                                     GetIID(), outer, context);
  }

  // Query the SmartComPtr interface from IUnknown. The function will set
  // SmartComPtr to NULL and return the HRESULT error is case it fails.
  HRESULT QueryFrom(IUnknown* unknown) {
    return SmartComPtrHelper::Query(
        reinterpret_cast<IUnknown**>(&p_), unknown, GetIID());
  }

  // Query the SmartComPtr interface from IUnknown. The function will set
  // SmartComPtr to NULL and return the HRESULT error is case it fails.
  HRESULT QueryServiceFrom(const IID& service_guid, IUnknown* unknown) {
    HRESULT hr;
    SmartComPtr<IServiceProvider> provider;
    if (FAILED(hr = provider.QueryFrom(unknown))) return hr;
    return provider->QueryService(service_guid, GetIID(),
          reinterpret_cast<void**>(&p_));
  }

  // Assign a smart pointer from any IUnknown-based interface pointer.
  template<class Q> T *operator=(Q *q) {
    return reinterpret_cast<T*>(SmartComPtrHelper::Assign(
      reinterpret_cast<IUnknown**>(&p_), q, GetIID()));
  }

  // Specialization for assigning a smart pointer from a like-typed interface
  // pointer.
  template<> T *operator=(T *q) {
    return reinterpret_cast<T*>(SmartComPtrHelper::Assign(
        reinterpret_cast<IUnknown**>(&p_), q));
  }

  // Assign a smart pointer from any other smart pointer.
  template<class Q > T *operator=(const SmartComPtr< Q> &q) {
    return operator=(q.p());
  }

  // Assign a smart pointer from a VARIANT.
  T *operator=(const VARIANT &v) {
    return reinterpret_cast<T*>(SmartComPtrHelper::Assign(
        reinterpret_cast<IUnknown**>(&p_), v, GetIID()));
  }

  // Assign a smart pointer from NULL.
  T *operator=(INT_PTR null) {
    DCHECK(null == NULL);
    return operator=(reinterpret_cast<T*>(null));
  }

  // Copy assignment; necessary when receiving returned SmartComPtr types.
  T *operator=(const SmartComPtr &q) {
    return operator=(q.p_);
  }

  // Assign a smart pointer by passing it as an out parameter.
  T **GetAddress() {
    Release();
    return &p_;
  }

  // Attach to an existing interface pointer.  This will not increase the
  // object's reference count.
  T *Attach(T* q) {
    return reinterpret_cast<T*>(SmartComPtrHelper::Attach(
      reinterpret_cast<IUnknown**>(&p_), q));
  }

  // Detach from the interface pointer.  This will not decrease the object's
  // reference count.
  T *Detach() {
    return reinterpret_cast<T*>(SmartComPtrHelper::Detach(
      reinterpret_cast<IUnknown**>(&p_)));
  }

  // Releases the interface pointer.
  void Release() {
    operator=(reinterpret_cast<T*>(NULL));
  }

  // Compares the IUnknown interfaces of a smart pointer and any IUnknown-
  // based interface pointer.
  template<class Q> bool operator==(Q *q) const {
    return SmartComPtrHelper::CompareUnknowns(p_, q) == 0;
  }

  // Specialization for comparing a smart pointer to a like-typed interface
  // pointer.
  template<> bool operator==(T *q) const {
    return p_ == q;
  }

  // Compares the IUnknown interfaces of a smart pointer and any other smart
  // pointer.
  template<class Q > bool operator==(const SmartComPtr< Q> &q) const {
    return operator==(q.p());
  }

  // Compares the IUnknown interfaces of a smart pointer against any IUnknown-
  // based interface pointer.
  template<class Q> bool operator!=(Q *q) const {
    return SmartComPtrHelper::CompareUnknowns(p_, q) != 0;
  }

  // Specialization for comparing a smart pointer against a like-typed
  // interface pointer.
  template<> bool operator!=(T *q) const {
    return p_ != q;
  }

  // Compares the IUnknown interfaces of a smart pointer against any other
  // smart pointer.
  template<class Q > bool operator!=(const SmartComPtr< Q> &q) const {
    return operator!=(q.p());
  }

  // Compares a smart pointer with NULL.
  bool operator==(INT_PTR null) const {
    DCHECK(null == NULL);
    return operator==(reinterpret_cast<T*>(null));
  }

  // Compares a smart pointer against NULL.
  bool operator!=(INT_PTR null) const {
    DCHECK(null == NULL);
    return operator!=(reinterpret_cast<T*>(null));
  }

  // Accessors.
  T *p() const { return p_; }
  operator T*() const { return p_; }
  SmartComPtrContainer<T> *operator->() const {
    return reinterpret_cast<SmartComPtrContainer<T>*>(p_);
  }

  void Swap(SmartComPtr& sp2) {
    std::swap(p_, sp2.p_);
  }

  // Return the IID of the interface stored in this smart pointer.  This
  // function should be specialized for interfaces for which there is no
  // associated IID (eg. __uuid() does not work, IUniformResourceLocator is
  // but one example).
  static const IID &GetIID() { return __uuidof(T); }

 private:
  T *p_;
};

// Reverse comparison operators.
template<class T > bool operator==(int null, const SmartComPtr<T> &p) {
  return p == null;
}

template<class T > bool operator!=(int null, const SmartComPtr<T> &p) {
  return p != null;
}

template<class Q, class T > bool operator==(Q *q, const SmartComPtr<T> &p) {
  return p == q;
}

template<class Q, class T > bool operator!=(Q *q, const SmartComPtr<T> &p) {
  return p != q;
}

template <class T> void Swap(SmartComPtr<T>& sp1, SmartComPtr<T>& sp2) {
  sp1.Swap(sp2);
}

#endif  // GOOPY_COMMON_SMART_COM_PTR_H_
