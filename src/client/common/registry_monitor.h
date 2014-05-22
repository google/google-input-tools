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

#ifndef GOOPY_COMMON_REGISTRY_MONITOR_H_
#define GOOPY_COMMON_REGISTRY_MONITOR_H_

#include <atlbase.h>
#include <string>
#include <vector>
#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "base/scoped_handle.h"
#include "base/threading/platform_thread.h"

namespace ime_goopy {

// An monitor class that monitors a specific registry key.
// If any value or subkey under the monitored key changes, the KeyChanged
// method of the delegate will be called by the monitor.
class RegistryMonitor
  : public base::PlatformThread::Delegate {
 public:
  class Delegate {
  public:
    virtual ~Delegate() {}
    // Notifies that the key has been changed.
    virtual void KeyChanged() = 0;
  };
  RegistryMonitor(HKEY hkey, const wchar_t* sub_key, Delegate* delegate);
  virtual ~RegistryMonitor();
  // Starts monitoring the registry key.
  bool Start();
  // Stops monitoring the registry key.
  void Stop();
  // Overridden from base::PlatformThread::Delegate.
  void ThreadMain();

 private:
  Delegate* delegate_;
  CRegKey monitored_key_;
  ScopedHandle key_changed_;
  ScopedHandle start_monitoring_;
  base::PlatformThreadHandle thread_handle_;
  bool started_;
  DISALLOW_COPY_AND_ASSIGN(RegistryMonitor);
};

}  // namespace ime_goopy

#endif  // GOOPY_COMMON_REGISTRY_MONITOR_H_
