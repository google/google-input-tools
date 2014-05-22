/*
  Copyright 2011 Google Inc.

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
#include "ggadget/scriptable_interface.h"
#include "ggadget/variant.h"
#include "unittest/gtest.h"

using namespace ggadget;

template <typename T>
void CheckType(Variant::Type t0) {
  Variant::Type t1 = VariantType<T>::type;
  ASSERT_EQ(t0, t1);
}

TEST(Variant, TestVoid) {
  Variant v;
  ASSERT_EQ(Variant::TYPE_VOID, v.type());
  CheckType<void>(Variant::TYPE_VOID);
  VariantValue<void>()(v);
  Variant v1(v);
  ASSERT_EQ(Variant::TYPE_VOID, v.type());
  printf("%s\n", v.Print().c_str());

  std::string s;
  ASSERT_TRUE(v.ConvertToString(&s));
  ASSERT_STREQ("", s.c_str());
  int i;
  ASSERT_FALSE(v.ConvertToInt(&i));
  bool b = true;
  ASSERT_TRUE(v.ConvertToBool(&b));
  ASSERT_FALSE(b);
}

template <typename T, Variant::Type Type>
void CheckVariant(T value, const char *str_value) {
  CheckType<T>(Type);
  Variant v(value);
  ASSERT_EQ(Type, v.type());
  ASSERT_TRUE(value == VariantValue<T>()(v));
  Variant v1(v);
  ASSERT_EQ(Type, v1.type());
  ASSERT_TRUE(value == VariantValue<T>()(v1));
  Variant v2;
  v2 = v;
  ASSERT_EQ(Type, v2.type());
  ASSERT_TRUE(value == VariantValue<T>()(v2));
  printf("%s\n", v.Print().c_str());

  std::string str;
  if (str_value) {
    ASSERT_TRUE(v.ConvertToString(&str));
    ASSERT_STREQ(str_value, str.c_str());
  } else {
    ASSERT_FALSE(v.ConvertToString(&str));
  }
}

void CheckBoolVariant(bool value, const char *str_value, int int_value) {
  CheckVariant<bool, Variant::TYPE_BOOL>(value, str_value);
  int i;
  ASSERT_TRUE(Variant(value).ConvertToInt(&i));
  ASSERT_EQ(i, int_value);
}

TEST(Variant, TestBool) {
  CheckBoolVariant(true, "true", 1);
  CheckBoolVariant(false, "false", 0);
}

template <typename T, Variant::Type Type>
void CheckNumberVariant(T value, const char *str_value) {
  CheckVariant<T, Type>(value, str_value);
  double d;
  ASSERT_TRUE(Variant(value).ConvertToDouble(&d));
  ASSERT_EQ(static_cast<double>(value), d);
  int i;
  ASSERT_TRUE(Variant(value).ConvertToInt(&i));
  ASSERT_EQ(static_cast<int>(value), i);
  int64_t i64;
  ASSERT_TRUE(Variant(value).ConvertToInt64(&i64));
  ASSERT_EQ(static_cast<int64_t>(value), i64);
  bool b;
  ASSERT_TRUE(Variant(value).ConvertToBool(&b));
  ASSERT_EQ(static_cast<bool>(value), b);
}

template <typename T>
void CheckIntVariant(T value, const char *str_value) {
  CheckNumberVariant<T, Variant::TYPE_INT64>(value, str_value);
}

enum {
  NONAME_1,
  NONAME_2,
};
enum NamedEnum {
  NAMED_1,
  NAMED_2,
};

TEST(Variant, TestInt) {
  Variant ve0(NONAME_2);
  ASSERT_EQ(Variant::TYPE_INT64, ve0.type());
  ASSERT_EQ(1, VariantValue<int>()(ve0));

  CheckIntVariant<NamedEnum>(NAMED_2, "1");
  CheckIntVariant<int>(1234, "1234");
  CheckIntVariant<unsigned int>(1234U, "1234");
  CheckIntVariant<char>('a', "97");
  CheckIntVariant<unsigned char>(0x20, "32");
  CheckIntVariant<short>(2345, "2345");
  CheckIntVariant<unsigned short>(3456, "3456");
  CheckIntVariant<long>(-4567890, "-4567890");
  CheckIntVariant<unsigned long>(5678901, "5678901");
  CheckIntVariant<int64_t>(INT64_C(0x1234567887654321), "1311768467139281697");
  // We don't support uint64_t bigger than 0x7fffffffffffffff for now.
  CheckIntVariant<uint64_t>(UINT64_C(0x7865432112345678),
                            "8675414066517530232");
}

template <typename T>
void CheckDoubleVariant(T value, const char *str_value) {
  CheckNumberVariant<T, Variant::TYPE_DOUBLE>(value, str_value);
}

TEST(Variant, TestDouble) {
#if defined(OS_WIN)
  // Windows will dispaly 3 digits for exponent.
  // See http://msdn.microsoft.com/en-us/library/hf4y5e3w(v=vs.71).aspx.
  CheckDoubleVariant<float>(-12345.6789e-20f, "-1.23457e-016");
  CheckDoubleVariant<float>(-12345.6789e+5f, "-1.23457e+009");
#elif defined(OS_POSIX)
  CheckDoubleVariant<float>(-12345.6789e-20f, "-1.23457e-16");
#endif
  CheckDoubleVariant<double>(30423.34932, "30423.3");
  CheckDoubleVariant<double>(0, "0");
}

template <typename T, typename VT, Variant::Type Type>
void CheckStringVariantBase(T value) {
  CheckType<T>(Type);
  Variant v(value);
  ASSERT_EQ(Type, v.type());
  ASSERT_TRUE(VT(value) == VT(VariantValue<T>()(v)));
  Variant v1(v);
  ASSERT_EQ(Type, v1.type());
  ASSERT_TRUE(VT(value) == VT(VariantValue<T>()(v1)));
  Variant v2;
  v2 = v;
  ASSERT_EQ(Type, v2.type());
  ASSERT_TRUE(VT(value) == VT(VariantValue<T>()(v2)));
  printf("%s\n", v.Print().c_str());
  Variant v3("1234");
  v3 = v;
  ASSERT_EQ(Type, v3.type());
  ASSERT_TRUE(VT(value) == VT(VariantValue<T>()(v3)));
}

template <typename T>
void CheckStringVariant(T value) {
  CheckStringVariantBase<T, std::string, Variant::TYPE_STRING>(value);
}

TEST(Variant, TestString) {
  CheckStringVariant<const char *>("abcdefg");
  CheckStringVariant<std::string>("xyz");
  CheckStringVariant<const std::string &>("120394");
  Variant v(static_cast<const char *>(NULL));
  ASSERT_EQ(std::string(""), VariantValue<std::string>()(v));
  ASSERT_TRUE(NULL == VariantValue<const char *>()(v));
  Variant v1(v);
  ASSERT_TRUE(NULL == VariantValue<const char *>()(v1));
  Variant v2;
  v2 = v;
  ASSERT_TRUE(NULL == VariantValue<const char *>()(v2));
  Variant v3("1234");
  v3 = v;
  ASSERT_TRUE(NULL == VariantValue<const char *>()(v3));

  double d;
  ASSERT_TRUE(v3.ConvertToDouble(&d));
  ASSERT_EQ(0, d);
  v3 = Variant("1234.6");
  ASSERT_TRUE(v3.ConvertToDouble(&d));
  ASSERT_EQ(1234.6, d);
  int i;
  ASSERT_TRUE(v3.ConvertToInt(&i));
  ASSERT_EQ(1235, i);
  bool b;
  ASSERT_FALSE(v3.ConvertToBool(&b));
  Variant v4("1234abc");
  ASSERT_FALSE(v4.ConvertToDouble(&d));
  ASSERT_FALSE(v4.ConvertToInt(&i));
  Variant v5("true");
  ASSERT_TRUE(v5.ConvertToBool(&b));
  ASSERT_TRUE(b);
  Variant v6("false");
  ASSERT_TRUE(v6.ConvertToBool(&b));
  ASSERT_FALSE(b);
  Variant v7("");
  ASSERT_TRUE(v7.ConvertToBool(&b));
  ASSERT_FALSE(b);
  Variant v8(static_cast<const char *>(NULL));
  ASSERT_TRUE(v8.ConvertToBool(&b));
  ASSERT_FALSE(b);
  std::string str;
  ASSERT_TRUE(v8.ConvertToString(&str));
  ASSERT_STREQ("", str.c_str());
  ASSERT_TRUE(v6.ConvertToString(&str));
  ASSERT_STREQ(VariantValue<const char *>()(v6), str.c_str());

  Variant v9(" 123.4  ");
  ASSERT_TRUE(v9.ConvertToDouble(&d));
  ASSERT_EQ(123.4, d);

  Variant v10(" 1234  ");
  ASSERT_TRUE(v10.ConvertToInt(&i));
  ASSERT_EQ(1234, i);

  Variant v11(" 1234 abc");
  ASSERT_FALSE(v11.ConvertToInt(&i));
}

TEST(Variant, TestJSON) {
  CheckVariant<JSONString, Variant::TYPE_JSON>(JSONString(std::string("abc")),
                                               NULL);
  CheckVariant<const JSONString &, Variant::TYPE_JSON>(JSONString("def"),
                                                       NULL);
}

template <typename T>
void CheckUTF16StringVariant(T value) {
  CheckStringVariantBase<T, UTF16String, Variant::TYPE_UTF16STRING>(value);
}

TEST(Variant, TestUTF16String) {
  UTF16Char p[] = { 100, 200, 300, 400, 500, 0 };
  CheckUTF16StringVariant<const UTF16Char *>(p);
  CheckUTF16StringVariant<UTF16String>(UTF16String(p));
  CheckUTF16StringVariant<const UTF16String &>(UTF16String(p));
}

class Scriptable1 : public ScriptableInterface { };

TEST(Variant, TestScriptable) {
  CheckVariant<ScriptableInterface *, Variant::TYPE_SCRIPTABLE>(NULL, NULL);
  CheckVariant<Scriptable1 *, Variant::TYPE_SCRIPTABLE>(NULL, NULL);
}

TEST(Variant, TestDate) {
  CheckVariant<Date, Variant::TYPE_DATE>(Date(1234), NULL);
  CheckVariant<const Date &, Variant::TYPE_DATE>(Date(1234), NULL);
}

TEST(Variant, TestSlot) {
  CheckVariant<Slot *, Variant::TYPE_SLOT>(NULL, NULL);
}

TEST(Variant, TestAny) {
  CheckVariant<void *, Variant::TYPE_ANY>(NULL, NULL);
  CheckVariant<const void *, Variant::TYPE_CONST_ANY>(NULL, NULL);
}

int main(int argc, char **argv) {
  testing::ParseGTestFlags(&argc, argv);
  return RUN_ALL_TESTS();
}
