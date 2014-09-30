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

#include <unistd.h>
#include <cstring>
#include <vector>
#include <string>

#include <gtk/gtk.h>
#include "ggadget/logger.h"
#include "ggadget/gtk/main_loop.h"
#include "ggadget/tests/main_loop_test.h"
#include "unittest/gtest.h"

using namespace ggadget;

TEST(GtkMainLoopTest, IOReadWatch) {
  ggadget::gtk::MainLoop main_loop;
  IOReadWatchTest(&main_loop);
}

// First, test basic functionalities of main loop in single thread by adding
// many timeout watches and check if they are called for expected times in a
// certain period.
TEST(GtkMainLoopTest, TimeoutWatch) {
  ggadget::gtk::MainLoop main_loop;
  TimeoutWatchTest(&main_loop);
}


int main(int argc, char **argv) {
  testing::ParseGTestFlags(&argc, argv);
  gtk_init(&argc, &argv);
  return RUN_ALL_TESTS();
}
