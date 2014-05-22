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

#ifndef GGADGET_TESTS_NATIVE_MAIN_LOOP_H__
#define GGADGET_TESTS_NATIVE_MAIN_LOOP_H__

#include <ggadget/common.h>
#include <ggadget/main_loop_interface.h>

namespace ggadget {

// Native implementation of MainLoopInterface.
// This implementation uses select() to watch over the file descriptors.
class NativeMainLoop : public MainLoopInterface {
 public:
  NativeMainLoop();
  virtual ~NativeMainLoop();
  virtual int AddIOReadWatch(int fd, WatchCallbackInterface *callback);
  virtual int AddIOWriteWatch(int fd, WatchCallbackInterface *callback);
  virtual int AddTimeoutWatch(int interval, WatchCallbackInterface *callback);
  virtual WatchType GetWatchType(int watch_id);
  virtual int GetWatchData(int watch_id);
  virtual void RemoveWatch(int watch_id);
  virtual void Run();
  virtual bool DoIteration(bool may_block);
  virtual void Quit();
  virtual bool IsRunning() const;
  virtual uint64_t GetCurrentTime() const;
  virtual bool IsMainThread() const;
  virtual void WakeUp();

 private:
  class Impl;
  Impl *impl_;
};

} // namespace ggadget

#endif  // GGADGET_TESTS_NATIVE_MAIN_LOOP_H__
