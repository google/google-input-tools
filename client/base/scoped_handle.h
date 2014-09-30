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

#ifndef GOOPY_BASE_SCOPED_HANDLE_H__
#define GOOPY_BASE_SCOPED_HANDLE_H__

#include <stdio.h>
#include "base/basictypes.h"

#if defined(WIN32) || defined(_WIN64)
#include "base/scoped_handle_win.h"
#endif

class ScopedStdioHandle {
 public:
  ScopedStdioHandle()
      : handle_(NULL) { }

  explicit ScopedStdioHandle(FILE* handle)
      : handle_(handle) { }

  ~ScopedStdioHandle() {
    Close();
  }

  void Close() {
    if (handle_) {
      fclose(handle_);
      handle_ = NULL;
    }
  }

  FILE* get() const { return handle_; }

  FILE* Detach() {
    FILE* temp = handle_;
    handle_ = NULL;
    return temp;
  }

  void Set(FILE* newhandle) {
    Close();
    handle_ = newhandle;
  }

 private:
  FILE* handle_;

  DISALLOW_COPY_AND_ASSIGN(ScopedStdioHandle);
};

#endif  // GOOPY_BASE_SCOPED_HANDLE_H__
