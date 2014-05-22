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

#include <cstdio>
#include "ggadget/uuid.h"
#include "unittest/gtest.h"

using namespace ggadget;

TEST(UUID, UUID) {
  unsigned char zero[16];
  memset(zero, 0, 16);
  UUID uuid;
  ASSERT_EQ(16u, sizeof(uuid));
  ASSERT_EQ(0, memcmp(zero, &uuid, 16));

  unsigned char data[16];
  uuid.GetData(data);
  ASSERT_EQ(0, memcmp(zero, data, 16));
  ASSERT_STREQ("00000000-0000-0000-0000-000000000000",
               uuid.GetString().c_str());

  UUID uuid1("00112233-4455-6677-8899-aabbccddeeff");
  unsigned char data1[] = { 0, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
                            0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff };
  uuid1.GetData(data);
  ASSERT_EQ(0, memcmp(data1, data, 16));
  ASSERT_STREQ("00112233-4455-6677-8899-aabbccddeeff",
               uuid1.GetString().c_str());

  UUID uuid2, uuid3;
  uuid2.Generate();
  uuid3.Generate();
  ASSERT_FALSE(uuid == uuid2);
  ASSERT_FALSE(uuid == uuid3);
  ASSERT_FALSE(uuid2 == uuid3);

  uuid3.GetData(data);
  ASSERT_EQ(0x40, data[6] & 0xf0);
  ASSERT_EQ(0x80, data[8] & 0xc0);
  ASSERT_EQ('4', uuid3.GetString()[14]);

  ASSERT_FALSE(uuid2.SetString("00000000"));
  ASSERT_FALSE(uuid2.SetString("00112233445566778899aabbccddeeff"));
  ASSERT_TRUE(uuid2.SetString(uuid3.GetString()));
  ASSERT_TRUE(uuid2 == uuid3);
}

TEST(UUID, UUIDRandomLayout) {
  for (int i = 0; i < 100; i++) {
    UUID uuid;
    unsigned char data[16];
    uuid.Generate();
    uuid.GetData(data);
    ASSERT_EQ(0x40, data[6] & 0xf0);
    ASSERT_EQ(0x80, data[8] & 0xc0);
    ASSERT_EQ('4', uuid.GetString()[14]);
  }
}

int main(int argc, char **argv) {
  testing::ParseGTestFlags(&argc, argv);
  return RUN_ALL_TESTS();
}
