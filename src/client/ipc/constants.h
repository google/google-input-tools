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

#ifndef GOOPY_IPC_CONSTANTS_H_
#define GOOPY_IPC_CONSTANTS_H_
#pragma once

#include "base/basictypes.h"

namespace ipc {

const uint32 kComponentDefault = 0;
const uint32 kComponentBroadcast = 0xFFFFFFFF;

const uint32 kInputContextNone = 0;
const uint32 kInputContextFocused = 0xFFFFFFFF;

const uint32 kShiftKeyMask = 1 << 0;
const uint32 kControlKeyMask = 1 << 1;
const uint32 kAltKeyMask = 1 << 2;
const uint32 kMetaKeyMask = 1 << 3;
const uint32 kCapsLockMask = 1 << 4;
// TODO(suzhe): Add more modifier masks.

}  // namespace ipc

#endif  // GOOPY_IPC_CONSTANTS_H_
