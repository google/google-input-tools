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
#include "native_main_loop.h"

#include <limits.h>
#include <sys/types.h>
#include <time.h>
#include <fcntl.h>
#include <map>
#include <ggadget/common.h>

#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif

#if defined(OS_POSIX)
#include <sys/select.h>
#endif

namespace ggadget {

// This class implements all functionalities of class NativeMainLoop.
// By using this class, all implementation details of class NativeMainLoop can
// be hidden from outside.
class NativeMainLoop::Impl {
  struct WatchNode {
    MainLoopInterface::WatchType type;

    // Indicates if the watch is being called, thus can't be removed.
    bool calling;

    // Indicates if the watch has been scheduled to be removed.
    bool removing;

    // For IO watch, it's fd, for timeout watch, it's interval.
    int data;

    // Only for timeout watch.
    uint64_t next_time;
    WatchCallbackInterface *callback;

    WatchNode()
      : type(MainLoopInterface::INVALID_WATCH),
        calling(false),
        removing(false),
        data(-1),
        next_time(0),
        callback(NULL) {
    }
  };

#ifdef HAVE_PTHREAD
  class WakeUpWatchCallback : public WatchCallbackInterface {
   public:
    WakeUpWatchCallback(int fd) : fd_(fd) {}
    virtual ~WakeUpWatchCallback() {}
    virtual bool Call(MainLoopInterface *main_loop, int watch_id) {
      char buf[10];
      // Just read the data out, should be only one byte.
      // The Call function will only be called by main loop when something
      // has been occurred on the fd_, so read() won't be blocked. As we
      // just want to clear the incoming buffer of fd_ here, we don't
      // need to care about the return value.
      read(fd_, buf, 10);
      return true;
    }
    virtual void OnRemove(MainLoopInterface *main_loop, int watch_id) {
      delete this;
    }

   private:
    int fd_;
  };
#endif

  typedef std::map<int, WatchNode> WatchMap;

 public:
  Impl(MainLoopInterface *main_loop)
    : main_loop_(main_loop),
#ifdef HAVE_PTHREAD
      main_loop_thread_(0),
#endif
      // serial_ starts from 1, because 0 is an invalid watch id.
      serial_(1),
      depth_(0) {
#ifdef HAVE_PTHREAD
    VERIFY(pthread_mutex_init(&mutex_, NULL) == 0);
    wakeup_pipe_[0] = wakeup_pipe_[1] = -1;
    // Add watch for waking up the main loop. Only useful in multi threads
    // environment.
    // pipe() seldom fails. But if it fails because of too many open
    // files, then we can't add a wake up pipe. In this case, the main loop can
    // still work in single thread environment. So it's safe to just ignore the
    // failure of pipe().
    if (pipe(wakeup_pipe_) == 0) {
      fcntl(wakeup_pipe_[0], F_SETFL, O_NONBLOCK);
      fcntl(wakeup_pipe_[1], F_SETFL, O_NONBLOCK);
      WakeUpWatchCallback *callback = new WakeUpWatchCallback(wakeup_pipe_[0]);
      AddIOWatch(IO_READ_WATCH, wakeup_pipe_[0], callback);
    }
#endif
  }

  ~Impl() {
    RemoveAllWatches();
#ifdef HAVE_PTHREAD
    if (wakeup_pipe_[0] >= 0) close(wakeup_pipe_[0]);
    if (wakeup_pipe_[1] >= 0) close(wakeup_pipe_[1]);
    VERIFY(pthread_mutex_destroy(&mutex_) == 0);
#endif
  }

  // Add an IO read or write watch for a specified file descriptor into main
  // loop.
  // type can be IO_READ_WATCH or IO_WRITE_WATCH.
  // Due to the limitation of select() function, fd must be less than
  // FD_SETSIZE.
  // callback points to an object to be called back when the required condition
  // occurs on fd.
  // This function returns an unique id of the new watch.
  int AddIOWatch(MainLoopInterface::WatchType type, int fd,
                 WatchCallbackInterface *callback) {
    if (fd < 0 || fd >= FD_SETSIZE || !callback) return -1;
#ifdef HAVE_PTHREAD
    pthread_mutex_lock(&mutex_);
#endif
    WatchNode node;
    int watch_id = serial_;
    node.type = type;
    node.data = fd;
    node.callback = callback;
    watches_[watch_id] = node;
    IncreaseSerial();
#ifdef HAVE_PTHREAD
    WakeUpUnLocked();
    pthread_mutex_unlock(&mutex_);
#endif
    return watch_id;
  }

  // Add a timeout watch with a specified interval into the main loop.
  // interval is number of milliseconds to wait before calling callback.
  // This function returns an unique id of the new watch.
  int AddTimeoutWatch(int interval, WatchCallbackInterface *callback) {
    if (interval < 0 || !callback) return -1;
#ifdef HAVE_PTHREAD
    pthread_mutex_lock(&mutex_);
#endif
    WatchNode node;
    int watch_id = serial_;
    node.type = TIMEOUT_WATCH;
    node.data = interval;
    node.next_time = GetCurrentTime() + static_cast<uint64_t>(interval);
    node.callback = callback;
    watches_[watch_id] = node;
    IncreaseSerial();
#ifdef HAVE_PTHREAD
    WakeUpUnLocked();
    pthread_mutex_unlock(&mutex_);
#endif
    return watch_id;
  }

  // Return the type of a watch.
  // It could be IO_READ_WATCH, IO_WRITE_WATCH or TIMEOUT_WATCH.
  // If it returns INVALID_WATCH, then means there is no watch
  // associated to specified watch_id.
  MainLoopInterface::WatchType GetWatchType(int watch_id) {
#ifdef HAVE_PTHREAD
    pthread_mutex_lock(&mutex_);
#endif
    MainLoopInterface::WatchType type = INVALID_WATCH;
    WatchMap::iterator iter = watches_.find(watch_id);
    if (iter != watches_.end())
      type = iter->second.type;
#ifdef HAVE_PTHREAD
    pthread_mutex_unlock(&mutex_);
#endif
    return type;
  }

  // Return the data of a watch.
  // For IO read or write watch, it returns the file descriptor.
  // For timeout watch, it returns the interval.
  // If the watch_id is invalid, then returns -1.
  int GetWatchData(int watch_id) {
#ifdef HAVE_PTHREAD
    pthread_mutex_lock(&mutex_);
#endif
    int data = -1;
    WatchMap::iterator iter = watches_.find(watch_id);
    if (iter != watches_.end())
      data = iter->second.data;
#ifdef HAVE_PTHREAD
    pthread_mutex_unlock(&mutex_);
#endif
    return data;
  }

  // Remove a watch by a specified watch_id.
  // OnRemove() method of associated watch callback object will be called
  // before removing the watch.
  void RemoveWatch(int watch_id) {
#ifdef HAVE_PTHREAD
    pthread_mutex_lock(&mutex_);
#endif
    WatchMap::iterator iter = watches_.find(watch_id);
    if (iter != watches_.end() && !iter->second.removing) {
      iter->second.removing = true;
      // Only do real remove when it's not being called.
      // If the watch is being called, it will be removed just after calling
      // by DoIteration method.
      if (!iter->second.calling) {
        WatchCallbackInterface *callback = iter->second.callback;
#ifdef HAVE_PTHREAD
        pthread_mutex_unlock(&mutex_);
#endif
        callback->OnRemove(main_loop_, watch_id);
#ifdef HAVE_PTHREAD
        pthread_mutex_lock(&mutex_);
#endif
        // It's safe to erase the watch node here. Because the removing flag
        // has been set to true, then this callback won't be called anymore
        // in DoIteration method and it won't be removed again.
        watches_.erase(watch_id);
        // Increase serial_ here to indicate that watches_ has been changed.
        IncreaseSerial();
#ifdef HAVE_PTHREAD
        WakeUpUnLocked();
#endif
      }
    }
#ifdef HAVE_PTHREAD
    pthread_mutex_unlock(&mutex_);
#endif
  }

  // Runs a single iteration of the main loop.
  // It'll check to see if any event watches are ready to be processed.
  // If no event watches are ready and may_block is true, then waits
  // for watches to become ready, then dispatches the watches that are
  // ready. Note that even when may_block is true, it is still possible
  // to return false, since the wait may be interrupted for other reasons
  // than an event watch becoming ready.
  // if may_block is false, then returns immediately if no event watches
  // are ready.
  // Return true if one or more watch has been dispatched during this
  // iteration.
  bool DoIteration(bool may_block) {
#ifdef HAVE_PTHREAD
    pthread_mutex_lock(&mutex_);
    main_loop_thread_ = pthread_self();
#endif

    // Record some states.
    int original_depth = depth_;
    int original_serial = serial_;

    // Gather file descriptors of all io watches and the minimal timeout value.
    fd_set read_fds;
    fd_set write_fds;
    uint64_t now = GetCurrentTime();
    int timeout = (may_block ? -1 : 0);
    int max_fd = 0;
    WatchMap::iterator iter = watches_.begin();
    FD_ZERO(&read_fds);
    FD_ZERO(&write_fds);
    for (; iter != watches_.end(); ++iter) {
      if (iter->second.type == IO_READ_WATCH) {
        FD_SET(iter->second.data, &read_fds);
        if (max_fd < iter->second.data) max_fd = iter->second.data;
      } else if (iter->second.type == IO_WRITE_WATCH) {
        FD_SET(iter->second.data, &write_fds);
        if (max_fd < iter->second.data) max_fd = iter->second.data;
      } else if (iter->second.type == TIMEOUT_WATCH) {
        if (iter->second.next_time <= now)
          timeout = 0;
        else if (timeout == -1 || iter->second.next_time <
                 static_cast<uint64_t>(timeout + now))
          timeout = static_cast<int>(iter->second.next_time - now);
      }
    }

    struct timeval wait_tv;
    if (timeout >= 0) {
      wait_tv.tv_sec = timeout / 1000;
      wait_tv.tv_usec = (timeout % 1000) * 1000;
    }

#ifdef HAVE_PTHREAD
    // Unlock mutex_ before selecting, so that others can call main loop
    // methods during selecting.
    pthread_mutex_unlock(&mutex_);
#endif
    int result = select(max_fd + 1, &read_fds, &write_fds, NULL,
                        (timeout >= 0 ? &wait_tv : NULL));
    if (result < 0) return false;

#ifdef HAVE_PTHREAD
    pthread_mutex_lock(&mutex_);
#endif

    // Check and call callbacks of available events.
    now = GetCurrentTime();
    iter = watches_.begin();
    bool ret = false;
    for (; iter != watches_.end(); ++iter) {
      bool call = false;
      int watch_id = iter->first;
      MainLoopInterface::WatchType type = iter->second.type;

      if ((type == IO_READ_WATCH && FD_ISSET(iter->second.data, &read_fds)) ||
          (type == IO_WRITE_WATCH && FD_ISSET(iter->second.data, &write_fds))) {
        call = true;
      } else if (type == TIMEOUT_WATCH && now >= iter->second.next_time) {
        call = true;
        iter->second.next_time += static_cast<uint64_t>(iter->second.data);
      }

      // Don't call the watch if it's currently being called or removed,
      // to prevent it from being called recursively. Such situation is only
      // possible when the main loop is being run recursively.
      if (call && !iter->second.calling && !iter->second.removing) {
        ret = true;
        WatchCallbackInterface *callback = iter->second.callback;
        // Set calling flag to prevent the watch from being removed during the
        // call.
        iter->second.calling = true;
#ifdef HAVE_PTHREAD
        pthread_mutex_unlock(&mutex_);
#endif
        bool keep = callback->Call(main_loop_, watch_id);
#ifdef HAVE_PTHREAD
        pthread_mutex_lock(&mutex_);
#endif
        WatchMap::iterator tmp_iter = watches_.find(watch_id);
        // The watch shouldn't be removed. Otherwise something wrong must be
        // happened.
        ASSERT(tmp_iter != watches_.end());

        if (tmp_iter != watches_.end()) {
          tmp_iter->second.calling = false;
          if (!keep || tmp_iter->second.removing) {
            tmp_iter->second.removing = true;
            callback = tmp_iter->second.callback;
#ifdef HAVE_PTHREAD
            pthread_mutex_unlock(&mutex_);
#endif
            callback->OnRemove(main_loop_, watch_id);
#ifdef HAVE_PTHREAD
            pthread_mutex_lock(&mutex_);
#endif
            watches_.erase(watch_id);
            IncreaseSerial();
          }
        }

        // If Quit() has been called, or any event has been added or removed,
        // then quit current iteration directly.
        // Remained events will be handled in the next iteration.
        if (original_depth != depth_ || original_serial != serial_)
          break;
      }
    }
#ifdef HAVE_PTHREAD
        pthread_mutex_unlock(&mutex_);
#endif
    return ret;
  }

  void Run() {
#ifdef HAVE_PTHREAD
    pthread_mutex_lock(&mutex_);
#endif
    ASSERT(depth_ >= 0);

#ifdef HAVE_PTHREAD
    // If the main loop is already running in another thread,
    // then just return.
    if (depth_ > 0 && pthread_equal(pthread_self(), main_loop_thread_) == 0) {
      ASSERT_M(false, ("Main loop can't be run in more than one threads!"));
      pthread_mutex_unlock(&mutex_);
      return;
    }
    main_loop_thread_ = pthread_self();
#endif

    int exit_depth = depth_;
    depth_++;

    while (depth_ != exit_depth) {
#ifdef HAVE_PTHREAD
      pthread_mutex_unlock(&mutex_);
#endif
      DoIteration(true);
#ifdef HAVE_PTHREAD
      pthread_mutex_lock(&mutex_);
#endif
    }
#ifdef HAVE_PTHREAD
    pthread_mutex_unlock(&mutex_);
#endif
  }

  void Quit() {
#ifdef HAVE_PTHREAD
    pthread_mutex_lock(&mutex_);
#endif
    ASSERT(depth_ >= 0);
    if (depth_ > 0) {
#ifdef HAVE_PTHREAD
      WakeUpUnLocked();
#endif
      --depth_;
    }
#ifdef HAVE_PTHREAD
    pthread_mutex_unlock(&mutex_);
#endif
  }

  // check whether the main loop is running or not.
  bool IsRunning() {
    return depth_ > 0;
  }

  uint64_t GetCurrentTime() const {
    struct timeval tv;
    gettimeofday(&tv, 0);
    return static_cast<uint64_t>(tv.tv_sec)*1000 + tv.tv_usec/1000;
  }

  bool IsMainThread() const {
#ifdef HAVE_PTHREAD
    return pthread_equal(pthread_self(), main_loop_thread_) == 0;
#else
    return true;
#endif
  }

  void WakeUp() {
#ifdef HAVE_PTHREAD
    if (!IsMainThread()) {
      pthread_mutex_lock(&mutex_);
      WakeUpUnLocked();
      pthread_mutex_unlock(&mutex_);
    }
#endif
  }

 private:
#ifdef HAVE_PTHREAD
  void WakeUpUnLocked() {
    if (!IsRunning()) return;
    // Wakeup the main loop by writing something into wakeup pipe.
    // This function can only be called in another thread.
    if (wakeup_pipe_[1] >= 0 &&
        pthread_equal(pthread_self(), main_loop_thread_) == 0) {
      write(wakeup_pipe_[1], "a", 1);
    }
  }
#endif

  void RemoveAllWatches() {
#ifdef HAVE_PTHREAD
    pthread_mutex_lock(&mutex_);
#endif
    WatchMap::iterator iter = watches_.begin();
    while (iter != watches_.end()) {
      int watch_id = iter->first;
      WatchCallbackInterface *callback = iter->second.callback;
      iter->second.removing = true;
#ifdef HAVE_PTHREAD
      pthread_mutex_unlock(&mutex_);
#endif
      callback->OnRemove(main_loop_, watch_id);
#ifdef HAVE_PTHREAD
      pthread_mutex_lock(&mutex_);
#endif
      watches_.erase(watch_id);
      iter = watches_.begin();
    }
#ifdef HAVE_PTHREAD
    pthread_mutex_unlock(&mutex_);
#endif
  }

  // Increase serial_ by one, taking care of overflow and overlap issue.
  // It's almost impossible that 2 ** 31 space are all occupied, so the while
  // won't be a dead loop.
  void IncreaseSerial() {
    if (serial_ == INT_MAX) {
      // serial_ starts from 1, because 0 is an invalid watch id.
      serial_ = 1;
    } else {
      ++serial_;
    }
    while (watches_.find(serial_) != watches_.end())
      ++serial_;
  }

  MainLoopInterface *main_loop_;

#ifdef HAVE_PTHREAD
  pthread_t main_loop_thread_;
  pthread_mutex_t mutex_;
  // pipe fds for waking up main loop.
  // wakeup_pipe_[0] (for reading) will be added into main loop.
  // wakeup_pipe_[1] (for writing) will be used by WakeUp() method.
  int wakeup_pipe_[2];
#endif

  WatchMap watches_;
  int serial_;
  int depth_;
};

NativeMainLoop::NativeMainLoop()
  : impl_(new Impl(this)) {
}
NativeMainLoop::~NativeMainLoop() {
  delete impl_;
}
int NativeMainLoop::AddIOReadWatch(int fd, WatchCallbackInterface *callback) {
  return impl_->AddIOWatch(IO_READ_WATCH, fd, callback);
}
int NativeMainLoop::AddIOWriteWatch(int fd, WatchCallbackInterface *callback) {
  return impl_->AddIOWatch(IO_WRITE_WATCH, fd, callback);
}
int NativeMainLoop::AddTimeoutWatch(int interval,
                                    WatchCallbackInterface *callback) {
  return impl_->AddTimeoutWatch(interval, callback);
}
MainLoopInterface::WatchType NativeMainLoop::GetWatchType(int watch_id) {
  return impl_->GetWatchType(watch_id);
}
int NativeMainLoop::GetWatchData(int watch_id) {
  return impl_->GetWatchData(watch_id);
}
void NativeMainLoop::RemoveWatch(int watch_id) {
  impl_->RemoveWatch(watch_id);
}
void NativeMainLoop::Run() {
  impl_->Run();
}
bool NativeMainLoop::DoIteration(bool may_block) {
  return impl_->DoIteration(may_block);
}
void NativeMainLoop::Quit() {
  impl_->Quit();
}
bool NativeMainLoop::IsRunning() const {
  return impl_->IsRunning();
}
uint64_t NativeMainLoop::GetCurrentTime() const {
  return impl_->GetCurrentTime();
}
bool NativeMainLoop::IsMainThread() const {
  return impl_->IsMainThread();
}
void NativeMainLoop::WakeUp() {
  impl_->WakeUp();
}

} // namespace ggadget
