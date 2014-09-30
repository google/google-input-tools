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

#include "run_once.h"
#include <cstdlib>
#include <list>
#include <signal.h>
#include <string>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include "light_map.h"
#include "main_loop_interface.h"
#include "signals.h"

namespace ggadget {

const size_t kBufferSize = 4096;
// UNIX_PATH_MAX is not available in <sys/un.h>.
const size_t kSizeOfSunPath =
    sizeof(reinterpret_cast<struct sockaddr_un *>(0)->sun_path);

class RunOnce::Impl : public WatchCallbackInterface {
 public:
  struct Session {
    int watch_id;
    std::string data;
  };

  Impl(const char *path)
      : path_(path),
        is_running_(false),
        watch_id_(-1),
        fd_(-1) {
    ASSERT(path);
    if (path_.size() < kSizeOfSunPath) {
      fd_ = RunAsServer();
      if (fd_ == -1) {
        fd_ = RunAsClient();
        if (fd_ != -1) {
          is_running_ = true;
          return;
        } else {
          unlink(path_.c_str());
          fd_ = RunAsServer();
        }
      }
    }

    is_running_ = false;
    if (fd_ != -1)
      watch_id_ = GetGlobalMainLoop()->AddIOReadWatch(fd_, this);
  }

  ~Impl() {
    if (is_running_) {
      close(fd_);
    } else {
      // Removes watch for all running clients.
      for (LightMap<int, Session>::iterator ite = connections_.begin();
           ite != connections_.end();
           ++ite) {
        GetGlobalMainLoop()->RemoveWatch(ite->second.watch_id);
      }
      // Removes watch for the listening socket.
      GetGlobalMainLoop()->RemoveWatch(watch_id_);
      unlink(path_.c_str());
    }
  }

  bool IsRunning() const {
    return is_running_;
  }

  size_t SendMessage(const std::string &data) {
    if (!is_running_)
      return 0;

    if (fd_ == -1) {
      // In case of repeated SendMessage() calls.
      fd_ = RunAsClient();
    }

    sig_t old_proc = signal(SIGPIPE, SIG_IGN);

    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(fd_, &fds);
    struct timeval time;

    size_t written = 0;
    while (written < data.size()) {
      // Waits for at most 1 second.
      time.tv_sec = 1;
      time.tv_usec = 0;
      int result = select(fd_ + 1, NULL, &fds, NULL, &time);
      if (result == -1 || result == 0) {
        goto end;
      }
      ssize_t current = write(fd_,
                              &data.c_str()[written],
                              data.size() - written);
      if (current < 1) {
        goto end;
      }
      written += current;
    }

end:
    FD_CLR(fd_, &fds);
    close(fd_);
    fd_ = -1;
    signal(SIGPIPE, old_proc);
    return written;
  }

  Connection *ConnectOnMessage(Slot1<void, const std::string&> *slot) {
    return on_message_.Connect(slot);
  }

  virtual bool Call(MainLoopInterface *main_loop, int watch_id) {
    int fd;
    char buf[kBufferSize];

    fd = main_loop->GetWatchData(watch_id);

    if (fd_ == fd) {
      socklen_t len;
      fd = accept(fd, NULL, &len);
      connections_[fd].watch_id =
          main_loop->AddIOReadWatch(fd, this);
      return true;
    }

    ssize_t size = 0;
    if ((size = read(fd, &buf, kBufferSize)) > 0) {
      connections_[fd].data += std::string(buf, size);
    } else {
      LightMap<int, Session>::iterator ite = connections_.find(fd);
      if (ite != connections_.end()) {
        on_message_(ite->second.data);
        main_loop->RemoveWatch(watch_id);
        connections_.erase(ite);
      }
      return false;
    }

    return true;
  }

  virtual void OnRemove(MainLoopInterface *main_loop, int watch_id) {
    close(main_loop->GetWatchData(watch_id));
  }

  int RunAsServer() {
    int fd;
    struct sockaddr_un uaddr;
    uaddr.sun_family = AF_UNIX;
    ASSERT(path_.size() < kSizeOfSunPath);
    strncpy(uaddr.sun_path, path_.c_str(), kSizeOfSunPath);
    fd = socket(PF_UNIX, SOCK_STREAM, 0);
    if (bind(fd, (struct sockaddr*) &uaddr, sizeof(uaddr)) == -1) {
      close(fd);
      return -1;
    }
    listen(fd, 5);
    return fd;
  }

  int RunAsClient() {
    int fd;
    struct sockaddr_un uaddr;
    uaddr.sun_family = AF_UNIX;
    ASSERT(path_.size() < kSizeOfSunPath);
    strncpy(uaddr.sun_path, path_.c_str(), kSizeOfSunPath);
    fd = socket(PF_UNIX, SOCK_STREAM, 0);
    if (connect(fd, (struct sockaddr*) &uaddr, sizeof(uaddr)) == -1) {
      close(fd);
      return -1;
    }
    return fd;
  }

  std::string path_;
  bool is_running_;
  int watch_id_;
  int fd_;
  LightMap<int, Session> connections_;
  Signal1<void, const std::string &> on_message_;
};

RunOnce::RunOnce(const char *path)
  : impl_(new Impl(path)) {
}

RunOnce::~RunOnce() {
  delete impl_;
}

bool RunOnce::IsRunning() const {
  return impl_->IsRunning();
}

size_t RunOnce::SendMessage(const std::string &data) {
  return impl_->SendMessage(data);
}

Connection *RunOnce::ConnectOnMessage(Slot1<void, const std::string&> *slot) {
  return impl_->ConnectOnMessage(slot);
}

} // namespace ggadget
