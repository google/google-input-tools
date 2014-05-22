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

#include <stdio.h>
#include <ggadget/common.h>
#include <ggadget/logger.h>
#include <ggadget/tests/init_extensions.h>
#include <unittest/gtest.h>
#include "../power.h"

using namespace ggadget;
using namespace ggadget::framework;
using namespace ggadget::framework::linux_system;

TEST(Power, All) {
  Power battery;
  DLOG("Is Charging: %s", battery.IsCharging() ? "yes" : "no");
  DLOG("Is PluggedIn: %s", battery.IsPluggedIn() ? "yes" : "no");
  DLOG("Percent Remaining: %d", battery.GetPercentRemaining());
  DLOG("Time Remaining: %d", battery.GetTimeRemaining());
  DLOG("Time Total: %d", battery.GetTimeTotal());
}

int main(int argc, char **argv) {
  testing::ParseGTestFlags(&argc, argv);

  static const char *kExtensions[] = {
    "libxml2_xml_parser/libxml2-xml-parser",
  };
  INIT_EXTENSIONS(argc, argv, kExtensions);

  return RUN_ALL_TESTS();
}
