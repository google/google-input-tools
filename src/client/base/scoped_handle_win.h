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

#ifndef GOOPY_BASE_SCOPED_HANDLE_WIN_H__
#define GOOPY_BASE_SCOPED_HANDLE_WIN_H__

#include <windows.h>
#include <commctrl.h>

#include "base/basictypes.h"
#include "base/logging.h"

// Used so we always remember to close the handle.
// The class interface matches that of ScopedStdioHandle in  addition to an
// IsValid() method since invalid handles on windows can be either NULL or
// INVALID_HANDLE_VALUE (-1).
//
// Example:
//   ScopedHandle hfile(CreateFile(...));
//   if (!hfile.Get())
//     ...process error
//   ReadFile(hfile.Get(), ...);
//
// To sqirrel the handle away somewhere else:
//   secret_handle_ = hfile.Detach();
//
// To explicitly close the handle:
//   hfile.Close();

template <class Handle, class ClosePolicy>
class GenericScopedHandle {
 public:
  GenericScopedHandle() : handle_(NULL) {
  }

  explicit GenericScopedHandle(Handle h) : handle_(NULL) {
    Reset(h);
  }

  ~GenericScopedHandle() {
    Close();
  }

  // Use this instead of comparing to INVALID_HANDLE_VALUE to pick up our NULL
  // usage for errors.
  bool IsValid() const {
    return handle_ != NULL;
  }

  void Reset(Handle new_handle) {
    Close();

    // Windows is inconsistent about invalid handles, so we always use NULL
    if (new_handle != INVALID_HANDLE_VALUE)
      handle_ = new_handle;
  }

  Handle Get() const {
    return handle_;
  }

  operator Handle() { return handle_; }

  Handle Detach() {
    // transfers ownership away from this object
    Handle h = handle_;
    handle_ = NULL;
    return h;
  }

  void Close() {
    if (handle_) {
      ClosePolicy::Do(handle_);
      handle_ = NULL;
    }
  }
 protected:
  Handle handle_;
  DISALLOW_COPY_AND_ASSIGN(GenericScopedHandle);
};

#define DEFINE_SCOPED_HANDLE(Name, Handle, CloseFunction) \
class Name##ClosePolicy { \
 public: \
  static void Do(Handle handle) { \
    CloseFunction(handle); \
  } \
}; \
typedef GenericScopedHandle<Handle, Name##ClosePolicy> Name;

// Common handles.
DEFINE_SCOPED_HANDLE(ScopedHandle, HANDLE, CloseHandle);
DEFINE_SCOPED_HANDLE(ScopedFindFileHandle, HANDLE, FindClose);
DEFINE_SCOPED_HANDLE(ScopedDCHandle, HDC, DeleteDC);
DEFINE_SCOPED_HANDLE(ScopedGlobalHandle, HGLOBAL, GlobalFree);
DEFINE_SCOPED_HANDLE(ScopedLibrary, HMODULE, FreeLibrary);
DEFINE_SCOPED_HANDLE(ScopedImageList, HIMAGELIST, ImageList_Destroy);

// Scoped GDI objects.
template <class Handle>
class ScopedGDIObjectClosePolicy {
 public:
  static void Do(Handle handle) {
    DeleteObject(handle);
  }
};
template <class Handle>
class ScopedGDIObject
    : public GenericScopedHandle<Handle,
                                 ScopedGDIObjectClosePolicy<Handle> > {
 public:
  ScopedGDIObject() {}
  explicit ScopedGDIObject(Handle h) {
    Reset(h);
  }
};

// Typedefs for some common use cases.
typedef ScopedGDIObject<HBITMAP> ScopedBitmap;
typedef ScopedGDIObject<HBRUSH> ScopedBrush;
typedef ScopedGDIObject<HFONT> ScopedFont;
typedef ScopedGDIObject<HPEN> ScopedPen;
typedef ScopedGDIObject<HRGN> ScopedRegion;

// Data locker for an HGLOBAL.
template<class T = void>
class ScopedGlobalHandleLocker {
 public:
  explicit ScopedGlobalHandleLocker(HGLOBAL glob) : glob_(glob) {
    data_ = reinterpret_cast<T*>(GlobalLock(glob_));
  }
  ~ScopedGlobalHandleLocker() {
    GlobalUnlock(glob_);
  }

  T* data() { return data_; }
  size_t size() const { return GlobalSize(glob_); }
  T* operator->() const  {
    assert(data_ != 0);
    return data_;
  }

 private:
  HGLOBAL glob_;
  T* data_;
  DISALLOW_COPY_AND_ASSIGN(ScopedGlobalHandleLocker);
};

#endif  // GOOPY_BASE_SCOPED_HANDLE_WIN_H__
