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

#ifndef GGADGET_GTK_MAIN_LOOP_H__
#define GGADGET_GTK_MAIN_LOOP_H__
#include <ggadget/main_loop_interface.h>

namespace ggadget {
namespace gtk {

/**
 * @ingroup GtkLibrary
 * It's is a wrapper around gtk's main loop functions to implement
 * MainLoopInterface interface.
 */
class MainLoop : public MainLoopInterface {
 public:
  MainLoop();
  virtual ~MainLoop();
  virtual int AddIOReadWatch(int fd, WatchCallbackInterface *callback);
  virtual int AddIOWriteWatch(int fd, WatchCallbackInterface *callback);
  virtual int AddTimeoutWatch(int interval, WatchCallbackInterface *callback);
  virtual WatchType GetWatchType(int watch_id);
  virtual int GetWatchData(int watch_id);
  virtual void RemoveWatch(int watch_id);
  // This function just call gtk_main(). So you can use either gtk_main() or
  // this function.
  virtual void Run();
  // This function just call gtk_main_iteration_do(). So you can use either
  // gtk_main_iteration_do() or this function.
  virtual bool DoIteration(bool may_block);
  // This function just call gtk_main_quit().
  virtual void Quit();
  virtual bool IsRunning() const;
  virtual uint64_t GetCurrentTime() const;
  virtual bool IsMainThread() const;
  virtual void WakeUp();

 private:
  class Impl;
  Impl *impl_;
  DISALLOW_EVIL_CONSTRUCTORS(MainLoop);
};

} // namespace gtk
} // namespace ggadget
#endif  // GGADGET_GTK_MAIN_LOOP_H__
