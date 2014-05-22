/*
  Copyright 2011 Google Inc.

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
#ifndef GGADGET_WIN32_THREAD_LOCAL_SINGLETON_HOLDER_H__
#define GGADGET_WIN32_THREAD_LOCAL_SINGLETON_HOLDER_H__

#include <windows.h>
#include "ggadget/common.h"

namespace ggadget {
namespace win32 {

/**
 * This class holds a pointer of type T for each thread, it's not responsible
 * for creating/deleting the singleton.
 */
template<typename T>
class ThreadLocalSingletonHolder {
 public:
  static T *GetValue() {
    if (tls_index_ == TLS_OUT_OF_INDEXES)
      return NULL;

    return reinterpret_cast<T *>(TlsGetValue(tls_index_));
  }

  static bool SetValue(T *value) {
    /** Create thread local storage. */
    if (tls_index_ == TLS_OUT_OF_INDEXES) {
      DWORD index = TlsAlloc();
      ASSERT(index != TLS_OUT_OF_INDEXES);
      if (index == TLS_OUT_OF_INDEXES)
        return false;

      if (InterlockedCompareExchange(
          &tls_index_, index, TLS_OUT_OF_INDEXES) != TLS_OUT_OF_INDEXES) {
        TlsFree(index);
      }
    }

    /** Set value to thread local storage. */
    return TlsSetValue(tls_index_, value) != 0;
  }

 private:
  static volatile LONG tls_index_;
};

template<typename T>
volatile LONG ThreadLocalSingletonHolder<T>::tls_index_ = TLS_OUT_OF_INDEXES;

} // namespace win32
} // namespace ggadget

#endif // GGADGET_WIN32_THREAD_LOCAL_SINGLETON_HOLDER_H__
