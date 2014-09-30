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
#include "../wireless.h"
#include "../wireless_access_point.h"

using namespace ggadget;
using namespace ggadget::framework;
using namespace ggadget::framework::linux_system;

TEST(WirelessAndWirelessAcessPoint, All) {
  Wireless wireless;
  DLOG("Testing Wireless API Now...");
  DLOG("Is available: %s", wireless.IsAvailable() ? "yes" : "no");
  DLOG("Is connected: %s", wireless.IsConnected() ? "yes" : "no");
  DLOG("Name: %s", wireless.GetName().c_str());
  DLOG("Network name: %s", wireless.GetNetworkName().c_str());
  DLOG("Network Strength: %d", wireless.GetSignalStrength());
  DLOG("EnumerationSupported: %s",
       wireless.EnumerationSupported() ? "yes" : "no");
  DLOG("Access Pointers count: %d", wireless.GetAPCount());

  DLOG("Testing WirelessAccessPoint Now...");
  for (int i = 0; i < wireless.GetAPCount(); ++i) {
    WirelessAccessPointInterface *ap = wireless.GetWirelessAccessPoint(i);
    if (ap) {
      DLOG("Name: %s", ap->GetName().c_str());
      DLOG("Type: %d", ap->GetType());
      DLOG("Strength: %d", ap->GetSignalStrength());
      ap->Destroy();
    }
  }
}

int main(int argc, char **argv) {
  testing::ParseGTestFlags(&argc, argv);
  static const char *kExtensions[] = {
    "libxml2_xml_parser/libxml2-xml-parser",
  };
  INIT_EXTENSIONS(argc, argv, kExtensions);

  return RUN_ALL_TESTS();
}
