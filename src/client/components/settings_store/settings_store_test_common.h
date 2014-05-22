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

#ifndef GOOPY_COMPONENTS_SETTINGS_STORE_SETTINGS_STORE_TEST_COMMON_H_
#define GOOPY_COMPONENTS_SETTINGS_STORE_SETTINGS_STORE_TEST_COMMON_H_

#include "base/basictypes.h"
#include "base/logging.h"
#include "components/settings_store/settings_store_base.h"
#include "ipc/protos/ipc.pb.h"
#include "ipc/testing.h"

namespace ime_goopy {
namespace components {

// Only for settings store test use.
class SettingsStoreTestCommon {
 public:
  static void TestValue(SettingsStoreBase* store) {
    ipc::proto::Variable value;
    value.set_type(ipc::proto::Variable::INTEGER);
    value.set_integer(1);

    bool changed = false;

    // Stores a value.
    ASSERT_TRUE(store->StoreValue("value1", value, &changed));
    EXPECT_TRUE(changed);

    // Empty key is not allowed.
    ASSERT_FALSE(store->StoreValue("", value, NULL));

    // Loads a stored value.
    ipc::proto::Variable load_value;
    ASSERT_TRUE(store->LoadValue("value1", &load_value));
    EXPECT_EQ(value.SerializeAsString(), load_value.SerializeAsString());

    // Loads nonexistent values.
    load_value.set_type(ipc::proto::Variable::NONE);
    EXPECT_FALSE(store->LoadValue("", &load_value));
    EXPECT_FALSE(store->LoadValue("nonexistent", &load_value));
    EXPECT_TRUE(load_value.IsInitialized());

    // Updates an existing value.
    ASSERT_TRUE(store->StoreValue("value1", value, &changed));
    EXPECT_FALSE(changed);

    // Loads the new value.
    ASSERT_TRUE(store->LoadValue("value1", &load_value));
    EXPECT_EQ(value.SerializeAsString(), load_value.SerializeAsString());

    // Stores another value.
    value.set_integer(2);
    ASSERT_TRUE(store->StoreValue("value2", value, &changed));
    EXPECT_TRUE(changed);

    // Loads the other value.
    ASSERT_TRUE(store->LoadValue("value2", &load_value));
    EXPECT_EQ(value.SerializeAsString(), load_value.SerializeAsString());

    // Removes an old value.
    value.Clear();
    ASSERT_TRUE(store->StoreValue("value1", value, &changed));
    EXPECT_TRUE(changed);

    // The value should have been removed.
    EXPECT_FALSE(store->LoadValue("value1", &load_value));

    // Removes the value again, nothing should happen.
    ASSERT_TRUE(store->StoreValue("value1", value, &changed));
    EXPECT_FALSE(changed);

    // Checks uniqueness of keys among values and array values.
    ipc::proto::VariableArray array;
    array.add_variable()->CopyFrom(value);

    ASSERT_TRUE(store->StoreArrayValue("value2", array, &changed));
    EXPECT_TRUE(changed);

    // The value with the same key should be removed.
    EXPECT_FALSE(store->LoadValue("value2", &load_value));

    // Stores a value with the same key again.
    value.set_type(ipc::proto::Variable::INTEGER);
    value.set_integer(1);
    ASSERT_TRUE(store->StoreValue("value2", value, &changed));
    EXPECT_TRUE(changed);

    // The array value with the same key should be removed.
    EXPECT_FALSE(store->LoadArrayValue("value2", &array));
  }

  static void TestArray(SettingsStoreBase* store) {
    ipc::proto::VariableArray array;
    ipc::proto::Variable* value;
    value = array.add_variable();
    value->set_type(ipc::proto::Variable::INTEGER);
    value->set_integer(1);
    value = array.add_variable();
    value->set_type(ipc::proto::Variable::STRING);
    value->set_string("hello");

    bool changed = false;

    // Stores an array.
    ASSERT_TRUE(store->StoreArrayValue("array1", array, &changed));
    EXPECT_TRUE(changed);

    // Empty key is not allowed.
    ASSERT_FALSE(store->StoreArrayValue("", array, NULL));

    // Loads a stored array.
    ipc::proto::VariableArray load_array;
    load_array.add_variable()->set_type(ipc::proto::Variable::NONE);
    ASSERT_TRUE(store->LoadArrayValue("array1", &load_array));
    EXPECT_EQ(array.SerializeAsString(), load_array.SerializeAsString());

    // Loads nonexistent arrays.
    load_array.Clear();
    load_array.add_variable()->set_type(ipc::proto::Variable::NONE);
    EXPECT_FALSE(store->LoadArrayValue("", &load_array));
    EXPECT_FALSE(store->LoadArrayValue("nonexistent", &load_array));
    ASSERT_EQ(load_array.variable_size(), 1);
    ASSERT_TRUE(load_array.variable(0).IsInitialized());

    // Updates an existing array.
    ASSERT_TRUE(store->StoreArrayValue("array1", array, &changed));
    EXPECT_FALSE(changed);

    // Loads the new array.
    ASSERT_TRUE(store->LoadArrayValue("array1", &load_array));
    EXPECT_EQ(array.SerializeAsString(), load_array.SerializeAsString());

    // Stores another array.
    array.add_variable()->set_type(ipc::proto::Variable::DATA);
    ASSERT_TRUE(store->StoreArrayValue("array2", array, &changed));
    EXPECT_TRUE(changed);

    // Loads the other array.
    ASSERT_TRUE(store->LoadArrayValue("array2", &load_array));
    EXPECT_EQ(array.SerializeAsString(), load_array.SerializeAsString());

    // Removes an old array.
    array.Clear();
    ASSERT_TRUE(store->StoreArrayValue("array1", array, &changed));
    EXPECT_TRUE(changed);

    // The array should have been removed.
    EXPECT_FALSE(store->LoadArrayValue("array1", &load_array));

    // Removes the array again, nothing should happen.
    ASSERT_TRUE(store->StoreArrayValue("array1", array, &changed));
    EXPECT_FALSE(changed);
  }

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(SettingsStoreTestCommon);
};

}  // namespace components
}  // namespace ime_goopy

#endif  // GOOPY_COMPONENTS_SETTINGS_STORE_SETTINGS_STORE_TEST_COMMON_H_
