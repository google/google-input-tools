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

#ifndef GGADGET_TESTS_SLOTS_H__
#define GGADGET_TESTS_SLOTS_H__

#include <vector>
#include <ggadget/scriptable_interface.h>
#include <ggadget/string_utils.h>
#include <ggadget/variant.h>

namespace ggadget {
DECLARE_VARIANT_PTR_TYPE(std::vector<int>);
}

using namespace ggadget;

// Hold the result of test functions/methods.
extern std::string result;

inline void TestVoidFunction0() {
  result = "TestVoidFunction0";
}

inline void TestVoidFunction9(int p1, bool p2, const char *p3,
                              const std::string &p4, std::string p5,
                              char p6, unsigned char p7,
                              short p8, std::vector<int> *p9) {
  result = ggadget::StringPrintf(
      "TestVoidFunction9: %d %d %s %s %s %c %c %d %d",
      p1, p2, p3, p4.c_str(), p5.c_str(), p6, p7, p8, (*p9)[0]);
}

inline bool TestBoolFunction0() {
  result = "TestBoolFunction0";
  return false;
}

inline bool TestBoolFunction9(int p1, bool p2, const char *p3,
                              const std::string &p4, std::string p5,
                              char p6, unsigned char p7,
                              short p8, const std::vector<int> *p9) {
  result = ggadget::StringPrintf(
      "TestBoolFunction9: %d %d %s %s %s %c %c %d %d",
      p1, p2, p3, p4.c_str(), p5.c_str(), p6, p7, p8, (*p9)[0]);
  return true;
}

inline Variant TestVariant(Variant p) {
  result = p.Print();
  return p;
}

struct TestVoidFunctor0 {
  void operator()() const {
    result = "TestVoidFunctor0";
  }
  // This operator== is required for testing.  Slot's == will call it.
  bool operator==(TestVoidFunctor0 f) const { return true; }
};

struct TestVoidFunctor9 {
  void operator()(int p1, bool p2, const char *p3, const std::string &p4,
                  std::string p5, char p6, unsigned char p7,
                  short p8, std::vector<int> *p9) const {
    result = ggadget::StringPrintf(
        "TestVoidFunctor9: %d %d %s %s %s %c %c %d %d",
        p1, p2, p3, p4.c_str(), p5.c_str(), p6, p7, p8, (*p9)[0]);
  }
  // This operator== is required for testing.  Slot's == will call it.
  bool operator==(TestVoidFunctor9 f) const { return true; }
};

struct TestBoolFunctor0 {
  bool operator()() const {
    result = "TestBoolFunctor0";
    return false;
  }
  // This operator== is required for testing.  Slot's == will call it.
  bool operator==(TestBoolFunctor0 f) const { return true; }
};

struct TestBoolFunctor9 {
  bool operator()(int p1, bool p2, const char *p3, const std::string &p4,
                  std::string p5, char p6, unsigned char p7,
                  short p8, const std::vector<int> *p9) const {
    result = ggadget::StringPrintf(
        "TestBoolFunctor9: %d %d %s %s %s %c %c %d %d",
        p1, p2, p3, p4.c_str(), p5.c_str(), p6, p7, p8, (*p9)[0]);
    return true;
  }
  // This operator== is required for testing.  Slot's == will call it.
  bool operator==(TestBoolFunctor9 f) const { return true; }
};

class TestClass0 {
 public:
  virtual ~TestClass0() { };
  virtual void TestVoidMethod2(char p1, unsigned long p2) = 0;
};

class TestClass : public TestClass0 {
 public:
  void TestVoidMethod0() {
    result = "TestVoidMethod0";
  }
  bool TestBoolMethod0() const {
    result = "TestBoolMethod0";
    return true;
  }
  virtual void TestVoidMethod2(char p1, unsigned long p2) {
    result = ggadget::StringPrintf("TestVoidMethod2: %c %lx", p1, p2);
  }
  double TestDoubleMethod2(int p1, double p2) const {
    result = ggadget::StringPrintf("TestDoubleMethod2: %d %.3lf", p1, p2);
    return 2;
  }
  void TestVoidMethod9(int p1, bool p2, const char *p3, const std::string &p4,
                       std::string p5, char p6, unsigned char p7,
                       short p8, std::vector<int> *p9) const {
    result = ggadget::StringPrintf(
        "TestVoidMethod9: %d %d %s %s %s %c %c %d %d",
        p1, p2, p3, p4.c_str(), p5.c_str(), p6, p7, p8, (*p9)[0]);
  }
  bool TestBoolMethod9(int p1, bool p2, const char *p3, const std::string &p4,
                       std::string p5, char p6, unsigned char p7,
                       short p8, const std::vector<int> *p9) {
    result = ggadget::StringPrintf(
        "TestBoolMethod9: %d %d %s %s %s %c %c %d %d",
        p1, p2, p3, p4.c_str(), p5.c_str(), p6, p7, p8, (*p9)[0]);
    return false;
  }

  Slot *TestSlotMethod(int i);
};

struct TestData {
  int argc;
  Variant::Type return_type;
  Variant::Type arg_types[10];
  Variant args[10];
  Variant return_value;
  const char *result;
};

extern TestData testdata[];
extern const int kNumTestData;

#endif // GGADGET_TESTS_SLOTS_H__
