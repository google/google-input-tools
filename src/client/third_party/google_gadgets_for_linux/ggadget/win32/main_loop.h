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

#ifndef GGADGET_WIN32_MAIN_LOOP_H__
#define GGADGET_WIN32_MAIN_LOOP_H__

#include <ggadget/common.h>
#include <ggadget/scoped_ptr.h>
#include <ggadget/main_loop_interface.h>

namespace ggadget {
namespace win32 {

/**
 * @ingroup Win32Library
 * It's is a win32 implementation of MainLoopInterface without read/write watch
 * support.
 */
class MainLoop : public MainLoopInterface {
 public:
  MainLoop();

  virtual ~MainLoop();

  // Overridden from MainLoopInterface.
  // Not used.
  virtual int AddIOReadWatch(int fd, WatchCallbackInterface *callback);

  // Not used.
  virtual int AddIOWriteWatch(int fd, WatchCallbackInterface *callback);

  // Should be called before main loop run or in the same thread main loop is
  // running in.
  virtual int AddTimeoutWatch(int interval, WatchCallbackInterface *callback);

  // Should be called in the same thread main loop is running.
  virtual WatchType GetWatchType(int watch_id);

  // Should be called in the same thread main loop is running.
  virtual int GetWatchData(int watch_id);

  // Should be called in the same thread main loop is running.
  virtual void RemoveWatch(int watch_id);

  // Start message loop, should only be called if |main_thread| of constructor
  // is false.
  virtual void Run();

  // This function should be called only by |Run|.
  // Don't call this directly.
  virtual bool DoIteration(bool may_block);

  // This function quit message loop if running, should only be called if
  // |main_thread| of constructor is false.
  virtual void Quit();

  // Return true if loop's running.
  virtual bool IsRunning() const;

  // Simply return current time in milliseconds from systems start up.
  virtual uint64_t GetCurrentTime() const;

  // Return true if current thread is the same one as that called |Run| or
  // |DoIteration|.
  virtual bool IsMainThread() const;

  // Not used.
  virtual void WakeUp();

 private:
  class Impl;
  scoped_ptr<Impl> impl_;
  DISALLOW_EVIL_CONSTRUCTORS(MainLoop);
};

}  // namespace win32
}  // namespace ggadget
#endif  // GGADGET_WIN32_MAIN_LOOP_H__
