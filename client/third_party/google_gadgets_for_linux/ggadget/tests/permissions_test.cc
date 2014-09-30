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

#include "ggadget/common.h"
#include "ggadget/permissions.h"
#include "ggadget/slot.h"
#include "ggadget/locales.h"
#include "ggadget/logger.h"
#include "unittest/gtest.h"

using namespace ggadget;

TEST(Permissions, Granted) {
  Permissions perm;

  // <network/> is mutually exclusive with <fileread/> and <devicestatus/>
  // <fileread/> and <devicestatus/> are granted by default.
  EXPECT_TRUE(perm.IsGranted(Permissions::FILE_READ));
  EXPECT_TRUE(perm.IsGranted(Permissions::DEVICE_STATUS));
  perm.SetGranted(Permissions::NETWORK, true);
  EXPECT_FALSE(perm.IsGranted(Permissions::FILE_READ));
  EXPECT_FALSE(perm.IsGranted(Permissions::DEVICE_STATUS));
  perm.SetGranted(Permissions::NETWORK, false);
  EXPECT_TRUE(perm.IsGranted(Permissions::FILE_READ));
  EXPECT_TRUE(perm.IsGranted(Permissions::DEVICE_STATUS));

  // Check each permission.
  for (int i = Permissions::FILE_READ; i < Permissions::ALL_ACCESS; ++i) {
    if (i == Permissions::FILE_READ || i == Permissions::DEVICE_STATUS) {
      // <fileread/> and <devicestatus/> are granted by default.
      EXPECT_TRUE(perm.IsGranted(i));
      perm.SetGranted(i, false);
      EXPECT_FALSE(perm.IsGranted(i));
      perm.SetGrantedByName(Permissions::GetName(i).c_str(), true);
      EXPECT_TRUE(perm.IsGranted(i));
    } else {
      EXPECT_FALSE(perm.IsGranted(i));
      perm.SetGranted(i, true);
      EXPECT_TRUE(perm.IsGranted(i));
      perm.SetGrantedByName(Permissions::GetName(i).c_str(), false);
      EXPECT_FALSE(perm.IsGranted(i));
    }
  }

  // <allaccess/> implies all other permissions.
  EXPECT_FALSE(perm.IsGranted(Permissions::ALL_ACCESS));
  perm.SetGranted(Permissions::ALL_ACCESS, true);
  EXPECT_TRUE(perm.IsGranted(Permissions::ALL_ACCESS));
  for (int i = Permissions::FILE_READ; i < Permissions::ALL_ACCESS; ++i) {
    // if <allaccess/> is granted, then denying individual permission won't
    // take effect.
    EXPECT_TRUE(perm.IsGranted(i));
    perm.SetGranted(i, true);
    EXPECT_TRUE(perm.IsGranted(i));
    perm.SetGranted(i, false);
    EXPECT_TRUE(perm.IsGranted(i));
  }
  perm.SetGrantedByName(Permissions::GetName(Permissions::ALL_ACCESS).c_str(),
                        false);
  // All permissions are denied explicitly, including <fileread/> and
  // <devicestatus/>
  for (int i = Permissions::FILE_READ; i < Permissions::ALL_ACCESS; ++i) {
    EXPECT_FALSE(perm.IsGranted(i));
  }
}

TEST(Permissions, Required) {
  Permissions perm;

  EXPECT_FALSE(perm.HasUngranted());
  // Denies <fileread/> and <devicestatus/> explicitly.
  perm.SetGranted(Permissions::FILE_READ, false);
  perm.SetGranted(Permissions::DEVICE_STATUS, false);
  for (int i = Permissions::FILE_READ; i < Permissions::ALL_ACCESS; ++i) {
    EXPECT_FALSE(perm.IsRequired(i));
    perm.SetRequired(i, true);
    EXPECT_TRUE(perm.IsRequired(i));
    EXPECT_TRUE(perm.HasUngranted());
    perm.SetGranted(i, true);
    EXPECT_FALSE(perm.HasUngranted());
    perm.SetGranted(i, false);
    perm.SetRequiredByName(Permissions::GetName(i).c_str(), false);
    EXPECT_FALSE(perm.IsRequired(i));
  }
  EXPECT_FALSE(perm.IsRequired(Permissions::ALL_ACCESS));
  perm.SetRequired(Permissions::ALL_ACCESS, true);
  EXPECT_TRUE(perm.IsRequired(Permissions::ALL_ACCESS));
  for (int i = Permissions::FILE_READ; i < Permissions::ALL_ACCESS; ++i) {
    EXPECT_TRUE(perm.IsRequired(i));
    perm.SetRequired(i, true);
    EXPECT_TRUE(perm.IsRequired(i));
    perm.SetRequired(i, false);
    EXPECT_TRUE(perm.IsRequired(i));
  }
  EXPECT_TRUE(perm.HasUngranted());
  perm.SetGranted(Permissions::ALL_ACCESS, true);
  EXPECT_FALSE(perm.HasUngranted());
}

struct EnumerateCallbackData {
  Permissions perm;
  int count;
};

bool EnumerateGrantedCallback(int permission, EnumerateCallbackData *data) {
  EXPECT_TRUE(data->perm.IsGranted(permission));
  data->count++;
  return true;
}

bool EnumerateRequiredCallback(int permission, EnumerateCallbackData *data) {
  EXPECT_TRUE(data->perm.IsRequired(permission));
  data->count++;
  return true;
}

TEST(Permissions, Enumerates) {
  EnumerateCallbackData data;

  data.count = 0;
  EXPECT_TRUE(data.perm.EnumerateAllGranted(
      NewSlot(EnumerateGrantedCallback, &data)));
  EXPECT_EQ(2, data.count);
  data.perm.SetGranted(Permissions::FILE_READ, false);
  data.perm.SetGranted(Permissions::DEVICE_STATUS, false);

  data.count = 0;
  EXPECT_FALSE(data.perm.EnumerateAllGranted(
      NewSlot(EnumerateGrantedCallback, &data)));
  EXPECT_EQ(0, data.count);
  int count = 0;
  for (int i = Permissions::FILE_READ; i <= Permissions::ALL_ACCESS; ++i) {
    data.perm.SetGranted(i, true);
    data.count = 0;
    EXPECT_TRUE(data.perm.EnumerateAllGranted(
        NewSlot(EnumerateGrantedCallback, &data)));
    EXPECT_EQ(++count, data.count);
  }
  for (int i = Permissions::FILE_READ; i < Permissions::ALL_ACCESS; ++i) {
    data.perm.SetGranted(i, false);
    data.count = 0;
    EXPECT_TRUE(data.perm.EnumerateAllGranted(
        NewSlot(EnumerateGrantedCallback, &data)));
    EXPECT_EQ(count, data.count);
  }
  data.perm.SetGranted(Permissions::ALL_ACCESS, false);
  data.count = 0;
  EXPECT_FALSE(data.perm.EnumerateAllGranted(
      NewSlot(EnumerateGrantedCallback, &data)));
  EXPECT_EQ(0, data.count);

  data.count = 0;
  EXPECT_FALSE(data.perm.EnumerateAllRequired(
      NewSlot(EnumerateRequiredCallback, &data)));
  EXPECT_EQ(0, data.count);
  count = 0;
  for (int i = Permissions::FILE_READ; i <= Permissions::ALL_ACCESS; ++i) {
    data.perm.SetRequired(i, true);
    data.count = 0;
    EXPECT_TRUE(data.perm.EnumerateAllRequired(
        NewSlot(EnumerateRequiredCallback, &data)));
    EXPECT_EQ(++count, data.count);
  }
  for (int i = Permissions::FILE_READ; i < Permissions::ALL_ACCESS; ++i) {
    data.perm.SetRequired(i, false);
    data.count = 0;
    EXPECT_TRUE(data.perm.EnumerateAllRequired(
        NewSlot(EnumerateRequiredCallback, &data)));
    EXPECT_EQ(count, data.count);
  }
  data.perm.SetRequired(Permissions::ALL_ACCESS, false);
  data.count = 0;
  EXPECT_FALSE(data.perm.EnumerateAllRequired(
      NewSlot(EnumerateRequiredCallback, &data)));
  EXPECT_EQ(0, data.count);
}

TEST(Permissions, Name) {
  static const char *kNames[] = {
    "fileread", "filewrite", "devicestatus", "network",
    "personaldata", "allaccess"
  };

  for (int i = Permissions::FILE_READ; i <= Permissions::ALL_ACCESS; ++i) {
    EXPECT_STREQ(kNames[i], Permissions::GetName(i).c_str());
    EXPECT_EQ(i, Permissions::GetByName(kNames[i]));
  }
}

TEST(Permissions, SaveLoad) {
  Permissions perm1, perm2;
  std::string str;

  str = perm1.ToString();
  perm2.FromString(str.c_str());
  EXPECT_TRUE(perm1 == perm2);

  for (int i = Permissions::FILE_READ; i <= Permissions::ALL_ACCESS; ++i) {
    perm1.SetGranted(i, true);
    str = perm1.ToString();
    DLOG("Permissions: %s", str.c_str());
    perm2.FromString(str.c_str());
    EXPECT_TRUE(perm1 == perm2);

    perm1.SetRequired(i, true);
    str = perm1.ToString();
    DLOG("Permissions: %s", str.c_str());
    perm2.FromString(str.c_str());
    EXPECT_TRUE(perm1 == perm2);
  }

  for (int i = Permissions::FILE_READ; i <= Permissions::ALL_ACCESS; ++i) {
    perm1.SetGranted(i, false);
    str = perm1.ToString();
    DLOG("Permissions: %s", str.c_str());
    perm2.FromString(str.c_str());
    EXPECT_TRUE(perm1 == perm2);

    perm1.SetRequired(i, false);
    str = perm1.ToString();
    DLOG("Permissions: %s", str.c_str());
    perm2.FromString(str.c_str());
    EXPECT_TRUE(perm1 == perm2);
  }
}

int main(int argc, char **argv) {
  ggadget::SetLocaleForUiMessage("en_US.UTF-8");
  testing::ParseGTestFlags(&argc, argv);

  return RUN_ALL_TESTS();
}
