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

#ifndef EXTENSIONS_LINUX_SYSTEM_FRAMEWORK_PERFMON_H__
#define EXTENSIONS_LINUX_SYSTEM_FRAMEWORK_PERFMON_H__

#include <ggadget/framework_interface.h>

namespace ggadget {
namespace framework {
namespace linux_system {


class Perfmon : public PerfmonInterface {
 public:
  Perfmon();
  virtual ~Perfmon();

  virtual Variant GetCurrentValue(const char *counter_path);
  virtual int AddCounter(const char *counter_path, CallbackSlot *slot);
  virtual void RemoveCounter(int id);
 private:
  class Impl;
  Impl *impl_;
};


} // namespace linux_system
} // namespace framework
} // namespace ggadget

#endif // EXTENSIONS_LINUX_SYSTEM_FRAMEWORK_PERFMON_H__
