/*
  Copyright 2011 Google Inc.

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

#ifndef GGADGET_WIN32_TESTS_MAIN_LOOP_TEST_UTILITY_H_
#define GGADGET_WIN32_TESTS_MAIN_LOOP_TEST_UTILITY_H_

#include <windows.h>

#if !defined(DEBUG)
enum {
  kWaitTimeout = 5000,
};
#else
enum {
  kWaitTimeout = INFINITE,
};
#endif  // DEBUG

static const wchar_t kTestWatchCalledEvent[] =
    L"ggadget_win32_MainLoopWatchCalledEvent";
static const wchar_t kTestWatchRemovedEvent[] =
    L"ggadget_win32_MainLoopWatchRemovedEvent";
static const wchar_t kTestProcessQuitEvent[] =
    L"ggadget_win32_MainLoopProcessQuitEvent";

#endif  // GGADGET_WIN32_TESTS_MAIN_LOOP_TEST_UTILITY_H_
