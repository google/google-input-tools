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

#ifndef GGADGET_RUN_ONCE_H__
#define GGADGET_RUN_ONCE_H__

#include <ggadget/common.h>
#include <ggadget/slot.h>
#include <string>

namespace ggadget {

/**
 * @ingroup Utilities
 * Class for hosts to keep only one running instance.
 */
class RunOnce {
 public:
  /**
   * Initializes the unique instance application.
   * The instance of this class will be deleted automatically when the MainLoop
   * object destructs.  So don't create it on stack. Please always use operator
   * @c new to create the instance of this object and don't delete it manually.
   *
   * @param path the UNIX domain socket for the application.  The caller has to
   *     ensure that the path is valid.
   */
  RunOnce(const char *path);
  ~RunOnce();

 public:
  /**
   * Tests whether there is another instance of this application running.
   * @return @c true if there is another instance, or @c false if current
   *     instance is the only one instance.
   */
  bool IsRunning() const;
  /**
   * Sends a message to the existing instance.
   * @param handle the handle to the application.
   * @param data the data to send.
   * @return the size of the data when succeeded, or 0 if error occured.
   */
  size_t SendMessage(const std::string &data);

  /**
   * Adds a slot for monitoring the messages from the other instances.
   */
  Connection *ConnectOnMessage(Slot1<void, const std::string&> *slot);

 private:
  class Impl;
  Impl *impl_;

  DISALLOW_EVIL_CONSTRUCTORS(RunOnce);
};

} // namespace ggadget

#endif // GGADGET_RUN_ONCE_H__
