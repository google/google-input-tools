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

#include <set>
#include <stdio.h>
#include <stdlib.h>
#include <dbus/dbus.h>
#include <limits>

#include "ggadget/dbus/dbus_proxy.h"
#include "ggadget/dbus/dbus_utils.h"
#include "ggadget/scriptable_array.h"
#include "ggadget/logger.h"
#include "unittest/gtest.h"

using namespace ggadget;
using namespace ggadget::dbus;

namespace {

DBusMessage* GetMarshalledMessage(const char *signature,
                                  const Variant &value) {
  DBusMessage *message = dbus_message_new_method_call("org.freedesktop.DBus",
                                                      "/org/freedesktop/DBus",
                                                      "org.freedesktop.DBus",
                                                      "Hello");
  if (!signature) return message;

  DBusMarshaller marshaller(message);
  Argument arg(signature, value);
  EXPECT_TRUE(marshaller.AppendArgument(arg));
  return message;
}

template <typename T>
void TestBasicMarshal(const char *signature, int dbus_type,
                      const Variant &value, const T &check_value) {
  DLOG("Testing basic type: %s", signature);
  DBusMessage *message = GetMarshalledMessage(signature, value);
  DBusMessageIter iter;
  dbus_message_iter_init(message, &iter);
  int real_type = dbus_message_iter_get_arg_type(&iter);
  T v = 0;
  dbus_message_iter_get_basic(&iter, &v);
  EXPECT_EQ(check_value, v);
  EXPECT_EQ(dbus_type, real_type);
  EXPECT_FALSE(dbus_message_iter_has_next(&iter));
  dbus_message_unref(message);
}

template <>
void TestBasicMarshal<std::string>(const char *signature, int dbus_type,
                                   const Variant &value,
                                   const std::string &check_value) {
  DBusMessage *message = GetMarshalledMessage(signature, value);
  DBusMessageIter iter;
  dbus_message_iter_init(message, &iter);
  int real_type = dbus_message_iter_get_arg_type(&iter);
  char *v = 0;
  dbus_message_iter_get_basic(&iter, &v);
  EXPECT_STREQ(check_value.c_str(), v);
  EXPECT_EQ(dbus_type, real_type);
  EXPECT_FALSE(dbus_message_iter_has_next(&iter));
  dbus_message_unref(message);
}

template <typename T>
ResultVariant GenerateVariantArray(size_t size, T first, T diff) {
  ScriptableArray *array = new ScriptableArray();
  T value = first;
  for (size_t i = 0; i < size; ++i) {
    array->Append(Variant(value));
    value = value + diff;
  }
  return ResultVariant(Variant(array));
}

template <typename P1, typename P2>
ResultVariant GenerateVariantDict(size_t size, P1 key, P1 key_diff,
                                  P2 value, P2 value_diff) {
  ScriptableDBusContainer *obj = new ScriptableDBusContainer;
  P1 k = key;
  P2 v = value;
  for (size_t i = 0; i < size; ++i) {
    Variant vk(k);
    Variant vv(v);
    std::string key;
    EXPECT_TRUE(vk.ConvertToString(&key));
    obj->AddProperty(key.c_str(), vv);
    k = k + key_diff;
    v = v + value_diff;
  }
  return ResultVariant(Variant(obj));
}

ResultVariant GenerateVariantStruct(size_t size, ...) {
  ScriptableArray *array = new ScriptableArray();
  va_list args;
  va_start(args, size);
  for (size_t i = 0; i < size; ++i) {
    Variant *v;
    v = va_arg(args, Variant*);
    array->Append(*v);
  }
  va_end(args);
  return ResultVariant(Variant(array));
}

void TestArrayMarshal() {
  DLOG("Testing Array Marshalling...");
  const size_t vector_size = 10;
  ResultVariant v = GenerateVariantArray<uint64_t>(vector_size, 12, 4);
  DBusMessage *message = GetMarshalledMessage("at", v.v());
  /* begin to demarshal the message. */
  DBusMessageIter iter, subiter;
  dbus_message_iter_init(message, &iter);
  char *sig = dbus_message_iter_get_signature(&iter);
  EXPECT_STREQ("at", sig);
  dbus_free(sig);
  EXPECT_FALSE(dbus_message_iter_has_next(&iter));
  EXPECT_EQ(DBUS_TYPE_ARRAY, dbus_message_iter_get_arg_type(&iter));
  dbus_message_iter_recurse(&iter, &subiter);
  for (size_t i = 0; i < vector_size; ++i) {
    uint64_t v;
    dbus_message_iter_get_basic(&subiter, &v);
    EXPECT_EQ(12 + i * 4, v);
    dbus_message_iter_next(&subiter);
  }
  EXPECT_FALSE(dbus_message_iter_has_next(&subiter));
  dbus_message_unref(message);
  /* testing marshal without explicit signature. */
  message = GetMarshalledMessage("", v.v());
  dbus_message_iter_init(message, &iter);
  sig = dbus_message_iter_get_signature(&iter);
  EXPECT_STREQ("ai", sig);
  dbus_free(sig);
  dbus_message_unref(message);
}

/* add a operator to Variant so that we can use the template function. */
Variant operator+(const Variant &first, const Variant &second) {
  return first;
}

void TestStructMarshal() {
  DLOG("Testing Struct Marshalling...");
  Variant v1("Gadget"), v2(64), v3(true);
  ResultVariant vs = GenerateVariantStruct(3, &v1, &v2, &v3);
  DBusMessage *message = GetMarshalledMessage("(sub)", vs.v());
  DBusMessageIter iter, subiter;
  dbus_message_iter_init(message, &iter);
  EXPECT_EQ(DBUS_TYPE_STRUCT, dbus_message_iter_get_arg_type(&iter));
  EXPECT_FALSE(dbus_message_iter_has_next(&iter));
  char *sig = dbus_message_iter_get_signature(&iter);
  EXPECT_STREQ("(sub)", sig);
  dbus_free(sig);
  dbus_message_iter_recurse(&iter, &subiter);
  char *str;
  dbus_uint32_t num;
  dbus_bool_t yes;
  EXPECT_EQ(DBUS_TYPE_STRING, dbus_message_iter_get_arg_type(&subiter));
  dbus_message_iter_get_basic(&subiter, &str);
  EXPECT_STREQ("Gadget", str);
  EXPECT_TRUE(dbus_message_iter_next(&subiter));
  EXPECT_EQ(DBUS_TYPE_UINT32, dbus_message_iter_get_arg_type(&subiter));
  dbus_message_iter_get_basic(&subiter, &num);
  EXPECT_EQ(64u, num);
  EXPECT_TRUE(dbus_message_iter_next(&subiter));
  EXPECT_EQ(DBUS_TYPE_BOOLEAN, dbus_message_iter_get_arg_type(&subiter));
  dbus_message_iter_get_basic(&subiter, &yes);
  EXPECT_TRUE(yes);
  EXPECT_FALSE(dbus_message_iter_has_next(&subiter));
  dbus_message_unref(message);
  /* testing marshal without explicit signature. */
  message = GetMarshalledMessage("", vs.v());
  dbus_message_iter_init(message, &iter);
  sig = dbus_message_iter_get_signature(&iter);
  EXPECT_STREQ("(sib)", sig);
  dbus_free(sig);
  dbus_message_unref(message);
}

void TestDictMarshal() {
  DLOG("Testing Dict Marshalling...");
  const size_t dict_size = 10;
  ResultVariant v = GenerateVariantDict(dict_size, 123, 3, 256, 9);
  DBusMessage *message = GetMarshalledMessage("a{it}", v.v());
  DBusMessageIter iter, item_iter, dict_iter;
  dbus_message_iter_init(message, &iter);
  char *sig = dbus_message_iter_get_signature(&iter);
  EXPECT_STREQ("a{it}", sig);
  dbus_free(sig);
  EXPECT_EQ(DBUS_TYPE_ARRAY, dbus_message_iter_get_arg_type(&iter));
  EXPECT_FALSE(dbus_message_iter_has_next(&iter));
  dbus_message_iter_recurse(&iter, &item_iter);
  for (size_t i = 0; i < dict_size; ++i) {
    EXPECT_EQ(DBUS_TYPE_DICT_ENTRY, dbus_message_iter_get_arg_type(&item_iter));
    dbus_message_iter_recurse(&item_iter, &dict_iter);
    int key;
    uint64_t value;
    dbus_message_iter_get_basic(&dict_iter, &key);
    EXPECT_EQ(static_cast<int>(123 + 3 * i), key);
    EXPECT_TRUE(dbus_message_iter_next(&dict_iter));
    dbus_message_iter_get_basic(&dict_iter, &value);
    EXPECT_EQ(256 + i * 9, value);
    EXPECT_FALSE(dbus_message_iter_has_next(&dict_iter));
    dbus_message_iter_next(&item_iter);
  }
  EXPECT_FALSE(dbus_message_iter_has_next(&item_iter));
  dbus_message_unref(message);
  /* testing marshal without explicit signature. */
  message = GetMarshalledMessage("", v.v());
  dbus_message_iter_init(message, &iter);
  sig = dbus_message_iter_get_signature(&iter);
  EXPECT_STREQ("a{si}", sig);
  dbus_free(sig);
  dbus_message_unref(message);
}

void TestVariantMarshal() {
  DLOG("Testing Variant Marshalling...");
  DBusMessageIter iter, itemiter, arrayiter, subiter;
  /* testing basic types */
  DBusMessage *message = GetMarshalledMessage("v", Variant(false));
  dbus_message_iter_init(message, &iter);
  EXPECT_EQ(DBUS_TYPE_VARIANT, dbus_message_iter_get_arg_type(&iter));
  char *sig = dbus_message_iter_get_signature(&iter);
  EXPECT_STREQ("v", sig);
  dbus_free(sig);
  dbus_message_iter_recurse(&iter, &itemiter);
  sig = dbus_message_iter_get_signature(&itemiter);
  EXPECT_STREQ("b", sig);
  dbus_free(sig);
  bool yes = true;
  dbus_message_iter_get_basic(&itemiter, &yes);
  EXPECT_FALSE(yes);
  dbus_message_unref(message);
  message = GetMarshalledMessage("v", Variant(3.141592653589793));
  dbus_message_iter_init(message, &iter);
  EXPECT_EQ(DBUS_TYPE_VARIANT, dbus_message_iter_get_arg_type(&iter));
  sig = dbus_message_iter_get_signature(&iter);
  EXPECT_STREQ("v", sig);
  dbus_free(sig);
  dbus_message_iter_recurse(&iter, &itemiter);
  sig = dbus_message_iter_get_signature(&itemiter);
  EXPECT_STREQ("d", sig);
  dbus_free(sig);
  dbus_message_unref(message);
  message = GetMarshalledMessage("v", Variant("variant"));
  dbus_message_iter_init(message, &iter);
  EXPECT_EQ(DBUS_TYPE_VARIANT, dbus_message_iter_get_arg_type(&iter));
  sig = dbus_message_iter_get_signature(&iter);
  EXPECT_STREQ("v", sig);
  dbus_free(sig);
  dbus_message_iter_recurse(&iter, &itemiter);
  sig = dbus_message_iter_get_signature(&itemiter);
  EXPECT_STREQ("s", sig);
  dbus_free(sig);
  char *str;
  dbus_message_iter_get_basic(&itemiter, &str);
  EXPECT_STREQ("variant", str);
  dbus_message_unref(message);

  /* testing variant as an array container. */
  ResultVariant array = GenerateVariantArray<uint64_t>(4, 5, 39);
  message = GetMarshalledMessage("v", array.v());
  dbus_message_iter_init(message, &iter);
  EXPECT_EQ(DBUS_TYPE_VARIANT, dbus_message_iter_get_arg_type(&iter));
  sig = dbus_message_iter_get_signature(&iter);
  EXPECT_STREQ("v", sig);
  dbus_free(sig);
  EXPECT_FALSE(dbus_message_iter_has_next(&iter));
  dbus_message_iter_recurse(&iter, &itemiter);
  sig = dbus_message_iter_get_signature(&itemiter);
  EXPECT_STREQ("ai", sig);
  dbus_free(sig);
  EXPECT_FALSE(dbus_message_iter_has_next(&itemiter));
  dbus_message_iter_recurse(&itemiter, &subiter);
  for (int i = 0; i < 4; ++i) {
    int v;
    dbus_message_iter_get_basic(&subiter, &v);
    EXPECT_EQ(5 + 39 * i, v);
    dbus_message_iter_next(&subiter);
  }
  EXPECT_FALSE(dbus_message_iter_has_next(&subiter));
  dbus_message_unref(message);
  Variant v1("Gadget"), v2(64), v3(true);
  ResultVariant structure = GenerateVariantStruct(3, &v1, &v2, &v3);

  /* testing variant as a structure container. */
  message = GetMarshalledMessage("v", structure.v());
  dbus_message_iter_init(message, &iter);
  EXPECT_EQ(DBUS_TYPE_VARIANT, dbus_message_iter_get_arg_type(&iter));
  sig = dbus_message_iter_get_signature(&iter);
  EXPECT_STREQ("v", sig);
  dbus_free(sig);
  EXPECT_FALSE(dbus_message_iter_has_next(&iter));
  dbus_message_iter_recurse(&iter, &itemiter);
  sig = dbus_message_iter_get_signature(&itemiter);
  EXPECT_STREQ("(sib)", sig);
  dbus_free(sig);
  EXPECT_FALSE(dbus_message_iter_has_next(&itemiter));
  dbus_message_iter_recurse(&itemiter, &subiter);
  dbus_message_iter_get_basic(&subiter, &str);
  EXPECT_STREQ("Gadget", str);
  EXPECT_TRUE(dbus_message_iter_next(&subiter));
  int32_t v;
  dbus_message_iter_get_basic(&subiter, &v);
  EXPECT_EQ(64, v);
  EXPECT_TRUE(dbus_message_iter_next(&subiter));
  dbus_message_iter_get_basic(&subiter, &yes);
  EXPECT_TRUE(yes);
  EXPECT_FALSE(dbus_message_iter_has_next(&subiter));
  dbus_message_unref(message);

  /* testing variant as a dict container. */
  ResultVariant dict = GenerateVariantDict(10, 123, 3, 256, 9);
  message = GetMarshalledMessage("v", dict.v());
  dbus_message_iter_init(message, &iter);
  EXPECT_EQ(DBUS_TYPE_VARIANT, dbus_message_iter_get_arg_type(&iter));
  sig = dbus_message_iter_get_signature(&iter);
  EXPECT_STREQ("v", sig);
  dbus_free(sig);
  EXPECT_FALSE(dbus_message_iter_has_next(&iter));
  dbus_message_iter_recurse(&iter, &arrayiter);
  sig = dbus_message_iter_get_signature(&arrayiter);
  EXPECT_STREQ("a{si}", sig);
  dbus_free(sig);
  EXPECT_FALSE(dbus_message_iter_has_next(&arrayiter));
  dbus_message_iter_recurse(&arrayiter, &itemiter);
  for (int i = 0; i < 10; ++i) {
    dbus_message_iter_recurse(&itemiter, &subiter);
    char *key;
    int v;
    dbus_message_iter_get_basic(&subiter, &key);
    EXPECT_EQ(123 + 3 * i, atoi(key));
    EXPECT_TRUE(dbus_message_iter_next(&subiter));
    dbus_message_iter_get_basic(&subiter, &v);
    EXPECT_EQ(256 + 9 * i, v);
    EXPECT_FALSE(dbus_message_iter_next(&subiter));
    dbus_message_iter_next(&itemiter);
  }
  EXPECT_FALSE(dbus_message_iter_has_next(&itemiter));
  dbus_message_unref(message);
}

bool IsEqual(const Variant &v1, const Variant &v2);
class ScriptableIterator {
 public:
  bool operator==(const ScriptableIterator &another) const {
    if (array_.size() != another.array_.size() ||
        properties_.size() != another.properties_.size()) {
      DLOG("size mismatch, array: %zu:%zu; property: %zu:%zu.",
           array_.size(), another.array_.size(),
           properties_.size(), another.properties_.size());
      return false;
    }
    for (size_t i = 0; i < array_.size(); ++i)
      if (!IsEqual(array_[i], another.array_[i])) {
        DLOG("index %zu mismatch: %s, %s", i, array_[i].Print().c_str(),
             another.array_[i].Print().c_str());
        return false;
      }
    for (PropertyMap::const_iterator it = properties_.begin();
         it != properties_.end(); ++it) {
      PropertyMap::const_iterator second = another.properties_.find(it->first);
      if (second == another.properties_.end()) {
        DLOG("property %s does not exist in another", it->first.c_str());
        return false;
      }
      if (!IsEqual(it->second, second->second)) {
        DLOG("property %s mismatch: %s, %s", it->first.c_str(),
             it->second.Print().c_str(), second->second.Print().c_str());
        return false;
      }
    }
    return true;
  }
  bool EnumerateArray(int id, const Variant &value) {
    array_.push_back(value);
    return true;
  }
  bool EnumerateProperties(const char *name,
                           ScriptableInterface::PropertyType type,
                           const Variant &value) {
    if (type != ScriptableInterface::PROPERTY_METHOD)
      properties_[name] = value;
    return true;
  }
 private:
  std::vector<Variant> array_;
  typedef std::map<std::string, Variant> PropertyMap;
  PropertyMap properties_;
};

bool IsEqual(ScriptableInterface *s1, ScriptableInterface *s2) {
  ScriptableIterator it1, it2;
  s1->EnumerateElements(NewSlot(&it1, &ScriptableIterator::EnumerateArray));
  s1->EnumerateProperties(NewSlot(&it1,
                                  &ScriptableIterator::EnumerateProperties));
  s2->EnumerateElements(NewSlot(&it2, &ScriptableIterator::EnumerateArray));
  s2->EnumerateProperties(NewSlot(&it2,
                                  &ScriptableIterator::EnumerateProperties));
  return it1 == it2;
}

bool IsEqual(const Variant &v1, const Variant &v2) {
  if (v1.type() != Variant::TYPE_SCRIPTABLE) {
    bool ret = (v1 == v2);
    if (!ret) DLOG("simple type mismatch: %s, %s",
                   v1.Print().c_str(), v2.Print().c_str());
    return ret;
  }
  if (v2.type() != Variant::TYPE_SCRIPTABLE) {
    DLOG("type mismatch. one is not scriptable but the other is.");
    return false;
  }
  return IsEqual(VariantValue<ScriptableInterface*>()(v1),
                VariantValue<ScriptableInterface*>()(v2));
}

void TestBasicDemarshal(const char *signature,
                        const char *expect_signature, const Variant &value) {
  DLOG("Test Demarshal for signature: %s", expect_signature);
  DBusMessage *message = GetMarshalledMessage(signature, value);
  Argument arg(expect_signature);
  DBusDemarshaller demarshaller(message);
  EXPECT_TRUE(demarshaller.GetArgument(&arg));
  EXPECT_TRUE(IsEqual(value, arg.value.v()));
  dbus_message_unref(message);
}

void TestContainerDemarshal() {
  ResultVariant array = GenerateVariantArray<uint64_t>(4, 5, 39);
  TestBasicDemarshal("at", "at", array.v());
  TestBasicDemarshal("v", "v", array.v());
  Variant v1("Gadget"), v2(64), v3(true);
  ResultVariant structure = GenerateVariantStruct(3, &v1, &v2, &v3);
  TestBasicDemarshal("", "(sib)", structure.v());
  TestBasicDemarshal("v", "v", structure.v());
  ResultVariant dict = GenerateVariantDict(10, 123, 3, 256, 9);
  TestBasicDemarshal("a{yu}", "a{yu}", dict.v());
  TestBasicDemarshal("v", "v", dict.v());
  ResultVariant empty_dict = GenerateVariantDict(0, 0, 0, 0, 0);
  TestBasicDemarshal("a{sv}", "a{sv}", empty_dict.v());
}

template <typename T>
void TestBasicValistMarshal(const char *signature, const T &value,
                            MessageType first_arg_type, ...) {
  Arguments args;
  va_list va_args;
  va_start(va_args, first_arg_type);
  EXPECT_TRUE(DBusMarshaller::ValistAdaptor(&args, first_arg_type, &va_args));
  DBusMessage *message = dbus_message_new_method_call("org.freedesktop.DBus",
                                                      "/org/freedesktop/DBus",
                                                      "org.freedesktop.DBus",
                                                      "Hello");
  DBusMarshaller marshaller(message);
  EXPECT_TRUE(marshaller.AppendArguments(args));
  DBusMessageIter iter;
  dbus_message_iter_init(message, &iter);
  char *sig = dbus_message_iter_get_signature(&iter);
  EXPECT_STREQ(signature, sig);
  dbus_free(sig);
  T v;
  dbus_message_iter_get_basic(&iter, &v);
  EXPECT_EQ(value, v);
  dbus_message_unref(message);
}

template <>
void TestBasicValistMarshal<std::string>(const char *signature,
                                         const std::string &value,
                                         MessageType first_arg_type, ...) {
  Arguments args;
  va_list va_args;
  va_start(va_args, first_arg_type);
  EXPECT_TRUE(DBusMarshaller::ValistAdaptor(&args, first_arg_type, &va_args));
  DBusMessage *message = dbus_message_new_method_call("org.freedesktop.DBus",
                                                      "/org/freedesktop/DBus",
                                                      "org.freedesktop.DBus",
                                                      "Hello");
  DBusMarshaller marshaller(message);
  EXPECT_TRUE(marshaller.AppendArguments(args));
  DBusMessageIter iter;
  dbus_message_iter_init(message, &iter);
  char *sig = dbus_message_iter_get_signature(&iter);
  EXPECT_STREQ(signature, sig);
  dbus_free(sig);
  char *v;
  dbus_message_iter_get_basic(&iter, &v);
  EXPECT_STREQ(value.c_str(), v);
  dbus_message_unref(message);
}

void TestConvertedVariant(const Variant &value, const char *signature,
                          MessageType first_arg_type, ...) {
  DLOG("Test for valist signature: %s", signature);
  va_list va_args;
  va_start(va_args, first_arg_type);
  Arguments args;
  EXPECT_TRUE(DBusMarshaller::ValistAdaptor(&args, first_arg_type, &va_args));
  EXPECT_EQ(1u, args.size());
  EXPECT_TRUE(IsEqual(value, args[0].value.v()));
  EXPECT_STREQ(signature, args[0].signature.c_str());
}

void TestContainerValistMarshal() {
  ResultVariant array = GenerateVariantArray<std::string>(2, "AA", "a");
  TestConvertedVariant(array.v(), "as",
                       MESSAGE_TYPE_ARRAY, 2,
                       MESSAGE_TYPE_STRING, "AA",
                       MESSAGE_TYPE_STRING, "AAa",
                       MESSAGE_TYPE_INVALID);
  TestConvertedVariant(array.v(), "v", MESSAGE_TYPE_VARIANT,
                       MESSAGE_TYPE_ARRAY, 2,
                       MESSAGE_TYPE_STRING, "AA",
                       MESSAGE_TYPE_STRING, "AAa",
                       MESSAGE_TYPE_INVALID);
  Variant v1("Gadget"), v2(64), v3(true);
  ResultVariant structure = GenerateVariantStruct(3, &v1, &v2, &v3);
  TestConvertedVariant(structure.v(), "(syb)",
                       MESSAGE_TYPE_STRUCT, 3,
                       MESSAGE_TYPE_STRING, "Gadget",
                       MESSAGE_TYPE_BYTE, 64,
                       MESSAGE_TYPE_BOOLEAN, true,
                       MESSAGE_TYPE_INVALID);
  TestConvertedVariant(structure.v(), "v", MESSAGE_TYPE_VARIANT,
                       MESSAGE_TYPE_STRUCT, 3,
                       MESSAGE_TYPE_STRING, "Gadget",
                       MESSAGE_TYPE_BYTE, 64,
                       MESSAGE_TYPE_BOOLEAN, true,
                       MESSAGE_TYPE_INVALID);
  ResultVariant dict = GenerateVariantDict(2, 123, 3, 256, 9);
  TestConvertedVariant(dict.v(), "a{iu}",
                       MESSAGE_TYPE_DICT, 2,
                       MESSAGE_TYPE_INT32, 123,
                       MESSAGE_TYPE_UINT32, 256,
                       MESSAGE_TYPE_INT32, 126,
                       MESSAGE_TYPE_UINT32, 265,
                       MESSAGE_TYPE_INVALID);
  TestConvertedVariant(dict.v(), "v", MESSAGE_TYPE_VARIANT,
                       MESSAGE_TYPE_DICT, 2,
                       MESSAGE_TYPE_INT32, 123,
                       MESSAGE_TYPE_UINT32, 256,
                       MESSAGE_TYPE_INT32, 126,
                       MESSAGE_TYPE_UINT32, 265,
                       MESSAGE_TYPE_INVALID);
}

void TestValistDemarshal(MessageType first_arg_type, ...) {
  DLOG("Test Valist Demarshal. First arg: %d", first_arg_type);
  va_list va_args;
  Arguments in_args, out_args;
  va_start(va_args, first_arg_type);
  EXPECT_TRUE(DBusMarshaller::ValistAdaptor(&out_args, first_arg_type,
                                            &va_args));
  first_arg_type = static_cast<MessageType>(va_arg(va_args, int));
  EXPECT_TRUE(DBusDemarshaller::ValistAdaptor(out_args, first_arg_type,
                                              &va_args));
  va_end(va_args);
}

}  // anonymous namespace

TEST(DBusMarshaller, AppendArgument) {
  TestBasicMarshal<unsigned char>("y", DBUS_TYPE_BYTE, Variant(true), 1);
  TestBasicMarshal<dbus_bool_t>("b", DBUS_TYPE_BOOLEAN, Variant(true), true);
  TestBasicMarshal<int16_t>("n", DBUS_TYPE_INT16, Variant("-1"), -1);
  TestBasicMarshal<uint16_t>("q", DBUS_TYPE_UINT16, Variant("1"), 1);
  TestBasicMarshal<int32_t>("i", DBUS_TYPE_INT32, Variant(true), 1);
  TestBasicMarshal<uint32_t>("u", DBUS_TYPE_UINT32, Variant(true), 1);
  TestBasicMarshal<int64_t>("x", DBUS_TYPE_INT64, Variant(true), 1);
  TestBasicMarshal<uint64_t>("t", DBUS_TYPE_UINT64, Variant(true), 1);
  TestBasicMarshal<double>("d", DBUS_TYPE_DOUBLE, Variant("0"), 0);
  TestBasicMarshal<std::string>("s", DBUS_TYPE_STRING, Variant(true), "true");
  TestArrayMarshal();
  TestStructMarshal();
  TestDictMarshal();
  TestVariantMarshal();
}

TEST(DBusMarshaller, ValistAdaptor) {
  TestBasicValistMarshal<unsigned char>("y", 142, MESSAGE_TYPE_BYTE, 142,
                                        MESSAGE_TYPE_INVALID);
  TestBasicValistMarshal<dbus_bool_t>("b", true, MESSAGE_TYPE_BOOLEAN, true,
                         MESSAGE_TYPE_INVALID);
  TestBasicValistMarshal("n", std::numeric_limits<int16_t>::min(),
                         MESSAGE_TYPE_INT16,
                         std::numeric_limits<int16_t>::min(),
                         MESSAGE_TYPE_INVALID);
  TestBasicValistMarshal("q", std::numeric_limits<uint16_t>::max(),
                         MESSAGE_TYPE_UINT16,
                         std::numeric_limits<uint16_t>::max(),
                         MESSAGE_TYPE_INVALID);
  TestBasicValistMarshal("i", std::numeric_limits<int32_t>::min(),
                         MESSAGE_TYPE_INT32,
                         std::numeric_limits<int32_t>::min(),
                         MESSAGE_TYPE_INVALID);
  TestBasicValistMarshal("u", std::numeric_limits<int32_t>::max(),
                         MESSAGE_TYPE_UINT32,
                         std::numeric_limits<int32_t>::max(),
                         MESSAGE_TYPE_INVALID);
  TestBasicValistMarshal("x", std::numeric_limits<int64_t>::min(),
                         MESSAGE_TYPE_INT64,
                         std::numeric_limits<int64_t>::min(),
                         MESSAGE_TYPE_INVALID);
  TestBasicValistMarshal("t", std::numeric_limits<uint64_t>::max(),
                         MESSAGE_TYPE_UINT64,
                         std::numeric_limits<uint64_t>::max(),
                         MESSAGE_TYPE_INVALID);
  TestBasicValistMarshal("d", 3.1415927, MESSAGE_TYPE_DOUBLE,
                         3.1415927, MESSAGE_TYPE_INVALID);
  TestBasicValistMarshal<std::string>("s", "3.1415927", MESSAGE_TYPE_STRING,
                                      "3.1415927", MESSAGE_TYPE_INVALID);
  TestContainerValistMarshal();
}

TEST(DBusDemarshaller, GetArgument) {
  TestBasicDemarshal("y", "y",
                     Variant(std::numeric_limits<unsigned char>::max()));
  TestBasicDemarshal("", "i",
                     Variant(std::numeric_limits<unsigned char>::max()));
  TestBasicDemarshal("v", "v",
                     Variant(std::numeric_limits<unsigned char>::max()));

  TestBasicDemarshal("b", "b", Variant(false));
  TestBasicDemarshal("v", "v", Variant(false));
  TestBasicDemarshal("", "b", Variant(false));

  TestBasicDemarshal("n", "n", Variant(std::numeric_limits<int16_t>::min()));
  TestBasicDemarshal("", "i", Variant(std::numeric_limits<int16_t>::min()));
  TestBasicDemarshal("q", "q", Variant(std::numeric_limits<uint16_t>::max()));
  TestBasicDemarshal("", "i", Variant(std::numeric_limits<uint16_t>::max()));
  TestBasicDemarshal("i", "i", Variant(std::numeric_limits<int32_t>::min()));
  TestBasicDemarshal("", "i", Variant(std::numeric_limits<int32_t>::min()));
  TestBasicDemarshal("u", "u", Variant(std::numeric_limits<uint32_t>::max()));
  TestBasicDemarshal("", "i", Variant(25));
  TestBasicDemarshal("x", "x", Variant(std::numeric_limits<int64_t>::min()));
  TestBasicDemarshal("", "i", Variant(-123));
  TestBasicDemarshal("t", "t", Variant(std::numeric_limits<uint64_t>::max()));
  TestBasicDemarshal("", "i", Variant(321));

  TestBasicDemarshal("d", "d", Variant(3.1415926));
  TestBasicDemarshal("", "d", Variant(3.1415926));
  TestBasicDemarshal("v", "v", Variant(3.1415926));

  TestBasicDemarshal("s", "s", Variant("Google Gadget for Linux"));
  TestBasicDemarshal("", "s", Variant("Google Gadget for Linux"));
  TestBasicDemarshal("v", "v", Variant("Google Gadget for Linux"));
  TestContainerDemarshal();
}

TEST(DBusDemarshaller, ValistAdaptor) {
  unsigned char uc;
  TestValistDemarshal(MESSAGE_TYPE_BYTE, 21, MESSAGE_TYPE_INVALID,
                      MESSAGE_TYPE_BYTE, &uc, MESSAGE_TYPE_INVALID);
  EXPECT_EQ(21u, uc);
  bool yes = false;
  TestValistDemarshal(MESSAGE_TYPE_BOOLEAN, true, MESSAGE_TYPE_INVALID,
                      MESSAGE_TYPE_BOOLEAN, &yes, MESSAGE_TYPE_INVALID);
  EXPECT_TRUE(yes);
  int16_t int16 = 0;
  TestValistDemarshal(MESSAGE_TYPE_INT16, -423, MESSAGE_TYPE_INVALID,
                      MESSAGE_TYPE_INT16, &int16, MESSAGE_TYPE_INVALID);
  EXPECT_EQ(-423, int16);
  uint16_t uint16 = 0;
  TestValistDemarshal(MESSAGE_TYPE_UINT16, 987, MESSAGE_TYPE_INVALID,
                      MESSAGE_TYPE_UINT16, &uint16, MESSAGE_TYPE_INVALID);
  EXPECT_EQ(987u, uint16);
  int32_t int32 = 0;
  TestValistDemarshal(MESSAGE_TYPE_INT32, -42332, MESSAGE_TYPE_INVALID,
                      MESSAGE_TYPE_INT32, &int32, MESSAGE_TYPE_INVALID);
  EXPECT_EQ(-42332, int32);
  uint32_t uint32 = 0;
  TestValistDemarshal(MESSAGE_TYPE_UINT32, 4256, MESSAGE_TYPE_INVALID,
                      MESSAGE_TYPE_UINT32, &uint32, MESSAGE_TYPE_INVALID);
  EXPECT_EQ(4256u, uint32);
  int64_t int64 = 0;
  TestValistDemarshal(MESSAGE_TYPE_INT64, 98787866LL, MESSAGE_TYPE_INVALID,
                      MESSAGE_TYPE_INT64, &int64, MESSAGE_TYPE_INVALID);
  EXPECT_EQ(98787866LL, int64);
  uint64_t uint64 = 0;
  TestValistDemarshal(MESSAGE_TYPE_UINT64, 1234567890ULL, MESSAGE_TYPE_INVALID,
                      MESSAGE_TYPE_UINT64, &uint64, MESSAGE_TYPE_INVALID);
  EXPECT_EQ(1234567890u, uint64);
  double d = 0.0;
  TestValistDemarshal(MESSAGE_TYPE_DOUBLE, 1.41421, MESSAGE_TYPE_INVALID,
                      MESSAGE_TYPE_DOUBLE, &d, MESSAGE_TYPE_INVALID);
  EXPECT_EQ(1.41421, d);
  char *str = NULL;
  TestValistDemarshal(MESSAGE_TYPE_STRING, "e", MESSAGE_TYPE_INVALID,
                      MESSAGE_TYPE_STRING, &str, MESSAGE_TYPE_INVALID);
  EXPECT_STREQ("e", str);
  delete str;
  TestValistDemarshal(MESSAGE_TYPE_STRING, "pi ~= ",
                      MESSAGE_TYPE_DOUBLE, 3.1415926, MESSAGE_TYPE_INVALID,
                      MESSAGE_TYPE_STRING, &str,
                      MESSAGE_TYPE_DOUBLE, &d, MESSAGE_TYPE_INVALID);
  EXPECT_EQ(3.1415926, d);
  EXPECT_STREQ("pi ~= ", str);
  delete str;
}

int main(int argc, char **argv) {
  testing::ParseGTestFlags(&argc, argv);
  int result = RUN_ALL_TESTS();
  return result;
}
