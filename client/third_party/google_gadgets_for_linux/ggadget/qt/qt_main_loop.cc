/*
  Copyright 2008 Google Inc.

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
#include <cmath>
#include <cstdlib>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <time.h>
#include <stdint.h>
#include <map>
#include <QtCore/QThread>
#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtGui/QApplication>
#include <ggadget/common.h>
#include <ggadget/light_map.h>
#include <ggadget/logger.h>
#include "qt_main_loop.h"

namespace ggadget {
namespace qt {

#include "qt_main_loop.moc"

// Used for transfer Timeout information between theads
class TimeoutPipeEvent {
 public:
  int watch_id;
  int interval;
  WatchCallbackInterface *callback;
};

class QtMainLoop::Impl : public WatchCallbackInterface {
 public:
  Impl(QtMainLoop *main_loop)
    : main_loop_(main_loop), main_thread_(pthread_self()) {
    pipe_fd_[0] = pipe_fd_[1] = -1;
    if (pipe(pipe_fd_) == 0) {
      fcntl(pipe_fd_[0], F_SETFL, O_NONBLOCK);
      AddIOWatch(IO_READ_WATCH, pipe_fd_[0], this);
    } else {
      LOGE("Failed to create pipe for QtMainLoop.");
    }
  }

  virtual ~Impl() {
    FreeUnusedWatches();
    LightMap<int, WatchNode *>::iterator iter;
    for (iter = watches_.begin();
         iter != watches_.end();
         iter++) {
      delete iter->second;
    }
    watches_.clear();
    if (pipe_fd_[0] >= 0)
      close(pipe_fd_[0]);
    if (pipe_fd_[1] >= 0)
      close(pipe_fd_[1]);
  }

  // Handle thread adding timeout watches
  virtual bool Call(MainLoopInterface *main_loop, int watch_id) {
    GGL_UNUSED(main_loop);
    GGL_UNUSED(watch_id);
    union {
      char data[sizeof(TimeoutPipeEvent)];
      TimeoutPipeEvent event;
    } buf;

    while (read(pipe_fd_[0], buf.data, sizeof(buf)) == sizeof(buf)) {
      // we have to create a WatchNode in the main thread so as to the
      // timer can be triggered.
      WatchNode *node = new WatchNode(main_loop_,
                                          MainLoopInterface::TIMEOUT_WATCH,
                                          buf.event.callback);
      node->data_ = buf.event.interval;

      QTimer *timer = new QTimer();
      node->object_ = timer;
      timer->setInterval(node->data_);
      QObject::connect(timer, SIGNAL(timeout(void)),
                       node, SLOT(OnTimeout(void)));
      watches_[buf.event.watch_id] = node;
      timer->start();
    }
    return true;
  }

  virtual void OnRemove(MainLoopInterface *main_loop, int watch_id) {
    GGL_UNUSED(main_loop);
    GGL_UNUSED(watch_id);
    // do nothing
  }

  int AddIOWatch(MainLoopInterface::WatchType type, int fd,
                 WatchCallbackInterface *callback) {
    if (!IsMainThread()) return MainLoopInterface::INVALID_WATCH;

    FreeUnusedWatches();
    if (fd < 0 || !callback) return -1;

    QSocketNotifier::Type qtype;
    switch (type) {
      case MainLoopInterface::IO_READ_WATCH:
        qtype = QSocketNotifier::Read;
        break;
      case MainLoopInterface::IO_WRITE_WATCH:
        qtype = QSocketNotifier::Write;
        break;
      default:
        return -1;
    }

    QSocketNotifier *notifier = new QSocketNotifier(fd, qtype);
    WatchNode *node = new WatchNode(main_loop_, type, callback);
    node->data_ = fd;
    node->object_ = notifier;
    QObject::connect(notifier, SIGNAL(activated(int)),
                     node, SLOT(OnIOEvent(int)));
    int ret = AddWatchNode(node);
    return ret;
  }

  int AddTimeoutWatch(int interval, WatchCallbackInterface *callback) {
    if (interval < 0 || !callback) return -1;

    if (!IsMainThread()) {
      if (pipe_fd_[1] >= 0) {
        int watch_id = AddWatchNode(NULL);
        TimeoutPipeEvent e;
        e.interval = interval;
        e.watch_id = watch_id;
        e.callback = callback;
        if (write(pipe_fd_[1], &e, sizeof(e)) != sizeof(e)) {
          // FIXME: the empty watch node shall be removed.
          LOGE("Failed to add timeout watch.");
          return -1;
        }
        return watch_id;
      }
      LOGE("Can't add timeout watch from another thread without pipe.");
      return -1;
    }

    WatchNode *node = new WatchNode(main_loop_,
                                    MainLoopInterface::TIMEOUT_WATCH,
                                    callback);
    node->data_ = interval;
    int watch_id = AddWatchNode(node);

    FreeUnusedWatches();
    QTimer *timer = new QTimer();
    node->object_ = timer;
    timer->setInterval(interval);
    QObject::connect(timer, SIGNAL(timeout(void)),
                     node, SLOT(OnTimeout(void)));
    timer->start();
    return watch_id;
  }

  MainLoopInterface::WatchType GetWatchType(int watch_id) {
    QMutexLocker lock(&mutex_);
    WatchNode *node = GetWatchNode(watch_id);
    return node ? node->type_ : MainLoopInterface::INVALID_WATCH;
  }

  int GetWatchData(int watch_id) {
    QMutexLocker lock(&mutex_);
    WatchNode *node = GetWatchNode(watch_id);
    return node ? node->data_ : -1;
  }

  void RemoveWatch(int watch_id) {
    ASSERT (IsMainThread());
    FreeUnusedWatches();
    WatchNode *node = GetWatchNode(watch_id);
    if (node && !node->removing_) {
      node->removing_ = true;
      if (!node->calling_) {
        WatchCallbackInterface *callback = node->callback_;
        callback->OnRemove(main_loop_, watch_id);
        watches_.erase(watch_id);
        delete node;
      }
    }
  }

  bool IsMainThread() {
    return pthread_equal(pthread_self(), main_thread_) != 0;
  }

  std::list<WatchNode *> unused_watches_;

 private:
  WatchNode* GetWatchNode(int watch_id) {
    if (watches_.find(watch_id) == watches_.end())
      return NULL;
    else
      return watches_[watch_id];
  }

  int AddWatchNode(WatchNode *node) {
    int i;
    QMutexLocker locker(&mutex_);
    while (1) {
      i = rand();
      if (watches_.find(i) == watches_.end()) break;
    }
    if (node) node->watch_id_ = i;
    watches_[i] = node;
    return i;
  }

  void FreeUnusedWatches() {
    std::list<WatchNode *>::iterator iter;
    for (iter = unused_watches_.begin();
         iter != unused_watches_.end();
         iter++) {
      watches_.erase((*iter)->watch_id_);
      delete (*iter);
    }
    unused_watches_.clear();
  }

  LightMap<int, WatchNode*> watches_;
  QtMainLoop *main_loop_;
  pthread_t main_thread_;
  int pipe_fd_[2];
  QMutex mutex_;
};

QtMainLoop::QtMainLoop()
  : impl_(new Impl(this)) {
}

QtMainLoop::~QtMainLoop() {
  delete impl_;
}

// Don't support called from threads other than main
int QtMainLoop::AddIOReadWatch(int fd, WatchCallbackInterface *callback) {
  return impl_->AddIOWatch(IO_READ_WATCH, fd, callback);
}

// Don't support called from threads other than main
int QtMainLoop::AddIOWriteWatch(int fd, WatchCallbackInterface *callback) {
  return impl_->AddIOWatch(IO_WRITE_WATCH, fd, callback);
}

// Support called from threads other than main
int QtMainLoop::AddTimeoutWatch(int interval,
                                WatchCallbackInterface *callback) {
  return impl_->AddTimeoutWatch(interval, callback);
}

// Support called from threads other than main
MainLoopInterface::WatchType QtMainLoop::GetWatchType(int watch_id) {
  return impl_->GetWatchType(watch_id);
}

// Support called from threads other than main
int QtMainLoop::GetWatchData(int watch_id) {
  return impl_->GetWatchData(watch_id);
}

// Don't support called from threads other than main. Maybe added if needed
void QtMainLoop::RemoveWatch(int watch_id) {
  impl_->RemoveWatch(watch_id);
}

void QtMainLoop::Run() {
  DLOG("MainLoop started");
  QApplication::exec();
}

bool QtMainLoop::DoIteration(bool may_block) {
  GGL_UNUSED(may_block);
  return true;
}

void QtMainLoop::Quit() {
  DLOG("MainLoop quit");
  QApplication::exit();
}

bool QtMainLoop::IsRunning() const {
  return true;
}

uint64_t QtMainLoop::GetCurrentTime() const {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return static_cast<uint64_t>(tv.tv_sec) * 1000 + tv.tv_usec / 1000;
}

bool QtMainLoop::IsMainThread() const {
  return impl_->IsMainThread();
}

void QtMainLoop::WakeUp() {
  // FIXME
}

void QtMainLoop::MarkUnusedWatchNode(WatchNode *node) {
  impl_->unused_watches_.push_back(node);
}

WatchNode::WatchNode(QtMainLoop *main_loop, MainLoopInterface::WatchType type,
                     WatchCallbackInterface *callback)
  : type_(type),
    calling_(false),
    removing_(false),
    main_loop_(main_loop),
    callback_(callback),
    object_(NULL),
    watch_id_(-1) {
}

WatchNode::~WatchNode() {
    if (object_) delete object_;
}

void WatchNode::OnTimeout() {
  if (!calling_ && !removing_) {
    calling_ = true;
    bool ret = callback_->Call(main_loop_, watch_id_);
    calling_ = false;
    if (!ret) {
      callback_->OnRemove(main_loop_, watch_id_);
    }
    if (!ret || removing_) {
      QTimer* timer = reinterpret_cast<QTimer *>(object_);
      timer->stop();
      main_loop_->MarkUnusedWatchNode(this);
    }
  }
}

void WatchNode::OnIOEvent(int fd) {
  GGL_UNUSED(fd);
  if (!calling_ && !removing_) {
    calling_ = true;
    bool ret = callback_->Call(main_loop_, watch_id_);
    calling_ = false;
    if (!ret) {
      callback_->OnRemove(main_loop_, watch_id_);
    }
    if (!ret || removing_) {
      QSocketNotifier *notifier =
          reinterpret_cast<QSocketNotifier*>(object_);
      notifier->setEnabled(false);
      main_loop_->MarkUnusedWatchNode(this);
    }
  }
}

} // namespace qt
} // namespace ggadget
