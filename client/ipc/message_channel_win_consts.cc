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

#include "ipc/message_channel_win_consts.h"

namespace ipc {

const wchar_t kWinIPCServerName[] = L"ipc_server";
const wchar_t kWinIPCPipeNamePrefix[] =
    L"\\\\.\\pipe\\com_google_ime_goopy_";
const wchar_t kWinIPCSharedMemoryName[] = L"Local\\GoopyIPCSharedMemory";

}  // namespace ipc
