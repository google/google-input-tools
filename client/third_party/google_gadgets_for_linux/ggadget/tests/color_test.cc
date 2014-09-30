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
#include "ggadget/color.h"
#include "unittest/gtest.h"

using namespace ggadget;

TEST(Color, FromString) {
  Color color;
  double alpha;
  EXPECT_TRUE(Color::FromString("#123456", &color, &alpha));
  EXPECT_TRUE(Color::FromChars(0x12, 0x34, 0x56) == color);
  EXPECT_EQ(1.0, alpha);
  EXPECT_TRUE(Color::FromString("#12345678", &color, &alpha));
  EXPECT_TRUE(Color::FromChars(0x34, 0x56, 0x78) == color);
  EXPECT_EQ(0x12/255.0, alpha);
  EXPECT_TRUE(Color::FromString("#12..56", &color, &alpha));
  EXPECT_TRUE(Color::FromChars(0x12, 0, 0x56) == color);
  EXPECT_EQ(1.0, alpha);
  EXPECT_TRUE(Color::FromString("#123456", &color, NULL));
  EXPECT_TRUE(Color::FromChars(0x12, 0x34, 0x56) == color);
  EXPECT_FALSE(Color::FromString("1234567", &color, &alpha));
  EXPECT_FALSE(Color::FromString("#2345", &color, &alpha));
  EXPECT_FALSE(Color::FromString("#12345678", &color, NULL));
  EXPECT_FALSE(Color::FromString("#1234567", &color, &alpha));
  EXPECT_FALSE(Color::FromString("", &color, &alpha));
  EXPECT_FALSE(Color::FromString("#123456789", &color, &alpha));

  EXPECT_TRUE(Color::FromString("aliceblue", &color, &alpha));
  EXPECT_EQ(1.0, alpha);
  EXPECT_TRUE(Color::FromChars(240, 248, 255) == color);
  EXPECT_TRUE(Color::FromString("lightseagreen", &color, NULL));
  EXPECT_TRUE(Color::FromChars(32, 178, 170) == color);
  EXPECT_TRUE(Color::FromString("yellowgreen", &color, NULL));
  EXPECT_TRUE(Color::FromChars(154, 205, 50) == color);

  EXPECT_FALSE(Color::FromString("unknown", &color, NULL));
}

int main(int argc, char **argv) {
  testing::ParseGTestFlags(&argc, argv);
  return RUN_ALL_TESTS();
}
