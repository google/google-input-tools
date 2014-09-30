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

#include "main_loop.h"

#pragma comment(lib, "winmm.lib")
#include <mmsystem.h>
#include <windows.h>
#include <atlbase.h>
#include <algorithm>
#include <map>
#include <vector>
#include <ggadget/common.h>
#include <ggadget/win32/thread_local_singleton_holder.h>

namespace {

enum {
  // User defined win32 message for main loop.
  // WPARAM for this message stand for:
  // 0 : Add watch.
  // 1 : Remove watch.
  // 2 : GetCurrentTime.
  // 3 : Quit.
  WM_MAIN_LOOP_CONTROL_MSG = WM_USER,
};

// WPARAM definition for WM_MAIN_LOOP_CONTROL_MSG.
enum {
  // Need a |WatchNode *node| type variable as lParam.
  MAIN_LOOP_CMD_ADD_WATCH = 0,
  // Need a |int watch_id| type variable as lParam.
  MAIN_LOOP_CMD_REMOVE_WATCH = 1,
  // Need a |uint64_t *| out type variable as lParam.
  MAIN_LOOP_CMD_GET_CURRENT_TIME = 2,
  // Need a |WatchNode *| in out type variable as lParam with watch_id set.
  MAIN_LOOP_CMD_GET_WATCH_NODE = 3,
  // lParam is not necessarily set.
  MAIN_LOOP_CMD_QUIT = 4,
};

const wchar_t kMainLoopClass[] = L"ggadget_main_loop_class";

HWND CreateMainLoopControlWindow(WNDPROC proc, void *data) {
  // Register ipc window class.
  WNDCLASSEXW wnd_class;

  HINSTANCE instance = _AtlBaseModule.m_hInst;
  if (!GetClassInfoEx(instance, kMainLoopClass, &wnd_class)) {
    memset(&wnd_class, 0, sizeof(wnd_class));

    wnd_class.cbSize = sizeof(wnd_class);
    wnd_class.style = 0;
    wnd_class.lpfnWndProc = proc;
    wnd_class.hInstance = instance;
    wnd_class.hIcon = NULL;
    wnd_class.hCursor = NULL;
    wnd_class.hbrBackground = NULL;
    wnd_class.lpszMenuName = NULL;
    wnd_class.lpszClassName = kMainLoopClass;
    wnd_class.hIconSm = NULL;

    if (!RegisterClassExW(&wnd_class))
      return NULL;
  }

  HWND hwnd = CreateWindowExW(0,
                              kMainLoopClass,
                              L"",
                              WS_OVERLAPPED|WS_DISABLED,
                              0,
                              0,
                              0,
                              0,
                              HWND_MESSAGE,
                              NULL,
                              NULL,
                              data);
  return hwnd;
}

}  // namespace

namespace ggadget {
namespace win32 {

namespace {

struct ThreadLocalTimeData {
  ThreadLocalTimeData() : last_seen_now(0), rollover_ms(0) {
  }
  // The last timeGetTime value we saw, to detect rollover.
  DWORD last_seen_now;
  // Accumulation of time lost due to rollover (in milliseconds).
  uint64_t rollover_ms;
};

// Get current time in milliseconds.
uint64_t GetRollOverNow() {
  DWORD now = timeGetTime();
  ThreadLocalTimeData *thread_time_data =
      ThreadLocalSingletonHolder<ThreadLocalTimeData>::GetValue();
  if (!thread_time_data) {
    thread_time_data = new ThreadLocalTimeData();
    bool result = ThreadLocalSingletonHolder<ThreadLocalTimeData>::SetValue(
        thread_time_data);
    ASSERT(result);
  }
  if (now < thread_time_data->last_seen_now)
    thread_time_data->rollover_ms += 0x100000000I64;
  thread_time_data->last_seen_now = now;
  return now + thread_time_data->rollover_ms;
}

}  // namespace

class MainLoop::Impl {
 public:
  struct WatchNode {
    MainLoopInterface::WatchType type;
    // Id of this watch.
    int watch_id;
    // Interval in milliseconds for timeout watch.
    int data;
    // Expected schedule time.
    uint64_t next_schedule_time;
    // Callback for this watch.
    WatchCallbackInterface *callback;
    // Indicates if the watch node is being called.
    bool is_calling;
    // Indicates if the watch node is being removed.
    bool is_removing;

    WatchNode()
        : type(MainLoopInterface::INVALID_WATCH),
          watch_id(-1),
          data(-1),
          next_schedule_time(0),
          callback(NULL),
          is_calling(false),
          is_removing(false) {
    }
    void CopyFrom(const WatchNode &node) {
      memcpy(this, &node, sizeof(WatchNode));
    }
  };

  explicit Impl(MainLoopInterface *main_loop);
  ~Impl();

  // See ggadget/main_loop_interface.h for description.
  // Not used.
  int AddIOWatch(MainLoopInterface::WatchType type, int fd,
                 WatchCallbackInterface *callback);

  int AddTimeoutWatch(int interval, WatchCallbackInterface *callback);

  MainLoopInterface::WatchType GetWatchType(int watch_id);
  int GetWatchData(int watch_id);
  void RemoveWatch(int watch_id);
  void Run();
  bool DoIteration(bool may_block);
  void Quit();
  bool IsRunning() const;
  uint64_t GetCurrentTime() const;
  bool IsMainThread() const;

  // Called by MainLoopWindowProcedure when timeout occurs.
  // Return true if at least one watch callback called.
  bool RunTimeoutWatchUnlocked();
  // Add watch node in the same thread of main loop.
  bool AddWatchNodeUnlocked(WatchNode *node);
  // Remove watch node in the same thread of main loop.
  void RemoveWatchNodeUnlocked(int watch_id);

 private:
  // Used by |Run| and |DoIteration|.
  // Return false if
  // - WM_QUIT received.
  // - Quit is called.
  // - GetMessage error.
  // watches_called (opt) is true if
  // - There are watches dispatched.
  bool DispatchWindowsMessage(bool *watches_called);
  // Return unused watch id.
  // Return 0 if failed.
  int NewWatchId();
  // Recaculate next callback to call, and set the timer.
  void ResetWatchTimerUnlocked();
  // The Control window's procedure.
  static LRESULT WINAPI MainLoopWindowProcedure(HWND hwnd,
                                                UINT msg,
                                                WPARAM wparam,
                                                LPARAM lparam);
  // Return watch data of certain watch with watch id = |watch_id|.
  // Leave |node| unchanged if watch not found.
  // - watch_id, used as key to query.
  // - node, out parameter with queried watch node information.
  void GetWatchNodeUnlocked(int watch_id, WatchNode *node);
  // Comparer used to sort the watch list.
  static bool CompareScheduledTime(WatchNode *a, WatchNode *b);

  // Pointer to owner of this class.
  MainLoopInterface *main_loop_;
  // Map from watch id to watch node.
  std::map<int, WatchNode *> id_to_watch_map_;
  // List of pointers of watch nodes sorted by next run time.
  typedef std::vector<WatchNode *>::iterator WatchNodesIterator;
  std::vector<WatchNode *> sorted_watches_list_;
  // The thread id of loop.
  DWORD main_thread_id_;
  // The message window for main loop.
  HWND hwnd_;
  // Store states of watch_id set.
  int watch_id_;
  // Indicate is the main loop is being destroyed.
  bool destroyed_;
  DISALLOW_EVIL_CONSTRUCTORS(Impl);
};

MainLoop::Impl::Impl(MainLoopInterface *main_loop)
    : main_loop_(main_loop),
      main_thread_id_(0),
      hwnd_(NULL),
      watch_id_(0),
      destroyed_(false) {
  main_thread_id_ = GetCurrentThreadId();
  hwnd_ = CreateMainLoopControlWindow(MainLoopWindowProcedure, this);
  ASSERT(hwnd_);
}

MainLoop::Impl::~Impl() {
  destroyed_ = true;
  std::map<int, WatchNode *>::iterator i = id_to_watch_map_.begin();
  for (; i != id_to_watch_map_.end(); ++i) {
    i->second->callback->OnRemove(main_loop_, i->second->watch_id);
    delete i->second;
  }
  DestroyWindow(hwnd_);
}

int MainLoop::Impl::AddIOWatch(MainLoopInterface::WatchType type, int fd,
                               WatchCallbackInterface *callback) {
  // Not used.
  return -1;
}

int MainLoop::Impl::AddTimeoutWatch(int interval,
                                    WatchCallbackInterface *callback) {
  if (interval < 0 || !callback || destroyed_)
    return -1;

  WatchNode *node = new WatchNode();
  node->type = MainLoopInterface::TIMEOUT_WATCH;
  node->data = interval;
  node->callback = callback;
  SendMessage(hwnd_, WM_MAIN_LOOP_CONTROL_MSG, MAIN_LOOP_CMD_ADD_WATCH,
              reinterpret_cast<LPARAM>(node));
  return node->watch_id;
}

MainLoopInterface::WatchType MainLoop::Impl::GetWatchType(int watch_id) {
  WatchNode node;
  memset(&node, 0, sizeof(node));
  node.watch_id = watch_id;
  node.type = MainLoopInterface::INVALID_WATCH;
  SendMessage(hwnd_, WM_MAIN_LOOP_CONTROL_MSG, MAIN_LOOP_CMD_GET_WATCH_NODE,
              reinterpret_cast<LPARAM>(&node));
  return node.type;
}

int MainLoop::Impl::GetWatchData(int watch_id) {
  WatchNode node;
  memset(&node, 0, sizeof(node));
  node.watch_id = watch_id;
  node.data = -1;
  SendMessage(hwnd_, WM_MAIN_LOOP_CONTROL_MSG, MAIN_LOOP_CMD_GET_WATCH_NODE,
              reinterpret_cast<LPARAM>(&node));
  return node.data;
}

void MainLoop::Impl::RemoveWatch(int watch_id) {
  if (!destroyed_) {
    SendMessage(hwnd_, WM_MAIN_LOOP_CONTROL_MSG, MAIN_LOOP_CMD_REMOVE_WATCH,
                watch_id);
  }
}

void MainLoop::Impl::Run() {
  if (destroyed_)
    return;
  while (DispatchWindowsMessage(NULL)) {
  }
}

bool MainLoop::Impl::DoIteration(bool may_block) {
  if (destroyed_)
    return false;

  MSG msg;
  bool has_dispatched_watches = false;
  bool ret = true;
  while (ret && !has_dispatched_watches) {
    if (!may_block && PeekMessage(&msg, hwnd_, 0, 0, PM_NOREMOVE) == 0)
      break;
    ret = DispatchWindowsMessage(&has_dispatched_watches);
  }
  return has_dispatched_watches;
}

void MainLoop::Impl::Quit() {
  PostMessage(hwnd_, WM_MAIN_LOOP_CONTROL_MSG, MAIN_LOOP_CMD_QUIT, 0);
}

bool MainLoop::Impl::IsRunning() const {
  return true;
}

uint64_t MainLoop::Impl::GetCurrentTime() const {
  uint64_t current_time = 0;
  SendMessage(hwnd_, WM_MAIN_LOOP_CONTROL_MSG,
              static_cast<WPARAM>(MAIN_LOOP_CMD_GET_CURRENT_TIME),
              reinterpret_cast<LPARAM>(&current_time));
  return current_time;
}

bool MainLoop::Impl::IsMainThread() const {
  return GetCurrentThreadId() == main_thread_id_;
}

// The function uses pointer of WatchNode to identify an watch which may cause
// bugs if an secondary call |new| for type WatchNode return a same pointer
// after it's release.
bool MainLoop::Impl::RunTimeoutWatchUnlocked() {
  if (sorted_watches_list_.empty())
    return false;
  uint64_t current_time = GetRollOverNow();
  // Move all timeout watches to temporary created list.
  // during the function.
  std::vector<WatchNode *> timeout_watches;
  while (!sorted_watches_list_.empty()) {
    WatchNode *node = sorted_watches_list_.front();
    if (node->next_schedule_time > current_time)
      break;
    timeout_watches.push_back(node);
    std::pop_heap(sorted_watches_list_.begin(), sorted_watches_list_.end(),
                  CompareScheduledTime);
    sorted_watches_list_.pop_back();
  }
  uint64_t next_schedule_time = static_cast<uint64_t>(-1);
  // Call timeout watches, timeout watch may be removed during the block,
  // so it's neccesary to validate it by querying |id_to_watch_map_| before the
  // call.
  WatchNodesIterator iter = timeout_watches.begin();
  for (; iter != timeout_watches.end(); ++iter) {
    WatchNode *node = *iter;
    if (id_to_watch_map_.count(node->watch_id)) {
      node->is_calling = true;
      node->next_schedule_time += node->data;
      bool ret = node->callback->Call(main_loop_, node->watch_id);
      // Watch node may be removed during execution time of its call function.
      if (!node->is_removing && !ret) {
        // According to MainLoopInterface, if |Call| return false, it should be
        // removed from main loop.
        RemoveWatchNodeUnlocked(node->watch_id);
      }  else if (!node->is_removing && ret &&
                  next_schedule_time > node->next_schedule_time) {
        // Record next schedule time of valid called watches.
        next_schedule_time = node->next_schedule_time;
      }
    }
  }

  // Record minimum next_schedule_time.
  if (!sorted_watches_list_.empty() &&
      next_schedule_time > sorted_watches_list_.front()->next_schedule_time) {
    next_schedule_time = sorted_watches_list_.front()->next_schedule_time;
  }
  if (next_schedule_time == static_cast<uint64_t>(-1)) {
    // No watches exist.
    KillTimer(hwnd_, 1);
    return !timeout_watches.empty();
  }
  // Reset timer since sorted_watches_list_ has been changed.
  if (next_schedule_time <= current_time) {
    // timeout happens.
    SetTimer(hwnd_, 1, USER_TIMER_MINIMUM, NULL);
  } else if (next_schedule_time - current_time > USER_TIMER_MAXIMUM) {
    SetTimer(hwnd_, 1, USER_TIMER_MAXIMUM, NULL);
  } else {
    SetTimer(hwnd_, 1, next_schedule_time - current_time, NULL);
  }
  // Push back active called watches into sorted_watches_list_.
  iter = timeout_watches.begin();
  for (; iter != timeout_watches.end(); ++iter) {
    WatchNode *node = *iter;
    if (node->is_removing) {
      delete node;
    } else {
      // the node should has been removed in upper block.
      ASSERT(id_to_watch_map_.count(node->watch_id));
      node->is_calling = false;
      sorted_watches_list_.push_back(*iter);
      std::push_heap(sorted_watches_list_.begin(), sorted_watches_list_.end(),
                     CompareScheduledTime);
    }
  }
  return !timeout_watches.empty();
}

bool MainLoop::Impl::AddWatchNodeUnlocked(WatchNode *node) {
  node->watch_id = NewWatchId();
  id_to_watch_map_[node->watch_id] = node;

  node->next_schedule_time = GetRollOverNow() + node->data;
  sorted_watches_list_.push_back(node);
  std::push_heap(sorted_watches_list_.begin(), sorted_watches_list_.end(),
                 CompareScheduledTime);
  ResetWatchTimerUnlocked();
  return true;
}

void MainLoop::Impl::RemoveWatchNodeUnlocked(int watch_id) {
  if (!id_to_watch_map_.count(watch_id))
    return;
  WatchNode *node = id_to_watch_map_[watch_id];
  // Remove it from id map.
  id_to_watch_map_.erase(watch_id);

  // Remove it from sorted list.
  WatchNodesIterator iter = sorted_watches_list_.begin();
  for (; iter != sorted_watches_list_.end(); ++iter) {
    if (*iter == node) {
      sorted_watches_list_.erase(iter);
      std::make_heap(sorted_watches_list_.begin(), sorted_watches_list_.end(),
                     CompareScheduledTime);
      break;
    }
  }
  node->callback->OnRemove(main_loop_, watch_id);
  if (!node->is_calling) {
    delete node;
  } else {
    // If watch node is removed during its call function.
    // set |is_removing| to notify |Call| the node should be deleted after call
    // finished.
    node->is_removing = true;
  }
  ResetWatchTimerUnlocked();
}

bool MainLoop::Impl::DispatchWindowsMessage(bool *watches_called) {
  if (watches_called)
    *watches_called = false;

  // Get one window message.
  MSG msg;
  BOOL ret = GetMessage(&msg, NULL, 0, 0);
  if (ret == 0) {
    // WM_QUIT message received.
    PostQuitMessage(msg.wParam);
    return false;
  } else if (ret == -1) {
    // GetMessage error.
    return false;
  }

  if (msg.hwnd == hwnd_ && msg.message == WM_MAIN_LOOP_CONTROL_MSG) {
    const int cmd = msg.wParam;
    if (cmd == MAIN_LOOP_CMD_QUIT) {
      // |Quit| is called. return false quit upper call of |Run|.
      return false;
    }
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  } else if (msg.hwnd == hwnd_ && msg.message == WM_TIMER) {
    ret = RunTimeoutWatchUnlocked();
    if (watches_called)
      *watches_called = ret;
  } else {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
  return true;
}

int MainLoop::Impl::NewWatchId() {
  const int begin = watch_id_;
  do {
    watch_id_++;
    if (watch_id_ <= 0)
      watch_id_ = 1;
    if (begin == watch_id_)
      return 0;
  } while (id_to_watch_map_.count(watch_id_));
  return watch_id_;
}

void MainLoop::Impl::ResetWatchTimerUnlocked() {
  if (sorted_watches_list_.empty()) {
    KillTimer(hwnd_, 1);
    return;
  }
  // Set timer.
  uint64_t current_time = GetRollOverNow();
  WatchNode *node = sorted_watches_list_.front();
  if (node->next_schedule_time <= current_time ||
      node->next_schedule_time - current_time < USER_TIMER_MINIMUM) {
    // Timeout happens.
    SetTimer(hwnd_, 1, USER_TIMER_MINIMUM, NULL);
  } else if (node->next_schedule_time - current_time < USER_TIMER_MAXIMUM) {
    SetTimer(hwnd_, 1, node->next_schedule_time - current_time, NULL);
  } else {
    SetTimer(hwnd_, 1, USER_TIMER_MAXIMUM, NULL);
  }
}

LRESULT WINAPI MainLoop::Impl::MainLoopWindowProcedure(
    HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
  MainLoop::Impl *impl = reinterpret_cast<MainLoop::Impl *>(
      GetWindowLongPtr(hwnd, GWLP_USERDATA));
  if (msg == WM_CREATE) {
    CREATESTRUCT* create_struct =
        reinterpret_cast<CREATESTRUCT*>(lparam);
    impl = reinterpret_cast<MainLoop::Impl *>(create_struct->lpCreateParams);
    SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(impl));
    impl->hwnd_ = hwnd;
    // Start timer if there are watches.
    impl->ResetWatchTimerUnlocked();
  } else if (msg == WM_TIMER) {
    impl->RunTimeoutWatchUnlocked();
    return 0;
  } else if (msg == WM_MAIN_LOOP_CONTROL_MSG) {
    const int cmd = wparam;
    switch (cmd) {
      case MAIN_LOOP_CMD_ADD_WATCH:
        // Add watch.
        impl->AddWatchNodeUnlocked(reinterpret_cast<WatchNode *>(lparam));
        return 0;
      case MAIN_LOOP_CMD_REMOVE_WATCH:
        // Remove watch.
        impl->RemoveWatchNodeUnlocked(static_cast<int>(lparam));
        return 0;
      case MAIN_LOOP_CMD_GET_CURRENT_TIME: {
        // GetCurrentTime.
        uint64_t *current_time_ptr = reinterpret_cast<uint64_t *>(lparam);
        *current_time_ptr = GetRollOverNow();
        return 0;
      }
      case MAIN_LOOP_CMD_GET_WATCH_NODE: {
        WatchNode *node = reinterpret_cast<WatchNode *>(lparam);
        impl->GetWatchNodeUnlocked(node->watch_id, node);
        return 0;
      }
      default:
        break;
    }
  }
  return DefWindowProc(hwnd, msg, wparam, lparam);
}

void MainLoop::Impl::GetWatchNodeUnlocked(int watch_id, WatchNode *node) {
  ASSERT(node);
  if (id_to_watch_map_.count(watch_id))
    node->CopyFrom(*id_to_watch_map_[watch_id]);
}

bool MainLoop::Impl::CompareScheduledTime(WatchNode *a, WatchNode *b) {
  return a->next_schedule_time > b->next_schedule_time;
}

MainLoop::MainLoop()
  : impl_(new Impl(this)) {
}

MainLoop::~MainLoop() {
}

int MainLoop::AddIOReadWatch(int fd, WatchCallbackInterface *callback) {
  return impl_->AddIOWatch(IO_READ_WATCH, fd, callback);
}

int MainLoop::AddIOWriteWatch(int fd, WatchCallbackInterface *callback) {
  return impl_->AddIOWatch(IO_WRITE_WATCH, fd, callback);
}

int MainLoop::AddTimeoutWatch(int interval,
                                    WatchCallbackInterface *callback) {
  return impl_->AddTimeoutWatch(interval, callback);
}

MainLoopInterface::WatchType MainLoop::GetWatchType(int watch_id) {
  return impl_->GetWatchType(watch_id);
}

int MainLoop::GetWatchData(int watch_id) {
  return impl_->GetWatchData(watch_id);
}

void MainLoop::RemoveWatch(int watch_id) {
  impl_->RemoveWatch(watch_id);
}

void MainLoop::Run() {
  impl_->Run();
}

bool MainLoop::DoIteration(bool may_block) {
  return impl_->DoIteration(may_block);
}

void MainLoop::Quit() {
  impl_->Quit();
}

bool MainLoop::IsRunning() const {
  return impl_->IsRunning();
}

uint64_t MainLoop::GetCurrentTime() const {
  return impl_->GetCurrentTime();
}

bool MainLoop::IsMainThread() const {
  return impl_->IsMainThread();
}

void MainLoop::WakeUp() {
  // no need to wake up.
}

}  // namespace win32
}  // namespace ggadget
