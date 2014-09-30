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
//
// This class contains the implementation of default behavior of Handler
// class. This class has a message processing method and a invoke method for
// general propose. The inherit classes can override these method to provide
// their own handling logic.

#include "appsensorapi/handler.h"

namespace ime_goopy {
static const size_t kUnspecifiedFileSize = 0;
static const uint64 kUnspecifiedFileTime = 0;

Handler::Handler() {
  // Normally an executable file should not have a file size of zero and
  // file time as zero, so we utilize this as the default value. An inherit
  // class need to assign the proper value of these valuables according to
  // the real application.
  version_info_.file_size = kUnspecifiedFileSize;
  version_info_.modified_time = kUnspecifiedFileTime;
  // The file information map is initial to be empty. It should be filled
  // in the inherit class according to the real application.
}

Handler::~Handler() {}

LRESULT Handler::HandleMessage(HWND hwnd, UINT message,
                               WPARAM wparam, LPARAM lparam) const {
  // Return FALSE to indicate that the message is not processed.
  return FALSE;
}

BOOL Handler::HandleCommand(uint32 command, void *data) const {
  // Return FALSE to indicate that the command is not processed.
  return FALSE;
}
}  // namespace ime_goopy
