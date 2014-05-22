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

#include "components/settings_store/settings_store_base.h"

#include <algorithm>
#include <vector>

#include "base/logging.h"
#include "base/scoped_ptr.h"
#include "ipc/message_types.h"
#include "ipc/mock_component_host.h"
#include "ipc/protos/ipc.pb.h"
#include "ipc/testing.h"

namespace {

using namespace ipc;

static bool ProtoMessageEqual(const ::google::protobuf::MessageLite& a,
                              const ::google::protobuf::MessageLite& b) {
  return a.SerializePartialAsString() == b.SerializePartialAsString();
}

class MockSettingsStore : public ime_goopy::components::SettingsStoreBase {
 public:
  enum StoreResult {
    NONE,
    FAILED,
    SUCCEEDED_UNCHANGED,
    SUCCEEDED_CHANGED,
  };

  MockSettingsStore() {
  }

  virtual ~MockSettingsStore() {
  }

  void SetExpectedStoreResult(const std::string& key, StoreResult result) {
    expected_store_results_[key] = result;
  }

  void SetExpectedLoadResult(const std::string& key, bool result) {
    expected_load_results_[key] = result;
  }

  bool CheckStoredValue(const std::string& key,
                        const proto::Variable& value) {
    if (!stored_values_.count(key))
      return false;
    return ProtoMessageEqual(value, stored_values_[key]);
  }

  bool CheckStoredArrayValue(const std::string& key,
                             const proto::VariableArray& array) {
    if (!stored_array_values_.count(key))
      return false;
    return ProtoMessageEqual(array, stored_array_values_[key]);
  }

  void Clear() {
    expected_store_results_.clear();
    expected_load_results_.clear();
    stored_values_.clear();
    stored_array_values_.clear();
  }

 protected:
  // Overridden from SettingsStoreBase:
  virtual bool StoreValue(const std::string& key,
                          const proto::Variable& value,
                          bool* changed) OVERRIDE {
    DCHECK(changed);
    stored_values_[key].CopyFrom(value);
    if (!expected_store_results_.count(key)) {
      *changed = true;
      return true;
    }

    switch (expected_store_results_[key]) {
      case SUCCEEDED_UNCHANGED:
        *changed = false;
        return true;
      case SUCCEEDED_CHANGED:
        *changed = true;
        return true;
      default:
        *changed = false;
        return false;
    }
  }
  virtual bool LoadValue(const std::string& key,
                         proto::Variable* value) OVERRIDE {
    if (!expected_load_results_.count(key) || expected_load_results_[key]) {
      value->set_type(proto::Variable::STRING);
      value->set_string(key);
      return true;
    }
    value->Clear();
    value->set_type(proto::Variable::NONE);
    return false;
  }
  virtual bool StoreArrayValue(const std::string& key,
                               const proto::VariableArray& array,
                               bool* changed) OVERRIDE {
    DCHECK(changed);
    stored_array_values_[key].CopyFrom(array);
    if (!expected_store_results_.count(key)) {
      *changed = true;
      return true;
    }

    switch (expected_store_results_[key]) {
      case SUCCEEDED_UNCHANGED:
        *changed = false;
        return true;
      case SUCCEEDED_CHANGED:
        *changed = true;
        return true;
      default:
        *changed = false;
        return false;
    }
  }
  virtual bool LoadArrayValue(const std::string& key,
                              proto::VariableArray* array) OVERRIDE {
    if (!expected_load_results_.count(key) || expected_load_results_[key]) {
      proto::Variable* var = array->add_variable();
      var->set_type(proto::Variable::STRING);
      var->set_string(key + "-1");
      var = array->add_variable();
      var->set_type(proto::Variable::STRING);
      var->set_string(key + "-2");
      return true;
    }
    array->Clear();
    return false;
  }

 private:
  std::map<std::string, StoreResult> expected_store_results_;
  std::map<std::string, bool> expected_load_results_;
  std::map<std::string, proto::Variable> stored_values_;
  std::map<std::string, proto::VariableArray> stored_array_values_;

  DISALLOW_COPY_AND_ASSIGN(MockSettingsStore);
};

}  // namespace

namespace ime_goopy {
namespace components {

class SettingsStoreBaseTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
    host_.AddComponent(&store_);
  }

  virtual void TearDown() {
    host_.RemoveComponent(&store_);
  }

  proto::Message* NewMessage(uint32 type, bool need_reply) {
    proto::Message* msg = new proto::Message();
    msg->set_type(type);
    msg->set_reply_mode(need_reply ? proto::Message::NEED_REPLY :
                        proto::Message::NO_REPLY);
    msg->set_target(store_.id());
    return msg;
  }

  void AddObserver(uint32 source, const std::string& key) {
    scoped_ptr<proto::Message> mptr(
        NewMessage(MSG_SETTINGS_ADD_CHANGE_OBSERVER, true));
    mptr->set_source(source);
    mptr->mutable_payload()->add_string(key);
    ASSERT_TRUE(host_.HandleMessage(mptr.release()));

    mptr.reset(host_.PopOutgoingMessage());
    ASSERT_EQ(MSG_SETTINGS_ADD_CHANGE_OBSERVER, mptr->type());
    ASSERT_EQ(proto::Message::IS_REPLY, mptr->reply_mode());
    ASSERT_TRUE(mptr->payload().boolean_size());
    ASSERT_TRUE(mptr->payload().boolean(0));
  }

  void RemoveObserver(uint32 source, const std::string& key) {
    scoped_ptr<proto::Message> mptr(
        NewMessage(MSG_SETTINGS_REMOVE_CHANGE_OBSERVER, true));
    mptr->set_source(source);
    mptr->mutable_payload()->add_string(key);
    ASSERT_TRUE(host_.HandleMessage(mptr.release()));

    mptr.reset(host_.PopOutgoingMessage());
    ASSERT_EQ(MSG_SETTINGS_REMOVE_CHANGE_OBSERVER, mptr->type());
    ASSERT_EQ(proto::Message::IS_REPLY, mptr->reply_mode());
    ASSERT_TRUE(mptr->payload().boolean_size());
    ASSERT_TRUE(mptr->payload().boolean(0));
  }

  MockComponentHost host_;
  MockSettingsStore store_;
};

TEST_F(SettingsStoreBaseTest, ObserverMapTest) {
  static const char *kPatterns[] = {
    "aaa",       // 1
    "aaa*",      // 2
    "aabb",      // 3
    "aabbc*",    // 4
    "aabbcc",    // 5
    "ab*",       // 6
    "abc*",      // 7
    "abcde*",    // 8
    "abd*",      // 9
    "bbbccc",    // 10
    "bbcc*",     // 11
    "bbccdd",    // 12
    "c*",        // 13
    "ccdd*",     // 14
    "ddeeffgg",  // 15
    NULL,
  };

  static const struct {
    const char* key;
    const size_t results_length;
    const uint32 results[10];
  } kTests[] = {
    { "a",        0, {} },           // Match nothing
    { "aaa",      2, { 1, 2 } },     // Match "aaa", "aaa*"
    { "aaaa",     1, { 2 } },        // Match "aaa*"
    { "aabb",     1, { 3 } },        // Match "aabb"
    { "aabbccdd", 1, { 4 } },        // Match "aabbc*"
    { "abcdef",   3, { 6, 7, 8 } },  // Match "ab*", "abc*", "abcde*"
    { "bbbcc",    0, {} },           // Match nothing
    { "ccdd",     2, { 13, 14 } },   // Match "c*", "ccdd*"
    { "ddeeffgg", 1, { 15 } },       // Match "ddeeffgg"
    { NULL, 0, { } },
  };

  ime_goopy::components::SettingsStoreBase::ObserverMap observers;

  EXPECT_TRUE(observers.empty());

  for (int i = 0; kPatterns[i]; ++i)
    observers.Add(kPatterns[i], i + 1);

  EXPECT_FALSE(observers.empty());

  std::vector<uint32> matched;
  // Test normal matching.
  for (int i = 0; kTests[i].key; ++i) {
    matched.clear();
    observers.Match(kTests[i].key, -1, &matched);
    std::sort(matched.begin(), matched.end());

    ASSERT_EQ(kTests[i].results_length, matched.size());
    for (size_t j = 0; j < kTests[i].results_length; ++j)
      EXPECT_EQ(kTests[i].results[j], matched[j]);
  }

  // Matching with a "*" entry.
  observers.Add("*", 0);
  for (int i = 0; kTests[i].key; ++i) {
    matched.clear();
    observers.Match(kTests[i].key, -1, &matched);
    std::sort(matched.begin(), matched.end());

    ASSERT_EQ(kTests[i].results_length + 1, matched.size());
    ASSERT_EQ(0, matched[0]);
    for (size_t j = 0; j < kTests[i].results_length; ++j)
      EXPECT_EQ(kTests[i].results[j], matched[j + 1]);
  }

  // Test |ignore| argument.
  matched.clear();
  observers.Match("aaaa", 0, &matched);
  ASSERT_EQ(1U, matched.size());
  EXPECT_EQ(2, matched[0]);

  // Multiple observers watching the same key
  observers.Add("ddeeffgg", 100);
  matched.clear();
  observers.Match("ddeeffgg", -1, &matched);
  ASSERT_EQ(3U, matched.size());
  EXPECT_EQ(0, matched[0]);
  EXPECT_EQ(15, matched[1]);
  EXPECT_EQ(100, matched[2]);

  // Remove
  observers.Remove("ddeeffgg", 100);
  matched.clear();
  observers.Match("ddeeffgg", -1, &matched);
  ASSERT_EQ(2U, matched.size());
  EXPECT_EQ(0, matched[0]);
  EXPECT_EQ(15, matched[1]);

  // RemoveObserver
  observers.RemoveObserver(0);
  matched.clear();
  observers.Match("ddeeffgg", -1, &matched);
  ASSERT_EQ(1U, matched.size());
  EXPECT_EQ(15, matched[0]);

  // Normalize key
  observers.Add("a1@3b%/ *c-d_e*", 100);
  matched.clear();
  observers.Match("a1_3b_/__c-d_efg", -1, &matched);
  ASSERT_EQ(1U, matched.size());
  EXPECT_EQ(100, matched[0]);

  // Remove all patterns.
  for (int i = 0; kPatterns[i]; ++i)
    observers.Remove(kPatterns[i], i + 1);
  observers.RemoveObserver(100);
  EXPECT_TRUE(observers.empty());
}

TEST_F(SettingsStoreBaseTest, SetValues) {
  scoped_ptr<proto::Message> mptr(
      NewMessage(MSG_SETTINGS_SET_VALUES, true));

  mptr->set_source(0);
  proto::MessagePayload* payload = mptr->mutable_payload();

  // Adds some settings.
  const std::string keys[4] = { "key1", "key2", "key3", "key4" };
  proto::Variable values[4];

  values[0].set_type(proto::Variable::INTEGER);
  values[0].set_integer(1234);
  payload->add_string(keys[0]);
  payload->add_variable()->CopyFrom(values[0]);

  values[1].set_type(proto::Variable::STRING);
  values[1].set_string("hello");
  payload->add_string(keys[1]);
  payload->add_variable()->CopyFrom(values[1]);

  values[2].set_type(proto::Variable::BOOLEAN);
  values[2].set_boolean(true);
  payload->add_string(keys[2]);
  payload->add_variable()->CopyFrom(values[2]);

  // Add a key without corresponding value to delete the old value of the key.
  payload->add_string(keys[3]);

  // Adds some observers.
  const uint32 targets[3] = { 23, 45, 67 };
  ASSERT_NO_FATAL_FAILURE(AddObserver(0, "*"));
  ASSERT_NO_FATAL_FAILURE(AddObserver(targets[0], "*"));
  ASSERT_NO_FATAL_FAILURE(AddObserver(targets[1], "k*"));
  ASSERT_NO_FATAL_FAILURE(AddObserver(targets[2], "key1"));

  // Sets expected results.
  store_.SetExpectedStoreResult(keys[0], MockSettingsStore::SUCCEEDED_CHANGED);
  store_.SetExpectedStoreResult(keys[1],
                                MockSettingsStore::SUCCEEDED_UNCHANGED);
  store_.SetExpectedStoreResult(keys[2], MockSettingsStore::FAILED);
  store_.SetExpectedStoreResult(keys[3], MockSettingsStore::SUCCEEDED_CHANGED);

  ASSERT_TRUE(host_.HandleMessage(mptr.release()));

  for (uint32 i = 0; i < 4; ++i)
    EXPECT_TRUE(store_.CheckStoredValue(keys[i], values[i]));

  // Three observer messages for key1 change. We won't receive observer message
  // to the component changing the values.
  for (uint32 i = 0; i < 3; ++i) {
    SCOPED_TRACE(testing::Message() << "Check observer message for key1:" << i);
    mptr.reset(host_.PopOutgoingMessage());
    ASSERT_TRUE(mptr.get());
    ASSERT_EQ(MSG_SETTINGS_CHANGED, mptr->type());
    ASSERT_EQ(1, mptr->payload().string_size());
    ASSERT_EQ(1, mptr->payload().variable_size());
    EXPECT_EQ(targets[i], mptr->target());
    EXPECT_EQ(keys[0], mptr->payload().string(0));
    EXPECT_TRUE(ProtoMessageEqual(values[0], mptr->payload().variable(0)));
  }

  // Two observer messages for key4 change. We won't receive observer message
  // to the component changing the values.
  for (uint32 i = 0; i < 2; ++i) {
    SCOPED_TRACE(testing::Message() << "Check observer message for key4:" << i);
    mptr.reset(host_.PopOutgoingMessage());
    ASSERT_TRUE(mptr.get());
    ASSERT_EQ(MSG_SETTINGS_CHANGED, mptr->type());
    ASSERT_EQ(1, mptr->payload().string_size());
    ASSERT_EQ(1, mptr->payload().variable_size());
    EXPECT_EQ(targets[i], mptr->target());
    EXPECT_EQ(keys[3], mptr->payload().string(0));
    EXPECT_TRUE(ProtoMessageEqual(values[3], mptr->payload().variable(0)));
  }

  // Reply message.
  mptr.reset(host_.PopOutgoingMessage());
  ASSERT_TRUE(mptr.get());
  ASSERT_EQ(MSG_SETTINGS_SET_VALUES, mptr->type());
  ASSERT_EQ(proto::Message::IS_REPLY, mptr->reply_mode());
  ASSERT_EQ(4, mptr->payload().boolean_size());
  EXPECT_TRUE(mptr->payload().boolean(0));
  EXPECT_TRUE(mptr->payload().boolean(1));
  EXPECT_FALSE(mptr->payload().boolean(2));
  EXPECT_TRUE(mptr->payload().boolean(3));

  // Test invalid message.
  mptr.reset(NewMessage(MSG_SETTINGS_SET_VALUES, true));
  ASSERT_TRUE(host_.HandleMessage(mptr.release()));
  mptr.reset(host_.PopOutgoingMessage());
  ASSERT_TRUE(mptr.get());
  ASSERT_EQ(MSG_SETTINGS_SET_VALUES, mptr->type());
  ASSERT_EQ(proto::Message::IS_REPLY, mptr->reply_mode());
  ASSERT_TRUE(mptr->payload().has_error());
  ASSERT_EQ(proto::Error::INVALID_PAYLOAD, mptr->payload().error().code());
}

TEST_F(SettingsStoreBaseTest, GetValues) {
  scoped_ptr<proto::Message> mptr(
      NewMessage(MSG_SETTINGS_GET_VALUES, true));

  mptr->set_source(0);
  proto::MessagePayload* payload = mptr->mutable_payload();

  // Adds some settings.
  std::string keys[3] = { "key1", "key2", "key3" };
  const bool results[3] = { true, false, true };
  for (size_t i = 0; i < 3; ++i) {
    payload->add_string(keys[i]);
    store_.SetExpectedLoadResult(keys[i], results[i]);
  }

  ASSERT_TRUE(host_.HandleMessage(mptr.release()));

  // Reply message.
  mptr.reset(host_.PopOutgoingMessage());
  ASSERT_TRUE(mptr.get());
  ASSERT_EQ(MSG_SETTINGS_GET_VALUES, mptr->type());
  ASSERT_EQ(proto::Message::IS_REPLY, mptr->reply_mode());
  ASSERT_EQ(3, mptr->payload().string_size());
  ASSERT_EQ(3, mptr->payload().variable_size());

  for (size_t i = 0; i < 3; ++i) {
    SCOPED_TRACE(testing::Message() << "Check load result:" << i);
    EXPECT_EQ(keys[i], mptr->payload().string(i));
    if (results[i]) {
      EXPECT_EQ(proto::Variable::STRING, mptr->payload().variable(i).type());
      EXPECT_EQ(keys[i], mptr->payload().variable(i).string());
    } else {
      EXPECT_EQ(proto::Variable::NONE, mptr->payload().variable(i).type());
    }
  }

  // Test invalid message.
  mptr.reset(NewMessage(MSG_SETTINGS_GET_VALUES, true));
  ASSERT_TRUE(host_.HandleMessage(mptr.release()));
  mptr.reset(host_.PopOutgoingMessage());
  ASSERT_TRUE(mptr.get());
  ASSERT_EQ(MSG_SETTINGS_GET_VALUES, mptr->type());
  ASSERT_EQ(proto::Message::IS_REPLY, mptr->reply_mode());
  ASSERT_TRUE(mptr->payload().has_error());
  ASSERT_EQ(proto::Error::INVALID_PAYLOAD, mptr->payload().error().code());
}

TEST_F(SettingsStoreBaseTest, SetArrayValue) {
  scoped_ptr<proto::Message> mptr(
      NewMessage(MSG_SETTINGS_SET_ARRAY_VALUE, true));

  mptr->set_source(0);
  proto::MessagePayload* payload = mptr->mutable_payload();

  // Adds some settings.
  const std::string key = "key1";
  proto::VariableArray array;
  proto::Variable* value = array.add_variable();

  value->set_type(proto::Variable::INTEGER);
  value->set_integer(1234);

  value = array.add_variable();
  value->set_type(proto::Variable::STRING);
  value->set_string("hello");

  value = array.add_variable();
  value->set_type(proto::Variable::BOOLEAN);
  value->set_boolean(true);

  payload->mutable_variable()->MergeFrom(array.variable());
  payload->add_string(key);

  // Adds some observers.
  const uint32 targets[3] = { 23, 45, 67 };
  ASSERT_NO_FATAL_FAILURE(AddObserver(0, "*"));
  ASSERT_NO_FATAL_FAILURE(AddObserver(targets[0], "*"));
  ASSERT_NO_FATAL_FAILURE(AddObserver(targets[1], "k*"));
  ASSERT_NO_FATAL_FAILURE(AddObserver(targets[2], "key1"));

  // Sets expected results.
  store_.SetExpectedStoreResult(key, MockSettingsStore::SUCCEEDED_CHANGED);

  ASSERT_TRUE(host_.HandleMessage(mptr.release()));

  EXPECT_TRUE(store_.CheckStoredArrayValue(key, array));

  // Three observer messages. We won't receive observer message to the component
  // changing the values.
  for (uint32 i = 0; i < 3; ++i) {
    SCOPED_TRACE(testing::Message() << "Check observer message:" << i);
    mptr.reset(host_.PopOutgoingMessage());
    ASSERT_TRUE(mptr.get());
    ASSERT_EQ(MSG_SETTINGS_CHANGED, mptr->type());
    ASSERT_EQ(1, mptr->payload().string_size());
    ASSERT_EQ(3, mptr->payload().variable_size());
    EXPECT_EQ(targets[i], mptr->target());
    EXPECT_EQ(key, mptr->payload().string(0));
    EXPECT_TRUE(ProtoMessageEqual(array.variable(0),
                                  mptr->payload().variable(0)));
    EXPECT_TRUE(ProtoMessageEqual(array.variable(1),
                                  mptr->payload().variable(1)));
    EXPECT_TRUE(ProtoMessageEqual(array.variable(2),
                                  mptr->payload().variable(2)));
  }

  // Reply message.
  mptr.reset(host_.PopOutgoingMessage());
  ASSERT_TRUE(mptr.get());
  ASSERT_EQ(MSG_SETTINGS_SET_ARRAY_VALUE, mptr->type());
  ASSERT_EQ(proto::Message::IS_REPLY, mptr->reply_mode());
  ASSERT_EQ(1, mptr->payload().boolean_size());
  EXPECT_TRUE(mptr->payload().boolean(0));

  // Test invalid message.
  mptr.reset(NewMessage(MSG_SETTINGS_SET_ARRAY_VALUE, true));
  ASSERT_TRUE(host_.HandleMessage(mptr.release()));
  mptr.reset(host_.PopOutgoingMessage());
  ASSERT_TRUE(mptr.get());
  ASSERT_EQ(MSG_SETTINGS_SET_ARRAY_VALUE, mptr->type());
  ASSERT_EQ(proto::Message::IS_REPLY, mptr->reply_mode());
  ASSERT_TRUE(mptr->payload().has_error());
  ASSERT_EQ(proto::Error::INVALID_PAYLOAD, mptr->payload().error().code());

  mptr.reset(NewMessage(MSG_SETTINGS_SET_ARRAY_VALUE, true));
  mptr->mutable_payload()->add_string("key1");
  mptr->mutable_payload()->add_string("key2");
  ASSERT_TRUE(host_.HandleMessage(mptr.release()));
  mptr.reset(host_.PopOutgoingMessage());
  ASSERT_TRUE(mptr.get());
  ASSERT_EQ(MSG_SETTINGS_SET_ARRAY_VALUE, mptr->type());
  ASSERT_EQ(proto::Message::IS_REPLY, mptr->reply_mode());
  ASSERT_TRUE(mptr->payload().has_error());
  ASSERT_EQ(proto::Error::INVALID_PAYLOAD, mptr->payload().error().code());
}

TEST_F(SettingsStoreBaseTest, GetArrayValue) {
  store_.SetExpectedLoadResult("key1", true);
  store_.SetExpectedLoadResult("key2", false);

  // Success case.
  scoped_ptr<proto::Message> mptr(
      NewMessage(MSG_SETTINGS_GET_ARRAY_VALUE, true));
  mptr->set_source(0);
  mptr->mutable_payload()->add_string("key1");

  ASSERT_TRUE(host_.HandleMessage(mptr.release()));

  // Reply message.
  mptr.reset(host_.PopOutgoingMessage());
  ASSERT_TRUE(mptr.get());
  ASSERT_EQ(MSG_SETTINGS_GET_ARRAY_VALUE, mptr->type());
  ASSERT_EQ(proto::Message::IS_REPLY, mptr->reply_mode());
  ASSERT_EQ(1, mptr->payload().string_size());
  ASSERT_EQ(2, mptr->payload().variable_size());
  EXPECT_EQ("key1", mptr->payload().string(0));
  EXPECT_EQ(proto::Variable::STRING, mptr->payload().variable(0).type());
  EXPECT_EQ("key1-1", mptr->payload().variable(0).string());
  EXPECT_EQ(proto::Variable::STRING, mptr->payload().variable(1).type());
  EXPECT_EQ("key1-2", mptr->payload().variable(1).string());

  // Failure case.
  mptr.reset(NewMessage(MSG_SETTINGS_GET_ARRAY_VALUE, true));
  mptr->set_source(0);
  mptr->mutable_payload()->add_string("key2");

  ASSERT_TRUE(host_.HandleMessage(mptr.release()));

  // Reply message.
  mptr.reset(host_.PopOutgoingMessage());
  ASSERT_TRUE(mptr.get());
  ASSERT_EQ(MSG_SETTINGS_GET_ARRAY_VALUE, mptr->type());
  ASSERT_EQ(proto::Message::IS_REPLY, mptr->reply_mode());
  ASSERT_EQ(1, mptr->payload().string_size());
  ASSERT_EQ(1, mptr->payload().variable_size());
  EXPECT_EQ("key2", mptr->payload().string(0));
  EXPECT_EQ(proto::Variable::NONE, mptr->payload().variable(0).type());

  // Test invalid message.
  mptr.reset(NewMessage(MSG_SETTINGS_GET_ARRAY_VALUE, true));
  ASSERT_TRUE(host_.HandleMessage(mptr.release()));
  mptr.reset(host_.PopOutgoingMessage());
  ASSERT_TRUE(mptr.get());
  ASSERT_EQ(MSG_SETTINGS_GET_ARRAY_VALUE, mptr->type());
  ASSERT_EQ(proto::Message::IS_REPLY, mptr->reply_mode());
  ASSERT_TRUE(mptr->payload().has_error());
  ASSERT_EQ(proto::Error::INVALID_PAYLOAD, mptr->payload().error().code());

  mptr.reset(NewMessage(MSG_SETTINGS_GET_ARRAY_VALUE, true));
  mptr->mutable_payload()->add_string("key1");
  mptr->mutable_payload()->add_string("key2");
  ASSERT_TRUE(host_.HandleMessage(mptr.release()));
  mptr.reset(host_.PopOutgoingMessage());
  ASSERT_TRUE(mptr.get());
  ASSERT_EQ(MSG_SETTINGS_GET_ARRAY_VALUE, mptr->type());
  ASSERT_EQ(proto::Message::IS_REPLY, mptr->reply_mode());
  ASSERT_TRUE(mptr->payload().has_error());
  ASSERT_EQ(proto::Error::INVALID_PAYLOAD, mptr->payload().error().code());
}

}  // namespace components
}  // namespace ime_goopy
