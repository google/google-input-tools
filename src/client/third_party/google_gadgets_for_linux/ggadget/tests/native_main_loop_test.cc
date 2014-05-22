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

#include <cstring>
#include <vector>
#include <string>

#include "ggadget/common.h"
#include "ggadget/logger.h"
#include "native_main_loop.h"
#include "main_loop_test.h"
#include "unittest/gtest.h"

using namespace ggadget;

TEST(GtkMainLoopTest, IOReadWatch) {
  NativeMainLoop main_loop;
  IOReadWatchTest(&main_loop);
}

TEST(GtkMainLoopTest, TimeoutWatch) {
  NativeMainLoop main_loop;
  TimeoutWatchTest(&main_loop);
}

int main(int argc, char **argv) {
  testing::ParseGTestFlags(&argc, argv);
  return RUN_ALL_TESTS();
}
