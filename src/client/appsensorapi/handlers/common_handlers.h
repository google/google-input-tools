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

#ifndef GOOPY_APPSENSORAPI_HANDLERS_COMMON_HANDLERS_H__
#define GOOPY_APPSENSORAPI_HANDLERS_COMMON_HANDLERS_H__

#include <windows.h>
#include "appsensorapi/handler.h"

namespace ime_goopy {
namespace handlers {

class NoCandidateWindowHandler : public ime_goopy::Handler {
 public:
  NoCandidateWindowHandler() : Handler() {}
  virtual ~NoCandidateWindowHandler() = 0 {}
  virtual LRESULT HandleMessage(HWND hwnd, UINT message,
                                WPARAM wparam, LPARAM lparam) const {
    // Because WM_IME_SETCONTEXT is not sent, so catching this message and
    // removing the SHOW_CANDIDATEWINDOW bit does not work. So we ignore all
    // notify messages regards to candiate window in our implementation.
    if (message == WM_IME_NOTIFY) {
      switch (wparam) {
        case IMN_OPENCANDIDATE:
        case IMN_CLOSECANDIDATE:
        case IMN_CHANGECANDIDATE:
          return TRUE;
        default:
          return FALSE;
      }
    }
    return FALSE;
  }
};

}  // namespace handlers
}  // namespace ime_goopy

#endif  // GOOPY_APPSENSORAPI_HANDLERS_COMMON_HANDLERS_H__
