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

#include "common/registry_monitor.h"

#include "base/compiler_specific.h"
#include "base/logging.h"
#include "base/scoped_handle.h"
#include "base/string_utils_win.h"
#include "base/threading/platform_thread.h"

namespace ime_goopy {

RegistryMonitor::RegistryMonitor(HKEY key,
                                 const wchar_t* sub_key,
                                 Delegate* delegate)
  : delegate_(delegate),
    key_changed_(::CreateEvent(NULL, FALSE, FALSE, NULL)),
    start_monitoring_(::CreateEvent(NULL, FALSE, FALSE, NULL)),
    thread_handle_(NULL),
    started_(false) {
  DCHECK(delegate_);
  DCHECK(key_changed_);
  monitored_key_.Open(key,
                      sub_key,
                      KEY_READ | KEY_WOW64_64KEY | KEY_NOTIFY);
  DCHECK(monitored_key_.m_hKey);
}

RegistryMonitor::~RegistryMonitor() {
  if (monitored_key_.m_hKey)
    monitored_key_.Close();
}

bool RegistryMonitor::Start() {
  if (!monitored_key_.m_hKey)
    return false;
  base::PlatformThread::Create(0, this, &thread_handle_);
  DCHECK(thread_handle_);
  if (!thread_handle_)
    return false;
  if (::WaitForSingleObject(start_monitoring_.Get(), INFINITE) != WAIT_OBJECT_0)
    return false;
  return started_;
}

void RegistryMonitor::Stop() {
  DCHECK(monitored_key_.m_hKey && thread_handle_);
  if (!monitored_key_.m_hKey || !thread_handle_)
    return;
  // Closing monitored_key_ will also signal the key_changed_ event.
  monitored_key_.Close();
  DCHECK(!monitored_key_.m_hKey);
  base::PlatformThread::Join(thread_handle_);
}

void RegistryMonitor::ThreadMain() {
  while (monitored_key_.m_hKey) {
    LONG ret = ::RegNotifyChangeKeyValue(
        monitored_key_.m_hKey,
        TRUE,  // Watch sub tree
        REG_NOTIFY_CHANGE_NAME | REG_NOTIFY_CHANGE_LAST_SET,
        key_changed_.Get(),
        TRUE);  // Asynchronous
    if (ret != ERROR_SUCCESS) {
#ifdef DEBUG
      wchar_t message[MAX_PATH] = {0};
      FormatMessage(
          FORMAT_MESSAGE_FROM_SYSTEM, 0, ret, 0, message, MAX_PATH, NULL);
      DLOG(ERROR) << L"RegNotifyChangeKeyValue error with message:"
                  << message;
#endif
      if (!started_)
        ::SetEvent(start_monitoring_.Get());
      return;
    }
    if (!started_) {
      started_ = true;
      ::SetEvent(start_monitoring_.Get());
    }
    if (::WaitForSingleObject(key_changed_.Get(), INFINITE) == WAIT_OBJECT_0 &&
        monitored_key_.m_hKey) {
      delegate_->KeyChanged();
    }
  }
}

}  // namespace ime_goopy
