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
//
// Based on the source code of Chromium.
// Original code: chromimutrunk/sandbox/src/sid.*

#include "common/sid.h"
#define _ATL_NO_EXCEPTIONS
#include <atlbase.h>
#include <atlsecurity.h>
#include <gtest/gunit.h>

namespace ime_goopy {

// Calls ::EqualSid. This function exists only to simplify the calls to
// ::EqualSid by removing the need to cast the input params.
BOOL EqualSid(const SID *sid1, const SID *sid2) {
  return ::EqualSid(const_cast<SID*>(sid1), const_cast<SID*>(sid2));
}

// Tests the creation if a Sid
TEST(SidTest, Constructors) {
  ATL::CSid sid_world = ATL::Sids::World();
  SID *sid_world_pointer = const_cast<SID*>(sid_world.GetPSID());

  // Check the SID* constructor
  Sid sid_sid_star(sid_world_pointer);
  ASSERT_TRUE(EqualSid(sid_world_pointer, sid_sid_star.GetPSID()));

  // Check the copy constructor
  Sid sid_copy(sid_sid_star);
  ASSERT_TRUE(EqualSid(sid_world_pointer, sid_copy.GetPSID()));

  // Note that the WELL_KNOWN_SID_TYPE constructor is tested in the GetPSID
  // test.
}

// Tests the method GetPSID
TEST(SidTest, GetPSID) {
  // Check for non-null result;
  ASSERT_NE(static_cast<SID*>(NULL), Sid(::WinLocalSid).GetPSID());
  ASSERT_NE(static_cast<SID*>(NULL), Sid(::WinCreatorOwnerSid).GetPSID());
  ASSERT_NE(static_cast<SID*>(NULL), Sid(::WinBatchSid).GetPSID());

  ASSERT_TRUE(EqualSid(Sid(::WinNullSid).GetPSID(),
                       ATL::Sids::Null().GetPSID()));

  ASSERT_TRUE(EqualSid(Sid(::WinWorldSid).GetPSID(),
                       ATL::Sids::World().GetPSID()));

  ASSERT_TRUE(EqualSid(Sid(::WinDialupSid).GetPSID(),
                       ATL::Sids::Dialup().GetPSID()));

  ASSERT_TRUE(EqualSid(Sid(::WinNetworkSid).GetPSID(),
                       ATL::Sids::Network().GetPSID()));

  ASSERT_TRUE(EqualSid(Sid(::WinBuiltinAdministratorsSid).GetPSID(),
                       ATL::Sids::Admins().GetPSID()));

  ASSERT_TRUE(EqualSid(Sid(::WinBuiltinUsersSid).GetPSID(),
                       ATL::Sids::Users().GetPSID()));

  ASSERT_TRUE(EqualSid(Sid(::WinBuiltinGuestsSid).GetPSID(),
                       ATL::Sids::Guests().GetPSID()));

  ASSERT_TRUE(EqualSid(Sid(::WinProxySid).GetPSID(),
                       ATL::Sids::Proxy().GetPSID()));
}

}  // namespace ime_goopy
