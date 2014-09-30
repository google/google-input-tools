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

#include "common/delayed_execute.h"
#include "base/callback.h"
#include "common/mutex.h"
#include <atlbase.h>
#include <atlwin.h>
#include <atlcrack.h>
#include <map>
#include <utility>

const int kWmDelayedExecute = RegisterWindowMessage(L"WM_DELAYED_EXECUTE");

namespace ime_goopy {
class MessageWindow : public CWindowImpl<MessageWindow> {
 private:
  BEGIN_MSG_MAP_EX(MessageWindow)
    MESSAGE_HANDLER(kWmDelayedExecute, OnExecute)
  END_MSG_MAP()
  LRESULT OnExecute(UINT msg, WPARAM wparam, LPARAM lparam, BOOL &handled) {
    Closure* closure = reinterpret_cast<Closure*>(lparam);
    closure->Run();
    delete closure;
    return 0;
  }
};

static map<DWORD, MessageWindow*> msg_windows;
static Mutex msg_mutex(NULL);

void ScheduleDelayedExecute(Closure *closure) {
  DWORD thread_id = GetCurrentThreadId();
  MutexLocker locker(&msg_mutex, INFINITE);
  map<DWORD, MessageWindow*>::iterator iter = msg_windows.find(thread_id);
  MessageWindow *window = NULL;
  if (iter == msg_windows.end()) {
    window = new MessageWindow();
    window->Create(HWND_MESSAGE);
    msg_windows.insert(std::make_pair(thread_id, window));
  } else {
    window = iter->second;
  }
  window->PostMessage(kWmDelayedExecute, 0, reinterpret_cast<LPARAM>(closure));
}

}  // namespace ime_goopy
