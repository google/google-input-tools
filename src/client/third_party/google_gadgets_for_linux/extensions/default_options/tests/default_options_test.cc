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

#include "ggadget/logger.h"
#include "ggadget/file_manager_factory.h"
#include "ggadget/options_interface.h"
#include "ggadget/system_utils.h"
#include "ggadget/tests/init_extensions.h"
#include "ggadget/tests/mocked_file_manager.h"
#include "ggadget/tests/mocked_timer_main_loop.h"
#include "unittest/gtest.h"

using namespace ggadget;

// TODO: system dependent.
#define TEST_DIRECTORY "/tmp/TestDefaultOptions"

MockedTimerMainLoop g_mocked_main_loop(0);
MockedFileManager g_mocked_fm;

TEST(DefaultOptions, TestAutoFlush) {
  ASSERT_EQ(std::string("profile://options/global-options.xml"),
            g_mocked_fm.requested_file_);

  const std::string kOptions1Path("profile://options/options1.xml");
  OptionsInterface *options = CreateOptions("options1");
  ASSERT_EQ(kOptions1Path, g_mocked_fm.requested_file_);
  ASSERT_TRUE(options);
  g_mocked_fm.requested_file_.clear();
  // Test the next flush timer without actual Flush() because data not changed.
  // There are two timer events, one is fired by the options instance being
  // tested, the other is the global options instance.
  g_mocked_main_loop.DoIteration(true);
  ASSERT_GE(g_mocked_main_loop.current_time_, UINT64_C(120000));
  ASSERT_LE(g_mocked_main_loop.current_time_, UINT64_C(180000));
  g_mocked_main_loop.DoIteration(true);
  ASSERT_GE(g_mocked_main_loop.current_time_, UINT64_C(120000));
  ASSERT_LE(g_mocked_main_loop.current_time_, UINT64_C(180000));
  // The file manager should not be called, because data not changed.
  ASSERT_EQ(std::string(), g_mocked_fm.requested_file_);

  options->PutValue("newItem", Variant("newValue"));
  g_mocked_main_loop.DoIteration(true);
  ASSERT_GE(g_mocked_main_loop.current_time_, UINT64_C(240000));
  ASSERT_LE(g_mocked_main_loop.current_time_, UINT64_C(360000));
  g_mocked_main_loop.DoIteration(true);
  ASSERT_GE(g_mocked_main_loop.current_time_, UINT64_C(240000));
  ASSERT_LE(g_mocked_main_loop.current_time_, UINT64_C(360000));
  // This time Flush() should be called because data changed.
  ASSERT_EQ(std::string("profile://options/options1.xml"),
            g_mocked_fm.requested_file_);
  options->RemoveAll();
  delete options;
  // Options instance should flush when it is deleted.
  ASSERT_EQ(kOptions1Path, g_mocked_fm.requested_file_);
  // The options file should be removed if the options contains nothing.
  ASSERT_EQ(std::string(), g_mocked_fm.data_[kOptions1Path]);
}

TEST(DefaultOptions, TestBasics) {
  g_mocked_fm.data_.clear();
  OptionsInterface *options = CreateOptions("options1");
  const std::string kOptions1Path("profile://options/options1.xml");
  ASSERT_EQ(kOptions1Path, g_mocked_fm.requested_file_);
  ASSERT_TRUE(options);
  const char kBinaryData[] = "\x01\0\x02xyz\n\r\"\'\\\xff\x7f<>&";
  const std::string kBinaryStr(kBinaryData, sizeof(kBinaryData) - 1);

  typedef std::map<std::string, Variant> TestData;
  TestData test_data;
  test_data["itemint"] = Variant(1);
  test_data["itembooltrue"] = Variant(true);
  test_data["itemboolfalse"] = Variant(false);
  test_data["itemdouble"] = Variant(1.234);
  test_data["itemstring"] = Variant("string");
  test_data["itemstringnull"] = Variant(static_cast<const char *>(NULL));
  test_data["itembinary"] = Variant(kBinaryStr);
  test_data["itemjson"] = Variant(JSONString("233456"));
  test_data["itemdate"] = Variant(Date(123456789));

  for (TestData::const_iterator it = test_data.begin();
       it != test_data.end(); ++it) {
    EXPECT_EQ(Variant(), options->GetValue(it->first.c_str()));
    options->PutValue(it->first.c_str(), it->second);
    options->PutValue((it->first + "_encrypted").c_str(), it->second);
    options->EncryptValue((it->first + "_encrypted").c_str());
  }

  for (TestData::const_iterator it = test_data.begin();
       it != test_data.end(); ++it) {
    EXPECT_EQ(Variant(), options->GetDefaultValue(it->first.c_str()));
    EXPECT_EQ(it->second, options->GetValue(it->first.c_str()));
    EXPECT_FALSE(options->IsEncrypted(it->first.c_str()));
    EXPECT_EQ(it->second, options->GetValue((it->first + "_encrypted").c_str()));
    EXPECT_TRUE(options->IsEncrypted((it->first + "_encrypted").c_str()));
  }

  options->PutDefaultValue("test_default", Variant("default"));
  options->PutInternalValue("test_internal", Variant("internal"));
  EXPECT_EQ(Variant("default"), options->GetDefaultValue("test_default"));
  EXPECT_EQ(Variant("default"), options->GetValue("test_default"));
  EXPECT_EQ(Variant("internal"), options->GetInternalValue("test_internal"));
  // Default and internal items don't affect count.
  EXPECT_EQ(test_data.size() * 2, options->GetCount());

  options->Flush();
  delete options;

  // NULL string become blank string when persisted and loaded.
  test_data["itemstringnull"] = Variant("");
  const std::string kOptions2Path("profile://options/options2.xml");
  g_mocked_fm.data_[kOptions2Path] = g_mocked_fm.data_[kOptions1Path];

  options = CreateOptions("options2");
  ASSERT_EQ(kOptions2Path, g_mocked_fm.requested_file_);
  ASSERT_TRUE(options);
  for (TestData::const_iterator it = test_data.begin();
       it != test_data.end(); ++it) {
    EXPECT_EQ(Variant(), options->GetDefaultValue(it->first.c_str()));
    EXPECT_EQ(it->second, options->GetValue(it->first.c_str()));
    EXPECT_FALSE(options->IsEncrypted(it->first.c_str()));
    EXPECT_EQ(it->second, options->GetValue((it->first + "_encrypted").c_str()));
    EXPECT_TRUE(options->IsEncrypted((it->first + "_encrypted").c_str()));
  }
  EXPECT_EQ(Variant("internal"), options->GetInternalValue("test_internal"));
  // Default values won't get persisted.
  EXPECT_EQ(Variant(), options->GetDefaultValue("test_default"));
  EXPECT_EQ(Variant(), options->GetValue("test_default"));

  // Test addditional default value logic.
  options->PutDefaultValue("itemdouble", Variant(456.7));
  options->Remove("itemdouble");
  EXPECT_EQ(Variant(456.7), options->GetValue("itemdouble"));
  options->PutValue("itemdouble", Variant(789));
  EXPECT_EQ(Variant(789), options->GetValue("itemdouble"));

  // If new value is set, encrypted state will be cleared.
  options->PutValue("itemdouble_encrypted", Variant(432.1));
  EXPECT_FALSE(options->IsEncrypted("itemdouble_encrypted"));
  options->DeleteStorage();
  delete options;
}

TEST(DefaultOptions, TestSizeLimit) {
  OptionsInterface *options = CreateOptions("options1");
  std::string big_string1(400000, 'a');
  std::string big_string2(600000, 'b');
  options->RemoveAll();
  options->Add("a", Variant(big_string1));
  ASSERT_EQ(Variant(big_string1), options->GetValue("a"));
  options->Add("b", Variant(big_string1));
  ASSERT_EQ(Variant(big_string1), options->GetValue("b"));
  // Exceed size limit.
  options->Add("c", Variant(big_string1));
  ASSERT_EQ(Variant(), options->GetValue("c"));
  options->PutValue("a", Variant(big_string2));
  ASSERT_EQ(Variant(big_string2), options->GetValue("a"));
  // Exceed size limit.
  options->PutValue("b", Variant(big_string2));
  ASSERT_EQ(Variant(big_string1), options->GetValue("b"));

  options->Remove("b");
  options->Add("c", Variant(big_string1));
  ASSERT_EQ(Variant(big_string1), options->GetValue("c"));
}

TEST(DefaultOptions, TestOptionsSharing) {
  g_mocked_fm.data_.clear();
  OptionsInterface *options = CreateOptions("options1");
  ASSERT_EQ(std::string("profile://options/options1.xml"),
            g_mocked_fm.requested_file_);
  g_mocked_fm.requested_file_.clear();

  // This new options instance should share the same backend data with the
  // previously created one.
  OptionsInterface *options1 = CreateOptions("options1");
  ASSERT_EQ(std::string(), g_mocked_fm.requested_file_);

  options1->PutValue("TestSharing", Variant(100));
  ASSERT_EQ(Variant(100), options->GetValue("TestSharing"));

  delete options1;
  ASSERT_EQ(Variant(100), options->GetValue("TestSharing"));
  delete options;
}

int main(int argc, char **argv) {
  SetGlobalMainLoop(&g_mocked_main_loop);
  SetGlobalFileManager(&g_mocked_fm);
  testing::ParseGTestFlags(&argc, argv);

  static const char *kExtensions[] = {
    "libxml2_xml_parser/libxml2-xml-parser",
    "default_options/default-options",
  };
  INIT_EXTENSIONS(argc, argv, kExtensions);

  return RUN_ALL_TESTS();
}
