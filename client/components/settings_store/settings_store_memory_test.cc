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

#include "components/settings_store/settings_store_memory.h"

#include <map>

#include "base/logging.h"
#include "components/settings_store/settings_store_test_common.h"
#include "ipc/protos/ipc.pb.h"
#include "ipc/testing.h"

namespace ime_goopy {
namespace components {

class SettingsStoreMemoryTest : public ::testing::Test,
                                public SettingsStoreMemory::Enumerator {
 protected:
  SettingsStoreMemoryTest()
      : enumerate_count_(0),
        max_enumerate_count_(0) {
  }

  // Overridden from SettingsStoreMemory::Enumerator:
  virtual bool EnumerateValue(const std::string& key,
                              const ipc::proto::Variable& value) OVERRIDE {
    EXPECT_FALSE(enumerated_values_.count(key));
    enumerated_values_[key].CopyFrom(value);

    enumerate_count_++;
    return enumerate_count_ < max_enumerate_count_;
  }

  virtual bool EnumerateArrayValue(
      const std::string& key,
      const ipc::proto::VariableArray& array) OVERRIDE {
    if (enumerated_arrays_.count(key))
      return false;

    enumerated_arrays_[key].CopyFrom(array);

    enumerate_count_++;
    return enumerate_count_ < max_enumerate_count_;
  }

  SettingsStoreMemory store_;

  int enumerate_count_;
  int max_enumerate_count_;

  std::map<std::string, ipc::proto::Variable> enumerated_values_;
  std::map<std::string, ipc::proto::VariableArray> enumerated_arrays_;
};

TEST_F(SettingsStoreMemoryTest, Value) {
  SettingsStoreTestCommon::TestValue(&store_);
}

TEST_F(SettingsStoreMemoryTest, ArrayValue) {
  SettingsStoreTestCommon::TestArray(&store_);
}

TEST_F(SettingsStoreMemoryTest, Enumerator) {
  // Stores some values.
  std::map<std::string, ipc::proto::Variable> values;
  ipc::proto::Variable value;
  value.set_type(ipc::proto::Variable::INTEGER);
  value.set_integer(1);
  ASSERT_TRUE(store_.StoreValue("value1", value, NULL));
  values["value1"] = value;
  value.set_integer(2);
  ASSERT_TRUE(store_.StoreValue("value2", value, NULL));
  values["value2"] = value;
  value.set_integer(3);
  ASSERT_TRUE(store_.StoreValue("value3", value, NULL));
  values["value3"] = value;

  // Stores some arrays.
  std::map<std::string, ipc::proto::VariableArray> arrays;
  ipc::proto::VariableArray array;
  array.add_variable()->set_type(ipc::proto::Variable::INTEGER);
  ASSERT_TRUE(store_.StoreArrayValue("array1", array, NULL));
  arrays["array1"] = array;
  array.add_variable()->set_type(ipc::proto::Variable::STRING);
  ASSERT_TRUE(store_.StoreArrayValue("array2", array, NULL));
  arrays["array2"] = array;
  array.add_variable()->set_type(ipc::proto::Variable::REAL);
  ASSERT_TRUE(store_.StoreArrayValue("array3", array, NULL));
  arrays["array3"] = array;

  // Enumerates all settings.
  max_enumerate_count_ = 100;
  EXPECT_TRUE(store_.Enumerate(this));
  EXPECT_EQ(6, enumerate_count_);
  EXPECT_EQ(3, enumerated_values_.size());
  EXPECT_EQ(3, enumerated_arrays_.size());

  for (std::map<std::string, ipc::proto::Variable>::iterator i = values.begin();
       i != values.end(); ++i) {
    ASSERT_TRUE(enumerated_values_.count(i->first));
    EXPECT_EQ(i->second.SerializeAsString(),
              enumerated_values_[i->first].SerializeAsString());
  }

  for (std::map<std::string, ipc::proto::VariableArray>::iterator i =
       arrays.begin(); i != arrays.end(); ++i) {
    ASSERT_TRUE(enumerated_arrays_.count(i->first));
    EXPECT_EQ(i->second.SerializeAsString(),
              enumerated_arrays_[i->first].SerializeAsString());
  }

  enumerated_values_.clear();
  enumerated_arrays_.clear();

  // Only enumerate three values.
  enumerate_count_ = 0;
  max_enumerate_count_ = 3;
  EXPECT_FALSE(store_.Enumerate(this));
  EXPECT_EQ(3, enumerate_count_);
  EXPECT_EQ(3, enumerated_values_.size());
  EXPECT_EQ(0, enumerated_arrays_.size());
}

}  // namespace components
}  // namespace ime_goopy
