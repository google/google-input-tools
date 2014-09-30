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

#include <atlbase.h>
#include <atlcom.h>
#include "common/smart_com_ptr.h"
#include "tsf/display_attribute.h"
#include <gtest/gtest_prod.h>

namespace ime_goopy {
namespace tsf {
const GUID kInputAttributeGUID = {
  0x87046dce,
  0x8eca,
  0x45c1,
  {0xa2, 0xd8, 0xcf, 0x92, 0x20, 0x61, 0x65, 0x25}
};

// A global vairant is required to use ATL.
CComModule  _module;

TEST(DisplayAttribute, Enumerator) {
  SmartComPtr<IEnumTfDisplayAttributeInfo> enumerator;
  SmartComPtr<ITfDisplayAttributeInfo> information;
  GUID guid;
  ASSERT_EQ(S_OK, DisplayAttribute::CreateEnumerator(&enumerator));
  ULONG fetched = 0;

  // Regular usage.
  ASSERT_EQ(S_OK, enumerator->Next(1, &information, &fetched));
  EXPECT_EQ(1, fetched);
  EXPECT_EQ(S_OK, information->GetGUID(&guid));
  EXPECT_TRUE(IsEqualGUID(kInputAttributeGUID, guid));

  // Reached the end.
  information = NULL;
  fetched = 0;
  EXPECT_EQ(S_FALSE, enumerator->Next(1, &information, &fetched));
  EXPECT_EQ(0, fetched);

  // After reset.
  information = NULL;
  fetched = 0;
  EXPECT_EQ(S_OK, enumerator->Reset());
  ASSERT_EQ(S_OK, enumerator->Next(1, &information, &fetched));
  EXPECT_EQ(1, fetched);
  EXPECT_EQ(S_OK, information->GetGUID(&guid));
  EXPECT_TRUE(IsEqualGUID(kInputAttributeGUID, guid));
}
}  // namespace tsf
}  // namespace ime_goopy

