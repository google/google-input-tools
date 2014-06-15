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

#include "base/logging.h"
#include "ipc/testing.h"

int main(int argc, char* argv[]) {
#if defined(OS_WIN)
  testing::InitGoogleTest(&argc, argv);
  logging::InitLogging("", logging::LOG_TO_BOTH_FILE_AND_SYSTEM_DEBUG_LOG,
      logging::DONT_LOCK_LOG_FILE, logging::APPEND_TO_OLD_LOG_FILE);
#elif defined(OS_LINUX)
  testing::InitGoogleTest(&argc, argv);
#endif

  return RUN_ALL_TESTS();
}
