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

#include <fcntl.h>
#include <stdint.h>
#include <glib/ghash.h>
#include <gtk/gtk.h>
#include <glib/gthread.h>
#include <ggadget/common.h>
#include "main_loop.h"
#include "utilities.h"

namespace ggadget {
namespace gtk {

class MainLoop::Impl {
  struct WatchNode {
    MainLoopInterface::WatchType type;

    // Indicates if the watch is being called, thus can't be removed.
    bool calling;

    // Indicates if the watch has been scheduled to be removed.
    bool removing;

    int watch_id;
    int data;      // For IO watch, it's fd, for timeout watch, it's interval.
    WatchCallbackInterface *callback;
    MainLoop::Impl *impl;

    WatchNode()
      : type(MainLoopInterface::INVALID_WATCH),
        calling(false),
        removing(false),
        watch_id(-1),
        data(-1),
        callback(NULL),
        impl(NULL) {
    }
  };

 public:
  Impl(MainLoopInterface *main_loop)
    : main_loop_(main_loop), destroyed_(false),
      main_thread_(NULL) {
    // Initialize the glib thread environment, otherwise the glib main loop
    // will not be thread safe.
    if (!g_thread_supported())
      g_thread_init(NULL);

    main_thread_ = g_thread_self();
    g_static_mutex_init(&mutex_);
    watches_ = g_hash_table_new_full(g_direct_hash,
                                     g_direct_equal,
                                     NULL,
                                     NodeDestroyCallback);
    ASSERT(watches_);
  }

  ~Impl() {
    g_static_mutex_lock(&mutex_);
    destroyed_ = true;
    g_hash_table_foreach_remove(watches_, ForeachRemoveCallback, this);
    g_hash_table_destroy(watches_);
    g_static_mutex_unlock(&mutex_);
    g_static_mutex_free(&mutex_);
  }

  int AddIOWatch(MainLoopInterface::WatchType type, int fd,
                 WatchCallbackInterface *callback) {
    if (fd < 0 || !callback) return -1;
    g_static_mutex_lock(&mutex_);
    if (destroyed_) {
      g_static_mutex_unlock(&mutex_);
      return -1;
    }
    GIOCondition cond =
        (type == MainLoopInterface::IO_READ_WATCH ? G_IO_IN : G_IO_OUT);
    cond = static_cast<GIOCondition>(cond | G_IO_HUP | G_IO_ERR);
    GIOChannel *channel = g_io_channel_unix_new(fd);
    WatchNode *node = new WatchNode();
    node->type = type;
    node->data = fd;
    node->callback = callback;
    node->impl = this;
    int watch_id = g_io_add_watch(channel, cond, IOWatchCallback, node);
    node->watch_id = watch_id;
    g_hash_table_insert(watches_, GINT_TO_POINTER(node->watch_id), node);
    g_io_channel_unref(channel);
    g_static_mutex_unlock(&mutex_);
    return watch_id;
  }

  int AddTimeoutWatch(int interval, WatchCallbackInterface *callback) {
    if (interval < 0 || !callback) return -1;
    g_static_mutex_lock(&mutex_);
    if (destroyed_) {
      g_static_mutex_unlock(&mutex_);
      return -1;
    }
    WatchNode *node = new WatchNode();
    node->type = MainLoopInterface::TIMEOUT_WATCH;
    node->data = interval;
    node->callback = callback;
    node->impl = this;
    int watch_id = (interval < 1 ?
         g_idle_add(TimeoutCallback, node) :
         // Lower priority of other timers to prevent them from congesting the
         // event loop.
         g_timeout_add_full(G_PRIORITY_DEFAULT_IDLE, interval, TimeoutCallback,
                            node, NULL));
    node->watch_id = watch_id;
    g_hash_table_insert(watches_, GINT_TO_POINTER(node->watch_id), node);
    g_static_mutex_unlock(&mutex_);
    return watch_id;
  }

  MainLoopInterface::WatchType GetWatchType(int watch_id) {
    g_static_mutex_lock(&mutex_);
    WatchNode *node = static_cast<WatchNode *>(
          g_hash_table_lookup(watches_, GINT_TO_POINTER(watch_id)));
    MainLoopInterface::WatchType type =
        node ? node->type : MainLoopInterface::INVALID_WATCH;
    g_static_mutex_unlock(&mutex_);
    return type;
  }

  int GetWatchData(int watch_id) {
    g_static_mutex_lock(&mutex_);
    WatchNode *node = static_cast<WatchNode *>(
        g_hash_table_lookup(watches_, GINT_TO_POINTER(watch_id)));
    int data = node ? node->data : -1;
    g_static_mutex_unlock(&mutex_);
    return data;
  }

  void RemoveWatch(int watch_id) {
    g_static_mutex_lock(&mutex_);
    if (!destroyed_) {
      WatchNode *node = static_cast<WatchNode *>(
          g_hash_table_lookup(watches_, GINT_TO_POINTER(watch_id)));
      if (node && !node->removing) {
        node->removing = true;
        if (!node->calling) {
          g_source_remove(watch_id);
          WatchCallbackInterface *callback = node->callback;
          //DLOG("MainLoop::RemoveWatch: id=%d", watch_id);
          g_static_mutex_unlock(&mutex_);
          callback->OnRemove(main_loop_, watch_id);
          g_static_mutex_lock(&mutex_);
          g_hash_table_remove(watches_, GINT_TO_POINTER(watch_id));
        }
      }
    }
    g_static_mutex_unlock(&mutex_);
  }

  void Run() {
    gtk_main();
  }

  bool DoIteration(bool may_block) {
    gtk_main_iteration_do(may_block);
    // Always returns true here, because the return value of
    // gtk_main_iteration_do() has different meaning.
    return true;
  }

  void Quit() {
    gtk_main_quit();
  }

  bool IsRunning() {
    return gtk_main_level() > 0;
  }

  bool IsMainThread() const {
    return g_thread_self() == main_thread_;
  }

 private:
  void RemoveWatchNode(WatchNode *node) {
    g_static_mutex_lock(&mutex_);
    if (!node->removing) {
      node->removing = true;
      int watch_id = node->watch_id;
      WatchCallbackInterface *callback = node->callback;
      //DLOG("MainLoop::RemoveWatchNode: id=%d", watch_id);
      g_static_mutex_unlock(&mutex_);
      callback->OnRemove(main_loop_, watch_id);
      g_static_mutex_lock(&mutex_);
      g_hash_table_remove(watches_, GINT_TO_POINTER(watch_id));
    }
    g_static_mutex_unlock(&mutex_);
  }

  static gboolean ForeachRemoveCallback(gpointer key, gpointer value,
                                    gpointer data) {
    Impl *impl = static_cast<MainLoop::Impl *>(data);
    int watch_id = GPOINTER_TO_INT(key);
    WatchNode *node = static_cast<WatchNode *>(value);
    if (node->removing)
      return TRUE;
    node->removing = true;
    WatchCallbackInterface *callback = node->callback;
    g_source_remove(watch_id);
    //DLOG("MainLoop::ForeachRemoveCallback: id=%d", watch_id);
    // mutex is locked in ~Impl().
    g_static_mutex_unlock(&impl->mutex_);
    callback->OnRemove(impl->main_loop_, watch_id);
    g_static_mutex_lock(&impl->mutex_);
    return TRUE;
  }

  // Delete WatchNode when removing it from the watches_ hash table.
  static void NodeDestroyCallback(gpointer node) {
    delete static_cast<WatchNode *>(node);
  }

  // Callback functions to be registered into gtk's main loop for io watch.
  static gboolean IOWatchCallback(GIOChannel *channel, GIOCondition condition,
                                  gpointer data) {
    GGL_UNUSED(channel);
    WatchNode *node = static_cast<WatchNode *>(data);
    if (node && !node->calling && !node->removing) {
      MainLoop::Impl *impl = node->impl;
      MainLoopInterface *main_loop = impl->main_loop_;
      WatchCallbackInterface *callback = node->callback;
      //DLOG("MainLoop::IOWatchCallback: id=%d fd=%d type=%d",
      //     node->watch_id, node->data, node->type);
      bool ret = false;
      // Only call callback if the condition is correct.
      if ((node->type == MainLoopInterface::IO_READ_WATCH &&
          (condition & (G_IO_IN | G_IO_HUP | G_IO_ERR))) ||
          (node->type == MainLoopInterface::IO_WRITE_WATCH &&
          (condition & (G_IO_OUT | G_IO_HUP | G_IO_ERR)))) {
        node->calling = true;
        ret = callback->Call(main_loop, node->watch_id);
        node->calling = false;
      }

      // node->removing == true means the watch was removed during the Call.
      // However, because node->calling is set to true before invoking Call(),
      // if RemoveWatch() is called during Call() to remove this watch, it
      // won't be removed, instead, removing will be set to true. In this case,
      // the watch must be removed here.
      if (!ret || node->removing) {
        node->removing = false;
        impl->RemoveWatchNode(node);
        ret = false;
      }

      return ret;
    }
    return false;
  }

  // Callback functions to be registered into gtk's main loop for timeout
  // watch.
  static gboolean TimeoutCallback(gpointer data) {
    WatchNode *node = static_cast<WatchNode*>(data);
    if (node && !node->calling && !node->removing) {
      MainLoop::Impl *impl = node->impl;
      MainLoopInterface *main_loop = impl->main_loop_;
      WatchCallbackInterface *callback = node->callback;
      //DLOG("MainLoop::TimeoutCallback: id=%d interval=%d",
      //     node->watch_id, node->data);
      node->calling = true;
      bool ret = callback->Call(main_loop, node->watch_id);
      node->calling = false;

      if (!ret || node->removing) {
        node->removing = false;
        impl->RemoveWatchNode(node);
        ret = false;
      }

      return ret;
    }
    return false;
  }

  MainLoopInterface *main_loop_;
  GHashTable *watches_;

  GStaticMutex mutex_;
  bool destroyed_;
  GThread *main_thread_;
};

MainLoop::MainLoop()
  : impl_(new Impl(this)) {
}
MainLoop::~MainLoop() {
  delete impl_;
  impl_ = NULL;
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
  return ggadget::gtk::GetCurrentTime();
}
bool MainLoop::IsMainThread() const {
  return impl_->IsMainThread();
}
void MainLoop::WakeUp() {
  g_main_context_wakeup(g_main_context_default());
}


} // namespace gtk
} // namespace ggadget
