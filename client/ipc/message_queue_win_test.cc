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

#include "base/logging.h"
#include "base/synchronization/waitable_event.h"
#include "base/scoped_ptr.h"
#include "base/threading/platform_thread.h"
#include "base/time.h"
#include "ipc/testing.h"
#include "ipc/thread_message_queue_runner.h"

namespace {

using ipc::ThreadMessageQueueRunner;
using ipc::MessageQueue;
using ipc::MessageQueueWin;
using ipc::proto::Message;

class ThreadMQRunnerDelegateBase : public ThreadMessageQueueRunner::Delegate,
                                   public MessageQueue::Handler {
 public:
  ThreadMQRunnerDelegateBase() {}

  virtual ~ThreadMQRunnerDelegateBase() {
  }

  virtual MessageQueue* CreateMessageQueue() OVERRIDE {
    message_queue_.reset(MessageQueue::Create(this));
    return message_queue_.get();
  }

  virtual void DestroyMessageQueue(MessageQueue* queue) OVERRIDE {
    DCHECK_EQ(message_queue_.get(), queue);
    message_queue_.reset(NULL);
  }

  MessageQueue* message_queue() { return message_queue_.get(); }
 private:
  scoped_ptr<MessageQueue> message_queue_;
  DISALLOW_COPY_AND_ASSIGN(ThreadMQRunnerDelegateBase);
};

// RecursiveCallRunnerDelegate is used for recursive DoMessage call test.
class RecursiveCallRunnerDelegate : public ThreadMQRunnerDelegateBase {
 public:
  RecursiveCallRunnerDelegate()
      : recursive_finished_event_(false, false),
        num_recursive_layers_(0) {
  }

  virtual void HandleMessage(Message* message, void* user_data) OVERRIDE {
    // Message is supposed to be deleted.
    scoped_ptr<Message> mptr(message);
    ++num_recursive_layers_;
    if (num_recursive_layers_ < 10) {
      // Start recusive call
      message_queue()->DoMessage(NULL);
    } else {
      // Finish the call.
      recursive_finished_event_.Signal();
    }
    --num_recursive_layers_;
  }

  bool WaitForRecursiveFinish() {
    return recursive_finished_event_.Wait();
  }

 private:
  int num_recursive_layers_;
  // Used to simulate a recursive call.
  base::WaitableEvent recursive_finished_event_;

  DISALLOW_COPY_AND_ASSIGN(RecursiveCallRunnerDelegate);
};

// This class is used for quiting recursive call of |DoMessage| test.
class QuitRecursiveCallDelegate : public ThreadMQRunnerDelegateBase {
 public:
  QuitRecursiveCallDelegate()
      : need_to_quit_event_(false, false),
        quit_called_event_(false, false),
        quit_all_recursive_call_event_(false, false),
      num_recursive_layers_(0) {}

  virtual void HandleMessage(Message* message, void* user_data) OVERRIDE {
    scoped_ptr<Message> mptr(message);
    ++num_recursive_layers_;
    EXPECT_LT(num_recursive_layers_, 6);

    if (num_recursive_layers_ != 5) {
      // Start second recusive call.
      EXPECT_TRUE(message_queue()->DoMessage(NULL));
    } else {
      // Inform main thread to call |Quit| to interrupt
      // the recursive call.
      need_to_quit_event_.Signal();
      // Wait for Quit is called.
      EXPECT_TRUE(quit_called_event_.Wait());
      EXPECT_FALSE(message_queue()->Post(mptr.release(), NULL));
      EXPECT_FALSE(message_queue()->DoMessage(NULL));
    }

    --num_recursive_layers_;

    if (num_recursive_layers_ == 0)
      quit_all_recursive_call_event_.Signal();
  }

  bool WaitQuitAllRecursiveCall() {
    return quit_all_recursive_call_event_.Wait();
  }

  bool WaitForNeedInterrupt() {
    return need_to_quit_event_.Wait();
  }

  void SignalQuitCalled() {
    quit_called_event_.Signal();
  }

 private:
  int num_recursive_layers_;
  // Used to simulate a Quit() call that interrupts the recusive call.
  base::WaitableEvent need_to_quit_event_;
  base::WaitableEvent quit_called_event_;
  base::WaitableEvent quit_all_recursive_call_event_;

  DISALLOW_COPY_AND_ASSIGN(QuitRecursiveCallDelegate);
};

// This class is used for |DoMessage| timeout test.
class TimeoutTestDelegate : public ThreadMQRunnerDelegateBase {
 public:
  TimeoutTestDelegate() : do_message_time_out_event_(false, false),
                          timeout_(-1) {
  }

  virtual void HandleMessage(Message* message, void* user_data) OVERRIDE {
    delete message;
    int timeout = 50;
    if (!message_queue()->DoMessage(&timeout)) {
      timeout_ = timeout;
      do_message_time_out_event_.Signal();
    }
  }

  bool WaitForTimeout(int* timeout) {
    if (do_message_time_out_event_.Wait()) {
      *timeout = timeout_;
      return true;
    }
    return false;
  }

 private:
  volatile int timeout_;
  base::WaitableEvent do_message_time_out_event_;
  DISALLOW_COPY_AND_ASSIGN(TimeoutTestDelegate);
};

// A class to create a window for test.
class WindowHelper {
 public:
  class MessageHandler {
   public:
    virtual ~MessageHandler() {}
    virtual void HandleMessage(HWND hwnd,
                               UINT msg,
                               WPARAM wparam,
                               LPARAM lparam) = 0;
  };

  static HWND Create(MessageHandler* handler) {
    WNDCLASSEXA wnd_class;
    if (!GetClassInfoExA(GetModuleHandle(NULL), "mqtestclass", &wnd_class)) {
      memset(&wnd_class, 0, sizeof(wnd_class));

      wnd_class.cbSize = sizeof(wnd_class);
      wnd_class.style = CS_HREDRAW | CS_VREDRAW;
      wnd_class.lpfnWndProc = MessageQueueTestWindowProc;
      wnd_class.hInstance = GetModuleHandle(NULL);
      wnd_class.hIcon = NULL;
      wnd_class.hCursor = NULL;
      wnd_class.hbrBackground = NULL;
      wnd_class.lpszMenuName = NULL;
      wnd_class.lpszClassName = "mqtestclass";
      wnd_class.hIconSm = NULL;

      ATOM atom = RegisterClassExA(&wnd_class);
      EXPECT_NE(0, atom);
    }

    HWND hwnd = CreateWindowExA(0,
                                "mqtestclass",
                                "mqtestwindow",
                                WS_OVERLAPPED|WS_DISABLED,
                                0,
                                0,
                                0,
                                0,
                                NULL,
                                NULL,
                                NULL,
                                handler);
    return hwnd;
  }

 private:
  static LRESULT CALLBACK MessageQueueTestWindowProc(HWND hwnd,
                                                     UINT msg,
                                                     WPARAM wparam,
                                                     LPARAM lparam) {
      switch (msg) {
        case WM_CREATE: {
          CREATESTRUCT* create_struct =
              reinterpret_cast<CREATESTRUCT*>(lparam);
          DCHECK(create_struct);

          MessageHandler* handler = reinterpret_cast<MessageHandler*>(
              create_struct->lpCreateParams);
          DCHECK(handler);
          SetWindowLongPtr(hwnd,
                           GWLP_USERDATA,
                           reinterpret_cast<LONG_PTR>(handler));
          break;
        }
        default: {
          if (msg >= WM_USER) {
            MessageHandler* handler = reinterpret_cast<MessageHandler*>(
                GetWindowLongPtr(hwnd, GWLP_USERDATA));
            handler->HandleMessage(hwnd, msg, wparam, lparam);
            return 0;
          }
          break;
      }
    }
    return DefWindowProc(hwnd, msg, wparam, lparam);
  }
};

// This class tests MessageQueue behavior within a real win32 application.
class WindowCompatibilityTestDelegate : public base::PlatformThread::Delegate,
                                        public MessageQueue::Handler,
                                        public WindowHelper::MessageHandler {
 public:
  // Used defined Win32 message type used in compatibility test.
  // Refer to ThreadMain for more details.
  enum {
    // WM_START_DO_MESSAGE is sent to raise a "DoMessage" blocking call
    // in application window's message handling context for the purpose of
    // test.
    WM_START_DO_MESSAGE = WM_USER,
    // WM_DELAYED_MESSAGE is sent when application's message handling procedure
    // is blocked "Doing message" for the purpose of test.
    WM_DELAYED_MESSAGE = WM_USER + 1,
  };

  WindowCompatibilityTestDelegate() : app_hwnd_(NULL),
                                      mq_message_received_event_(false, false),
                                      ipc_wnd_msg_recv_event_(false, false),
                                      delayed_message_received_event_(
                                          false, false) {}

  virtual ~WindowCompatibilityTestDelegate() {
    if (app_hwnd_)
      DestroyWindow(app_hwnd_);
  }

  bool Init() {
    if (CreateAppWindow() == NULL)
      return false;

    mq_.reset(new MessageQueueWin(this));
    if (!mq_.get())
      return false;

    if (!mq_->Init())
      return false;

    return true;
  }

  // Method of interface Handler.
  void HandleMessage(Message* message, void* user_data) OVERRIDE {
    DCHECK(message);

    if (message->type() == 1)
      ipc_wnd_msg_recv_event_.Signal();
    else if (message->type() == 2)
      mq_message_received_event_.Signal();
    delete message;
  }

  // Called by app window procedure to notify of delayed message received event.
  void OnDelayedMessageReceived() {
    delayed_message_received_event_.Signal();
  }

  HWND CreateAppWindow() {
    app_hwnd_ = WindowHelper::Create(this);
    return app_hwnd_;
  }

  MessageQueueWin* message_queue() { return mq_.get(); }

  HWND app_hwnd() { return app_hwnd_; }

  virtual void HandleMessage(HWND hwnd,
                             UINT msg,
                             WPARAM wparam,
                             LPARAM lparam) {
    switch (msg) {
      case WM_START_DO_MESSAGE: {
        // Start DoMessage.
        mq_->DoMessage(NULL);
        break;
      }
      case WM_DELAYED_MESSAGE: {
        // Delayed message.
        OnDelayedMessageReceived();
        break;
      }
      default: {
        ASSERT_TRUE(false);
      }
    }
  }

 private:

  void ThreadMain() {
    // Before message queue started, make sure application window works.
    EXPECT_EQ(TRUE, IsWindow(app_hwnd_));

    // Before compatibility test, application window is expected to work well.
    // Which means a message sent to message queue is expected to be received
    // and handled by "HandleMessage".
    //
    // Steps to test:
    // 1) Send a message to message queue.
    // 2) if received, "HandleMessage" will signal a waitable event.
    // 3) "WaitIPCProcedureMessageReceived" waiting for the event will be woke
    // up.
    PostMessageToIPCWindow();
    WaitIPCProcedureMessageReceived();

    // Compatibility test:
    // When the message queue shares thread with application, Only IPC messages
    // are expected to be handled by the message queue, all other application
    // messages should be blocked until message queue finishes its job.
    //
    // Steps to test:
    // 1) The message sent will cause a "DoMessage" in application window's
    // creating thread, which simulates the case that message queue share
    // application's thread.
    // 2) Post a normal window message M1 to application window.
    // 3) Post a IPC message M2 to message queue.
    // 4) M2 is received.
    // 5) M1 is received which is delayed.
    PostStartMessageToAppWindow();
    PostMessageToAppWindow();
    PostMessageToMQ();
    WaitMQMessageReceived();
    WaitDelayedMessageToAppWindow();

    // Quit main message loop.
    PostQuitMessageToAppWindow();
  }

  void PostMessageToIPCWindow() {
    Message* msg = new Message();
    msg->set_type(1);
    EXPECT_TRUE(mq_->Post(msg, NULL));
  }

  void WaitIPCProcedureMessageReceived() {
    ipc_wnd_msg_recv_event_.Wait();
  }

  void PostStartMessageToAppWindow() {
    EXPECT_NE(0, PostMessage(app_hwnd_, WM_START_DO_MESSAGE, 0, 0));
  }

  void PostMessageToAppWindow() {
    EXPECT_NE(0,
              PostMessage(app_hwnd_,
                                 WM_DELAYED_MESSAGE,
                                 0,
                                 0));
  }

  void PostMessageToMQ() {
    Message* msg = new Message();
    msg->set_type(2);
    EXPECT_TRUE(mq_->Post(msg, NULL));
  }

  void WaitMQMessageReceived() {
    EXPECT_TRUE(mq_message_received_event_.Wait());
  }

  void WaitDelayedMessageToAppWindow() {
    EXPECT_TRUE(delayed_message_received_event_.Wait());
  }

  void PostQuitMessageToAppWindow() {
    EXPECT_NE(0, PostMessage(app_hwnd_, WM_QUIT, 0, 0));
  }

  scoped_ptr<MessageQueueWin> mq_;
  HWND app_hwnd_;
  base::WaitableEvent delayed_message_received_event_;
  base::WaitableEvent mq_message_received_event_;
  base::WaitableEvent ipc_wnd_msg_recv_event_;
  DISALLOW_COPY_AND_ASSIGN(WindowCompatibilityTestDelegate);
};

// Used for testing message queue runner behavior if created thread is killed.
class ThreadKilledTestDelegate : public ThreadMQRunnerDelegateBase {
 public:
  ThreadKilledTestDelegate()
      : thread_id_(0),
        message_received_event_(true, false) {
  }
  virtual ~ThreadKilledTestDelegate() {
  }
  virtual void HandleMessage(Message* message, void* user_data) OVERRIDE {
    thread_id_ = GetCurrentThreadId();
    message_received_event_.Signal();
  }
  bool Wait() { return message_received_event_.Wait(); }
  DWORD thread_id() { return thread_id_; }
 private:
  DWORD thread_id_;
  base::WaitableEvent message_received_event_;
};

// Used for testing MessageQueue::DoMessageNonexclusive.
class NonexclusiveTestDelegate : public ThreadMQRunnerDelegateBase,
                                 public WindowHelper::MessageHandler {
 public:
  NonexclusiveTestDelegate()
      : hwnd_(NULL),
        recursive_level_(0),
        window_created_event_(false, false),
        message_received_event_(false, false),
        recursive_finished_event_(false, false) {
  }

  virtual ~NonexclusiveTestDelegate() {
  }

  virtual void HandleMessage(Message* message, void* user_data) OVERRIDE {

    if (message->type() == 1) {
      hwnd_ = WindowHelper::Create(this);
      ASSERT_TRUE(hwnd_);
      delete message;
      window_created_event_.Signal();
      return;
    }

    ++recursive_level_;
    switch (recursive_level_) {
      case 1:
      case 5: {
        // Start a UI message loop.
        MSG msg;
        while (::GetMessage(&msg, NULL, 0, 0) != -1) {
          if (msg.message == WM_QUIT) {
            PostQuitMessage(msg.wParam);
            break;
          }
          ::TranslateMessage(&msg);
          ::DispatchMessage(&msg);
        }
        break;
      }
      case 2:
      case 3:
      case 4: {
        EXPECT_TRUE(message_queue()->DoMessage(NULL));
        break;
      }
      default: {
        // Signal the main thread to call Quit.
        recursive_finished_event_.Signal();
      }
    }
    delete message;
  }

  virtual void HandleMessage(HWND hwnd,
                             UINT msg,
                             WPARAM wparam,
                             LPARAM lparam) {
    message_received_event_.Signal();
  }

  bool WaitForCreateWindow() {
    return window_created_event_.Wait();
  }

  bool WaitForMessageReceived() {
    return message_received_event_.Wait();
  }

  bool WaitForRecursiveFinish() {
    return recursive_finished_event_.Wait();
  }

  bool TryPostWindowMessage() {
    return ::PostMessage(hwnd_, WM_USER, 0, 0);
  }

 private:
  HWND hwnd_;
  int recursive_level_;
  base::WaitableEvent window_created_event_;
  base::WaitableEvent message_received_event_;
  base::WaitableEvent recursive_finished_event_;
};

// Test recursive call of |DoMessage|.
TEST(MessageQueueWinTest, RecursiveCallTest) {
  scoped_ptr<RecursiveCallRunnerDelegate> delegate(
      new RecursiveCallRunnerDelegate());
  scoped_ptr<ThreadMessageQueueRunner> runner(
      new ThreadMessageQueueRunner(delegate.get()));
  runner->Run();

  // Post recursive calls.
  for (int i = 0; i < 10; ++i)
    EXPECT_TRUE(delegate->message_queue()->Post(new Message(), NULL));

  // Wait for call over.
  EXPECT_TRUE(delegate->WaitForRecursiveFinish());

  runner->Quit();
}

// Test recursive call could call |Quit| to quit all recursive layers.
TEST(MessageQueueWinTest, QuitRecursiveTest) {
  scoped_ptr<QuitRecursiveCallDelegate> delegate(
      new QuitRecursiveCallDelegate());
  scoped_ptr<ThreadMessageQueueRunner> runner(
      new ThreadMessageQueueRunner(delegate.get()));
  runner->Run();

  // Post recursive calls.
  for (int i = 0; i < 5; ++i)
    EXPECT_TRUE(delegate->message_queue()->Post(new Message(), NULL));

  // Wait until recursive call up to layer 5.
  EXPECT_TRUE(delegate->WaitForNeedInterrupt());
  delegate->message_queue()->Quit();

  delegate->SignalQuitCalled();

  EXPECT_TRUE(delegate->WaitQuitAllRecursiveCall());

  runner->Quit();
}

// Test |DoMessage| call respects timeout.
TEST(MessageQueueWinTest, TimeOutTest) {
  scoped_ptr<TimeoutTestDelegate> delegate(new TimeoutTestDelegate());
  scoped_ptr<ThreadMessageQueueRunner> runner(
      new ThreadMessageQueueRunner(delegate.get()));
  runner->Run();

  // Initialize the first recursive call.
  EXPECT_TRUE(delegate->message_queue()->Post(new Message(), NULL));

  int timeout = -1;
  EXPECT_TRUE(delegate->WaitForTimeout(&timeout));
  EXPECT_EQ(0, timeout);

  runner->Quit();
}

// Test a windows application's window could peacefully work with message
// queue's IPC window.
TEST(MessageQueueWinTest, WindowCompatibleTest) {
  scoped_ptr<WindowCompatibilityTestDelegate> delegate(
      new WindowCompatibilityTestDelegate());

  EXPECT_TRUE(delegate->Init());
  HWND app_hwnd = delegate->app_hwnd();

  // Create a child thread to control both windows.
  base::PlatformThreadHandle thread_handle;
  EXPECT_TRUE(base::PlatformThread::Create(0, delegate.get(), &thread_handle));

  MSG msg;
  BOOL ret;
  while ((ret = GetMessage(&msg, NULL, 0, 0)) != 0) {
    EXPECT_NE(-1, ret);
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  base::PlatformThread::Join(thread_handle);
}

// Test message queue runner's thread mandatorily killed, message queue
// runner and attached message queue should gracefully quit.
TEST(MessageQueueWinTest, ThreadKilledTest) {
  scoped_ptr<ThreadKilledTestDelegate> delegate(
      new ThreadKilledTestDelegate());

  scoped_ptr<ThreadMessageQueueRunner> runner(
      new ThreadMessageQueueRunner(delegate.get()));

  runner->Run();

  // Post a message to message queue, it will block the message queue from
  // returning from |DoMessage|.
  EXPECT_TRUE(delegate->message_queue()->Post(new Message(), NULL));
  // Make sure the message is received by |DoMessage|.
  EXPECT_TRUE(delegate->Wait());
  // Kill the thread.
  DWORD thread_id = delegate->thread_id();
  HANDLE handle = OpenThread(THREAD_TERMINATE,
                             FALSE, thread_id);
  ASSERT_TRUE(handle != NULL);
  ASSERT_NE(0, TerminateThread(handle, 0));
  // Wait for thread exits.
  WaitForSingleObject(handle, INFINITE);
  CloseHandle(handle);

  // Denstruct runner & delegate.
  runner.reset(NULL);
  delegate.reset(NULL);
}

// Test mixing UI message loop and DoMessage.
TEST(MessageQueueWinTest, Nonexclusive) {
  scoped_ptr<NonexclusiveTestDelegate> delegate(
      new NonexclusiveTestDelegate());
  scoped_ptr<ThreadMessageQueueRunner> runner(
      new ThreadMessageQueueRunner(delegate.get()));
  runner->Run();
  Message* message = new Message();
  message->set_type(1);
  EXPECT_TRUE(delegate->message_queue()->Post(message, NULL));
  EXPECT_TRUE(delegate->WaitForCreateWindow());
  delegate->TryPostWindowMessage();
  EXPECT_TRUE(delegate->WaitForMessageReceived());
  // Post recursive calls.
  for (int i = 0; i < 10; ++i) {
    message = new Message();
    message->set_type(0);
    EXPECT_TRUE(delegate->message_queue()->Post(message, NULL));
  }

  // Wait for call over.
  EXPECT_TRUE(delegate->WaitForRecursiveFinish());

  runner->Quit();
}

}  // namespace
