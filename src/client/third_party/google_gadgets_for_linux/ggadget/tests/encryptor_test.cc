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
#include "ggadget/encryptor_interface.h"
#include "unittest/gtest.h"

using namespace ggadget;

TEST(Encryptor, Encryptor) {
  EncryptorInterface *encryptor = GetEncryptor();
  std::string input1("12345");
  std::string output1;
  encryptor->Encrypt(input1, &output1);
  ASSERT_NE(input1, output1);
  std::string check_input1;
  ASSERT_TRUE(encryptor->Decrypt(output1, &check_input1));
  ASSERT_EQ(input1, check_input1);

  std::string input2("1\02\0", 4);
  std::string output2;
  encryptor->Encrypt(input2, &output2);
  ASSERT_NE(input2, output2);
  std::string check_input2;
  ASSERT_TRUE(encryptor->Decrypt(output2, &check_input2));
  ASSERT_EQ(input2, check_input2);

  ASSERT_NE(output1, output2);
}

int main(int argc, char **argv) {
  testing::ParseGTestFlags(&argc, argv);
  return RUN_ALL_TESTS();
}
