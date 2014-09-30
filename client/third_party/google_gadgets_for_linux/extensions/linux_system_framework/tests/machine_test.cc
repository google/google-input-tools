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
#include "../machine.h"

using namespace ggadget;
using namespace ggadget::framework;
using namespace ggadget::framework::linux_system;

TEST(Machine, GetBiosSerialNumber) {
  Machine machine;
  std::string number = machine.GetBiosSerialNumber();
  EXPECT_TRUE(number.size());
  LOG("Serial#: %s", number.c_str());
}

TEST(Machine, GetMachineManufacturer) {
  Machine machine;
  std::string vendor = machine.GetMachineManufacturer();
  EXPECT_TRUE(vendor.size());
  LOG("vendor: %s", vendor.c_str());
}

TEST(Machine, GetMachineModel) {
  Machine machine;
  std::string model = machine.GetMachineModel();
  EXPECT_TRUE(model.size());
  LOG("model: %s", model.c_str());
}

TEST(Machine, GetProcessorArchitecture) {
  Machine machine;
  std::string arch = machine.GetProcessorArchitecture();
  EXPECT_TRUE(arch.size());
  LOG("process architecture: %s", arch.c_str());
}

TEST(Machine, GetProcessorCount) {
  Machine machine;
  int cpu_count = machine.GetProcessorCount();
  EXPECT_GT(cpu_count, 0);
  LOG("cpu count: %d", cpu_count);
}

TEST(Machine, GetProcessorFamily) {
  Machine machine;
  int family = machine.GetProcessorFamily();
  EXPECT_GT(family, 0);
  LOG("process family: %d", family);
}

TEST(Machine, GetProcessorModel) {
  Machine machine;
  int model = machine.GetProcessorModel();
  EXPECT_GT(model, 0);
  LOG("process model: %d", model);
}

TEST(Machine, GetProcessorName) {
  Machine machine;
  std::string name = machine.GetProcessorName();
  EXPECT_TRUE(name.size());
  LOG("process name: %s", name.c_str());
}

TEST(Machine, GetProcessorSpeed) {
  Machine machine;
  int speed = machine.GetProcessorSpeed();
  EXPECT_GT(speed, 0);
  LOG("process speed: %d", speed);
}

TEST(Machine, GetProcessorStepping) {
  Machine machine;
  int stepping = machine.GetProcessorStepping();
  EXPECT_GT(stepping, 0);
  LOG("process stepping: %d", stepping);
}

TEST(Machine, GetProcessorVendor) {
  Machine machine;
  std::string vendor = machine.GetProcessorVendor();
  EXPECT_TRUE(vendor.size());
  LOG("process vendor: %s", vendor.c_str());
}




int main(int argc, char **argv) {
  testing::ParseGTestFlags(&argc, argv);

  static const char *kExtensions[] = {
    "libxml2_xml_parser/libxml2-xml-parser",
  };
  INIT_EXTENSIONS(argc, argv, kExtensions);

  return RUN_ALL_TESTS();
}
