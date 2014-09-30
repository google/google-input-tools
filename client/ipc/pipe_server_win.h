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

#ifndef GOOPY_IPC_PIPE_SERVER_WIN_H_
#define GOOPY_IPC_PIPE_SERVER_WIN_H_

#include <windows.h>
#include <string>

#include "base/basictypes.h"
#include "base/scoped_ptr.h"

namespace ipc {

// A pipe server is responsible for creating named pipes, once a named pipe is
// connected, it will be delivered to the |delegate| for data transmission,
// after which the named pipe doesn't belong to pipe server any more.
//
// The named pipe created is asynchronous, duplex, byte-based and blocking, the
// biggest size allowed is 4M bytes.
//
// Only process that belongs to the same session could connect the pipe.
class PipeServerWin {
 public:
  // PipeServerWin will call |OnPipeConnected| to inform a Delegate that a new
  // pipe is created.
  class Delegate {
   public:
    virtual ~Delegate() {}
    virtual void OnPipeConnected(HANDLE pipe) = 0;
  };

  PipeServerWin(const std::wstring& name,
                PipeServerWin::Delegate* delegate);

  ~PipeServerWin();

  // Start the pipe server, a worker thread will be created:
  // 1. create a named pipe
  // 2. wait until the pipe is connected by a client.
  // 3. goto 1.
  // return false if the worker thread can't be created or named pipe can't be
  // created.
  // This method is not thread-safe, caller must alternately call |Start| and
  // |Stop|.
  bool Start();

  // Stop the worker thread, return after it terminates.
  // This method is not thread-safe.
  void Stop();

 private:
  class Impl;
  scoped_ptr<Impl> impl_;
  DISALLOW_COPY_AND_ASSIGN(PipeServerWin);
};

}  // namespace ipc

#endif  // GOOPY_IPC_PIPE_SERVER_WIN_H_
