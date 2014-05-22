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

#include "common/registry.h"

#include "base/logging.h"
#include "base/scoped_ptr.h"
#include <gtest/gunit.h>

namespace ime_goopy {
// Test location, this key will be deleted after the unittest.
static const wchar_t kSoftwareKey[] = L"Software";
static const wchar_t kRootKey[] = L"Software\\GooglePinyinUnittest";
static const wchar_t kRootKeyName[] = L"GooglePinyinUnittest";
static const wchar_t kStringName[] = L"TestString";
static const wchar_t kStringValue[] = L"TestValue";
static const wchar_t kStringName2[] = L"TestString2";
static const wchar_t kStringValue2[] = L"TestValue2";
static const wchar_t kEmptyStringValue[] = L"";
static const wchar_t kMultiStringName[] = L"TestMultiString";
static const wchar_t kIntegerName[] = L"TestInteger";
static const uint32 kIntegerValue = 50;
static const wchar_t kBinaryName[] = L"TestBinary";
static const uint8 kBinaryValue[] = "TestBinary";

class RegistryValueTest : public testing::Test {
 public:
  void SetUp() {
    // Make sure we get an empty key.
    RegistryKey::RecurseDeleteKey(HKEY_CURRENT_USER, kRootKey, 0);
    ASSERT_EQ(NULL,
              RegistryKey::OpenKey(HKEY_CURRENT_USER, kRootKey, KEY_READ));
    ASSERT_EQ(ERROR_SUCCESS,
              key_.Create(HKEY_CURRENT_USER, kRootKey));
  }

  void TearDown() {
    // Delete the key after unittests.
    EXPECT_EQ(ERROR_SUCCESS, key_.Close());
    RegistryKey::RecurseDeleteKey(HKEY_CURRENT_USER, kRootKey, 0);
    ASSERT_EQ(NULL,
              RegistryKey::OpenKey(HKEY_CURRENT_USER, kRootKey, KEY_READ));
  }

 protected:
  RegistryKey key_;
};

TEST_F(RegistryValueTest, String) {
  wstring value;
  EXPECT_NE(ERROR_SUCCESS, key_.QueryStringValue(kStringName, &value));
  EXPECT_EQ(ERROR_SUCCESS, key_.SetStringValue(kStringName, kStringValue));
  EXPECT_EQ(ERROR_SUCCESS, key_.QueryStringValue(kStringName, &value));
  EXPECT_STREQ(kStringValue, value.c_str());

  // Empty string.
  EXPECT_EQ(ERROR_SUCCESS, key_.SetStringValue(kStringName, kEmptyStringValue));
  EXPECT_EQ(ERROR_SUCCESS, key_.QueryStringValue(kStringName, &value));
  EXPECT_STREQ(kEmptyStringValue, value.c_str());

  bool existed = true;
  wstring previous_value;
  EXPECT_EQ(ERROR_SUCCESS, key_.SetStringValueIfNotExisted(kStringName2,
                                                           kStringValue2,
                                                           &existed,
                                                           &previous_value));
  EXPECT_FALSE(existed);
  EXPECT_STREQ(kEmptyStringValue, previous_value.c_str());
  EXPECT_EQ(ERROR_SUCCESS, key_.QueryStringValue(kStringName2, &value));
  EXPECT_STREQ(kStringValue2, value.c_str());
  existed = false;
  previous_value.clear();
  EXPECT_EQ(ERROR_SUCCESS, key_.SetStringValueIfNotExisted(kStringName2,
                                                           kStringValue,
                                                           &existed,
                                                           &previous_value));
  EXPECT_TRUE(existed);
  EXPECT_STREQ(kStringValue2, previous_value.c_str());
  EXPECT_EQ(ERROR_SUCCESS, key_.QueryStringValue(kStringName2, &value));
  EXPECT_STREQ(kStringValue2, value.c_str());
}

TEST_F(RegistryValueTest, MultiString) {
  vector<wstring> values;
  vector<wstring> results;
  EXPECT_NE(ERROR_SUCCESS,
            key_.QueryMultiStringValue(kMultiStringName, &results));
  EXPECT_EQ(ERROR_SUCCESS, key_.SetMultiStringValue(kMultiStringName, values));
  EXPECT_EQ(ERROR_SUCCESS,
            key_.QueryMultiStringValue(kMultiStringName, &results));
  EXPECT_EQ(0, results.size());
  values.push_back(L"001");
  values.push_back(L"");
  values.push_back(L"002");
  values.push_back(L"");
  values.push_back(L"");
  values.push_back(L"003");
  values.push_back(L"");
  values.push_back(L"004");
  EXPECT_EQ(ERROR_SUCCESS, key_.SetMultiStringValue(kMultiStringName, values));
  EXPECT_EQ(ERROR_SUCCESS,
            key_.QueryMultiStringValue(kMultiStringName, &results));
  EXPECT_EQ(values.size(), results.size());
  for (int i = 0; i < results.size(); ++i) {
    EXPECT_EQ(values[i], results[i]);
  }
}

TEST_F(RegistryValueTest, Binary) {
  ULONG length = 0;
  scoped_array<uint8> buffer;
  EXPECT_NE(ERROR_SUCCESS,
            key_.QueryBinaryValue(kBinaryName, &buffer, &length));
  EXPECT_TRUE(buffer.get() == NULL);

  EXPECT_EQ(
      ERROR_SUCCESS,
      key_.SetBinaryValue(kBinaryName, kBinaryValue, arraysize(kBinaryValue)));
  EXPECT_EQ(ERROR_SUCCESS,
            key_.QueryBinaryValue(kBinaryName, &buffer, &length));
  EXPECT_TRUE(buffer.get() != NULL);
  EXPECT_EQ(arraysize(kBinaryValue), length);
  for (uint32 i = 0; i < length; i++)
    EXPECT_EQ(kBinaryValue[i], buffer.get()[i]);
}

TEST_F(RegistryValueTest, EnumKey) {
  RegistryKey softare_key;
  EXPECT_EQ(ERROR_SUCCESS,
            softare_key.Open(HKEY_CURRENT_USER, kSoftwareKey, KEY_READ));
  uint32 index = 0;
  set<wstring> all_subkeys;
  wstring name;
  while (softare_key.EnumKey(index++, &name) == ERROR_SUCCESS)
    all_subkeys.insert(name);
  EXPECT_FALSE(all_subkeys.find(kRootKeyName) == all_subkeys.end());
}

TEST_F(RegistryValueTest, Encrypted) {
  wstring value;
  EXPECT_NE(ERROR_SUCCESS, key_.QueryEncryptedValue(kStringName, &value));
  EXPECT_EQ(ERROR_SUCCESS, key_.SetEncryptedValue(kStringName, kStringValue));
  EXPECT_EQ(ERROR_SUCCESS, key_.QueryEncryptedValue(kStringName, &value));
  EXPECT_STREQ(kStringValue, value.c_str());
}
}  // namespace ime_goopy
