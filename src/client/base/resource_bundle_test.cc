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

#include "base/resource_bundle.h"

#include "base/resource_bundle_test1.grh"
#include "base/resource_bundle_test2.grh"
#include "ime/shared/base/resource/data_pack.h"
#include "testing/base/public/gunit.h"

namespace {

#if defined(BASE_PATH)
static const char kBasePath[] = BASE_PATH;
#else
static const char kBasePath[] = "";
#endif

using ime_goopy::ResourceBundle;
using ime_shared::DataPack;
TEST(ResourceBundleTest, ResourceBundleTest) {
  // Use a fake local name.
  ResourceBundle::InitSharedInstanceWithLocale("en-XA-fake");
  std::string base_path = kBasePath;
  std::string locale = ResourceBundle::AddDataPackToSharedInstance(
      base_path + "/resource_bundle_test1_[LANG].pak");
  // Can not find "resource_bundle_test1_en_XA_fake.pak", fallback to en_XA;
  EXPECT_EQ("en_XA", locale);
  locale = ResourceBundle::AddDataPackToSharedInstance(
      base_path + "/[LANG]/resource_bundle_test2.pak");
  // Can not find "en_XA_fake/resource_bundle_test1.pak", fallback to en_XA;
  EXPECT_EQ("en_XA", locale);
  std::wstring test1 = ResourceBundle::GetSharedInstance().GetLocalizedString(
      IDS_TEST1);
  EXPECT_FALSE(test1.empty());
  std::wstring test2 = ResourceBundle::GetSharedInstance().GetLocalizedString(
      IDS_TEST2);
  EXPECT_FALSE(test2.empty());
  std::wstring test5 = ResourceBundle::GetSharedInstance().GetLocalizedString(
      IDS_TEST5);
  EXPECT_FALSE(test5.empty());
  ResourceBundle::GetSharedInstance().ReloadLocaleResources("en");
  test1 = ResourceBundle::GetSharedInstance().GetLocalizedString(IDS_TEST1);
  EXPECT_FALSE(test1.empty());
  test2 = ResourceBundle::GetSharedInstance().GetLocalizedString(IDS_TEST2);
  EXPECT_FALSE(test2.empty());
  test5 = ResourceBundle::GetSharedInstance().GetLocalizedString(IDS_TEST5);
  EXPECT_FALSE(test5.empty());
  ResourceBundle::GetSharedInstance().ReloadLocaleResources("ar-xx-yy");
  // Fallback to en.
  EXPECT_EQ(test1,
            ResourceBundle::GetSharedInstance().GetLocalizedString(IDS_TEST1));
  EXPECT_EQ(test2,
            ResourceBundle::GetSharedInstance().GetLocalizedString(IDS_TEST2));
  EXPECT_EQ(test5,
            ResourceBundle::GetSharedInstance().GetLocalizedString(IDS_TEST5));
};
}  // namespace

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
