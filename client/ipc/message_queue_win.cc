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

#include "ipc/message_queue_win.h"

#include <atlbase.h>
#include <tchar.h>
#include <windows.h>

#include "base/scoped_ptr.h"
#include "base/singleton.h"
#include "base/stringprintf.h"
#include "base/time.h"

namespace {
// Win32 message type used by message queue.
const int WM_GOOPY_IPC_MSG = WM_USER;

// WaitingType is used by DoMessage that indicates if and how DoMessage should
// wait incoming messages.
enum WaitingType {
  NO_WAIT = 0,
  WAIT_LIMITED,
  WAIT_FOREVER,
};

}  // namespace

namespace ipc {

class MessageQueueWin::IPCWindowHelper {
 public:
  IPCWindowHelper() {
    // Register ipc window class.
    WNDCLASSEXW wnd_class;
    memset(&wnd_class, 0, sizeof(wnd_class));

    wnd_class.cbSize = sizeof(wnd_class);
    wnd_class.style = CS_IME;
    wnd_class.lpfnWndProc = IPCWindowProc;
    wnd_class.hInstance = _AtlBaseModule.m_hInst;
    wnd_class.hIcon = NULL;
    wnd_class.hCursor = NULL;
    wnd_class.hbrBackground = NULL;
    wnd_class.lpszMenuName = NULL;
    wnd_class.lpszClassName = kWndClassName;
    wnd_class.hIconSm = NULL;

    if (!RegisterClassExW(&wnd_class)) {
      DLOG(INFO) << "Can't register class with name"
                  << kWndClassName
                  << " error_code = "
                  << GetLastError();
    }
  }

  ~IPCWindowHelper() {
    // Don't call UnregistrerClass here because the class should survival until
    // process quit instead of dll quit.
  }

  // Support singleton mode.
  static IPCWindowHelper* GetInstance() {
    return Singleton<IPCWindowHelper>::get();
  }

  // Create a window based on class name and procedure.
  HWND CreateIPCWindow(MessageQueueWin* mq) {
    wchar_t window_name[MAX_PATH];
    memset(window_name, 0, sizeof(window_name));
    if (swprintf(window_name,
                 MAX_PATH,
                 L"IPCWindow%d",
                 GetCurrentThreadId()) == -1) {
      DLOG(ERROR) << "Format window_name failed";
      return NULL;
    }
    HWND hwnd = CreateWindowExW(0,
                                kWndClassName,
                                window_name,
                                WS_OVERLAPPED|WS_DISABLED,
                                0,
                                0,
                                0,
                                0,
                                HWND_MESSAGE,
                                NULL,
                                NULL,
                                mq);
    if (!hwnd) {
      DLOG(ERROR) << "Create window failed error_code = "
                  << GetLastError();
    }
    return hwnd;
  }

  static LRESULT CALLBACK IPCWindowProc(HWND hwnd,
                                        UINT msg,
                                        WPARAM wparam,
                                        LPARAM lparam) {
    switch (msg) {
      case WM_CREATE: {
        CREATESTRUCT* create_struct =
            reinterpret_cast<CREATESTRUCT*>(lparam);
        DCHECK(create_struct);

        MessageQueueWin* mq = reinterpret_cast<MessageQueueWin*>(
            create_struct->lpCreateParams);
        DCHECK(mq);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(mq));
        return 0;
      }
      case WM_GOOPY_IPC_MSG: {
        MessageQueueWin* mq = reinterpret_cast<MessageQueueWin*>(
            GetWindowLongPtr(hwnd, GWLP_USERDATA));
        // Handle Goopy IPC Message.
        if (wparam != NULL) {
          proto::Message* msg = reinterpret_cast<proto::Message*>(wparam);
          void* user_data = reinterpret_cast<void*>(lparam);
          mq->handler_->HandleMessage(msg, user_data);
        } else if (mq->do_message_nonexclusive_level_) {
          // If mq->do_message_nonexclusive_level_ != 0, then the
          // WM_GOOPY_IPC_MSG must be dispatched from an UI message loop (f.g.
          // Dialog::DoModal) on top of the DoMessageNonexclusive calling stack.
          // So we need to PostQuitMessage the make the message loops quit.
          PostQuitMessage(0);
        }
        return 0;
      }
      case WM_TIMER:
        // Typically timer messages don't approach here because they're either
        // handled by DoMessage in upper layer of stack or terminated by
        // |KillTimer| if it's not in |DoMessage| context,
        // but there still may be timer messages posted before |KillTimer|
        // is called achieved.
        break;
      case WM_DESTROY: {
          // DestroyWindow is called, need to destroy all pending message here.
          MSG msg;
          while (PeekMessage(&msg, hwnd, 0, 0, PM_REMOVE) != 0) {
            if (msg.message == WM_GOOPY_IPC_MSG) {
              // Remove pending message.
              if (msg.wParam)
                delete reinterpret_cast<proto::Message*>(msg.wParam);
            }
          }
          break;
      }
      default:
        break;
    }
    return DefWindowProc(hwnd, msg, wparam, lparam);
  }

 private:
  static LPCWSTR kWndClassName;
  DISALLOW_COPY_AND_ASSIGN(IPCWindowHelper);
};

LPCWSTR MessageQueueWin::IPCWindowHelper::kWndClassName =
    L"GoopyIPCWindowClass";

// TODO(haicsun): check if there are priviledge issues using this message
// type posting to a high privilege process/thread, maybe use WM_NULL
// instead as a solution.
// Define a windows message type to use for goopy ipc,
// WM_GOOPY_IPC_MSG is set to a magic number to make sure
// it's not conflict with application message type.

MessageQueueWin::MessageQueueWin(Handler* handler)
    : thread_id_(GetCurrentThreadId()),
      hwnd_(NULL),
      is_quiting_(false),
      handler_(handler),
      do_message_exclusive_level_(0),
      do_message_nonexclusive_level_(0) {
  DCHECK(handler_);
}

MessageQueueWin::~MessageQueueWin() {
  // Message queue will be destroyed in the thread creates it unless the thread
  // is abnormally quit, in which case the ownership migrates to main thread,
  // so main thread has no need to destroy resources related to creating thread
  // like |hwnd_|, since it will cause no memory leak.
  // An common case of creating thread being killed is that when application
  // receives |WM_QUIT| message and return from main method, CRT will call
  // 'ExitProcess', during which all other threads than main thread are killed.
  if (thread_id_ == GetCurrentThreadId() && hwnd_ && IsWindow(hwnd_)) {
    Quit();
    DestroyWindow(hwnd_);
  }
}

bool MessageQueueWin::Init() {
  base::AutoLock lock(lock_);

  DCHECK(!hwnd_);
  DCHECK_EQ(GetCurrentThreadId(), thread_id_);

  hwnd_ = IPCWindowHelper::GetInstance()->CreateIPCWindow(this);
  return hwnd_ != NULL;
}

bool MessageQueueWin::Post(proto::Message* message, void* user_data) {
  base::AutoLock lock(lock_);
  scoped_ptr<proto::Message> mptr(message);

  if (!hwnd_ || !IsWindow(hwnd_) || is_quiting_)
    return false;
  return PostMessage(hwnd_,
                     WM_GOOPY_IPC_MSG,
                     reinterpret_cast<WPARAM>(mptr.release()),
                     reinterpret_cast<LPARAM>(user_data)) != 0;
}

bool MessageQueueWin::DoMessageInternal(bool exclusive, int* timeout) {
  DCHECK(hwnd_ && IsWindow(hwnd_));
  DCHECK(handler_);
  DCHECK_EQ(thread_id_, GetCurrentThreadId());

  const base::TimeTicks start_time = base::TimeTicks::Now();
  WaitingType waiting_type;

  // Parse waiting type from timeout
  if (timeout) {
    if (*timeout > 0)
      waiting_type = WAIT_LIMITED;
    else if (*timeout == 0)
      waiting_type = NO_WAIT;
    else
      waiting_type = WAIT_FOREVER;
  } else {
    waiting_type = WAIT_FOREVER;
  }

  if (waiting_type == WAIT_LIMITED) {
    // Even DoMessage could be called recursively, only one timer
    // is needed because timer in upper level will be killed before
    // lower level is called.
    SetTimer(hwnd_, 1, *timeout, NULL);
  }

  MSG msg;
  bool ret = false;
  while (true) {
    // Receive a message from message loop.
    // break if
    // a. don't block but no available message.
    // b. wait but error occurs.
    if (waiting_type == NO_WAIT &&
        PeekMessage(&msg, exclusive ? hwnd_ : NULL, 0, 0, PM_REMOVE) == 0) {
      // No message available.
      break;
    } else if (GetMessage(&msg, exclusive ? hwnd_ : NULL, 0, 0) == -1) {
      // error occured.
      break;
    }

    const base::TimeTicks current_time = base::TimeTicks::Now();
    if (msg.message == WM_QUIT) {
      // It needs to be reposted so that outer message loop could quit the
      // the main message loop.
      PostQuitMessage(static_cast<int>(msg.wParam));
      break;
    } else if (msg.hwnd == hwnd_) {
      if (msg.message == WM_GOOPY_IPC_MSG) {
        // Stop timer as a message is received.
        if (waiting_type == WAIT_LIMITED)
          KillTimer(hwnd_, 1);
        // Two case should be treated seperately.
        // 1. NULL IPC message means Quit() is called,
        // need to quit the message call immediately.
        // 2. non-null message should be dispatched to
        // IPCWIndowProc to handle.
        if (msg.wParam == NULL) {
          // NULL IPC Message is special message that implies Quit()
          // is called, so DoMessage should gracefully quit with |ret|
          // set to be false.
          ret = false;
          // Repost the message to make sure upper layer(if any) of
          // recursive DoMessage call doesn't block.
          // No Lock is needed here because after Quit() is
          // called, Post() is not allowed to Post any messages
          // to this message queue, so NULL Message is still the
          // last IPC message.
          PostMessage(msg.hwnd, WM_GOOPY_IPC_MSG, 0, 0);
        } else {
          // Let registered WindowProc handle IPC message.
          DispatchMessage(&msg);
          // Set |timeout| to remaining time waiting for the message.
          if (waiting_type == WAIT_LIMITED) {
            *timeout = *timeout -
                static_cast<int>((current_time - start_time).InMilliseconds());
            if (*timeout < 0)
              *timeout = 0;
          }
          ret = true;
        }
        break;
      } else if (msg.message == WM_TIMER) {
        // Handle Timeout Message, break while loop only if time out occurs.
        // We need to check if the timeout is expired in case previous called
        // of DoMessage queue a WM_TIMER message but not processed.
        base::TimeTicks expected_time =
            start_time + base::TimeDelta::FromMilliseconds(
            static_cast<int64>(*timeout));
        if (waiting_type == WAIT_LIMITED && expected_time < current_time) {
          // Need to return if no message is received within given time.
          ret = false;
          *timeout = 0;
          KillTimer(hwnd_, 1);
          break;
        }
      }
    } else {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }
  return ret;
}

bool MessageQueueWin::DoMessage(int* timeout) {
  ++do_message_exclusive_level_;
  bool success = DoMessageInternal(true, timeout);
  --do_message_exclusive_level_;
  return success;
}

bool MessageQueueWin::DoMessageNonexclusive(int* timeout) {
  // Do not allow DoMessageNonexclusive be called from the stack of DoMessage.
  DCHECK(!do_message_exclusive_level_);
  ++do_message_nonexclusive_level_;
  bool success = DoMessageInternal(false, timeout);
  --do_message_nonexclusive_level_;
  return success;
}

void MessageQueueWin::Quit() {
  base::AutoLock lock(lock_);
  is_quiting_ = true;
  // Post a NULL IPC Message implies a quit.
  PostMessage(hwnd_, WM_GOOPY_IPC_MSG, 0, 0);
}

boolean MessageQueueWin::InCurrentThread() {
  return thread_id_ == ::GetCurrentThreadId();
}

// Make MessageQueueWin Object as default MessageQueue to create.
MessageQueue* MessageQueue::Create(Handler* handler) {
  scoped_ptr<MessageQueueWin> mq(new MessageQueueWin(handler));

  if (!mq->Init())
    return NULL;

  return mq.release();
}

}  // namespace ipc
