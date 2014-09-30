/*
  Copyright 2013 Google Inc.

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

#ifndef GGADGET_MAC_COCOA_MAIN_LOOP_H_
#define GGADGET_MAC_COCOA_MAIN_LOOP_H_

#include "ggadget/main_loop_interface.h"
#include "ggadget/scoped_ptr.h"

namespace ggadget {
namespace mac {

class CocoaMainLoop : public MainLoopInterface {
 public:
  CocoaMainLoop();
  virtual ~CocoaMainLoop();
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
  scoped_ptr<Impl> impl_;
  DISALLOW_EVIL_CONSTRUCTORS(CocoaMainLoop);
};

} // namespace mac
} // namespace ggadget

#endif /* GGADGET_MAC_COCOA_MAIN_LOOP_H_ */
