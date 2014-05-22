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

#ifndef _CALLBACK_TYPES_H
#define _CALLBACK_TYPES_H

#include "base/type_traits.h"

struct CallbackUtils_ {
  static void FailIsRepeatable(const char* name);
};

namespace base {
namespace internal {

template <typename T>
struct ConstRef {
  typedef typename ::base::remove_reference<T>::type base_type;
  typedef const base_type& type;
};

}  // namespace internal
}  // namespace base

class Closure {
 public:
  virtual ~Closure() { }
  virtual bool IsRepeatable() const { return false; }
  virtual void CheckIsRepeatable() const { }
  virtual void Run() = 0;
  // This file was generated without tracecontext support
  // TraceContext *trace_context_ptr() { return NULL; }
 protected:
  Closure() {}
};

template <class R>
class ResultCallback {
 public:
  virtual ~ResultCallback() { }
  virtual bool IsRepeatable() const { return false; }
  virtual void CheckIsRepeatable() const { }
  virtual R Run() = 0;
  // This file was generated without tracecontext support
  // TraceContext *trace_context_ptr() { return NULL; }
 protected:
  ResultCallback() {}
};

template <class A1>
class Callback1 {
 public:
  virtual ~Callback1() { }
  virtual bool IsRepeatable() const { return false; }
  virtual void CheckIsRepeatable() const { }
  virtual void Run(A1) = 0;
  // This file was generated without tracecontext support
  // TraceContext *trace_context_ptr() { return NULL; }
 protected:
  Callback1() {}
};

template <class R, class A1>
class ResultCallback1 {
 public:
  virtual ~ResultCallback1() { }
  virtual bool IsRepeatable() const { return false; }
  virtual void CheckIsRepeatable() const { }
  virtual R Run(A1) = 0;
  // This file was generated without tracecontext support
  // TraceContext *trace_context_ptr() { return NULL; }
 protected:
  ResultCallback1() {}
};

template <class A1,class A2>
class Callback2 {
 public:
  virtual ~Callback2() { }
  virtual bool IsRepeatable() const { return false; }
  virtual void CheckIsRepeatable() const { }
  virtual void Run(A1,A2) = 0;
  // This file was generated without tracecontext support
  // TraceContext *trace_context_ptr() { return NULL; }
 protected:
  Callback2() {}
};

template <class R, class A1,class A2>
class ResultCallback2 {
 public:
  virtual ~ResultCallback2() { }
  virtual bool IsRepeatable() const { return false; }
  virtual void CheckIsRepeatable() const { }
  virtual R Run(A1,A2) = 0;
  // This file was generated without tracecontext support
  // TraceContext *trace_context_ptr() { return NULL; }
 protected:
  ResultCallback2() {}
};

template <class A1,class A2,class A3>
class Callback3 {
 public:
  virtual ~Callback3() { }
  virtual bool IsRepeatable() const { return false; }
  virtual void CheckIsRepeatable() const { }
  virtual void Run(A1,A2,A3) = 0;
  // This file was generated without tracecontext support
  // TraceContext *trace_context_ptr() { return NULL; }
 protected:
  Callback3() {}
};

template <class R, class A1,class A2,class A3>
class ResultCallback3 {
 public:
  virtual ~ResultCallback3() { }
  virtual bool IsRepeatable() const { return false; }
  virtual void CheckIsRepeatable() const { }
  virtual R Run(A1,A2,A3) = 0;
  // This file was generated without tracecontext support
  // TraceContext *trace_context_ptr() { return NULL; }
 protected:
  ResultCallback3() {}
};

template <class A1,class A2,class A3,class A4>
class Callback4 {
 public:
  virtual ~Callback4() { }
  virtual bool IsRepeatable() const { return false; }
  virtual void CheckIsRepeatable() const { }
  virtual void Run(A1,A2,A3,A4) = 0;
  // This file was generated without tracecontext support
  // TraceContext *trace_context_ptr() { return NULL; }
 protected:
  Callback4() {}
};

template <class R, class A1,class A2,class A3,class A4>
class ResultCallback4 {
 public:
  virtual ~ResultCallback4() { }
  virtual bool IsRepeatable() const { return false; }
  virtual void CheckIsRepeatable() const { }
  virtual R Run(A1,A2,A3,A4) = 0;
  // This file was generated without tracecontext support
  // TraceContext *trace_context_ptr() { return NULL; }
 protected:
  ResultCallback4() {}
};

template <class A1,class A2,class A3,class A4,class A5>
class Callback5 {
 public:
  virtual ~Callback5() { }
  virtual bool IsRepeatable() const { return false; }
  virtual void CheckIsRepeatable() const { }
  virtual void Run(A1,A2,A3,A4,A5) = 0;
  // This file was generated without tracecontext support
  // TraceContext *trace_context_ptr() { return NULL; }
 protected:
  Callback5() {}
};

template <class R, class A1,class A2,class A3,class A4,class A5>
class ResultCallback5 {
 public:
  virtual ~ResultCallback5() { }
  virtual bool IsRepeatable() const { return false; }
  virtual void CheckIsRepeatable() const { }
  virtual R Run(A1,A2,A3,A4,A5) = 0;
  // This file was generated without tracecontext support
  // TraceContext *trace_context_ptr() { return NULL; }
 protected:
  ResultCallback5() {}
};

#endif /* _CALLBACK_TYPES_H */
