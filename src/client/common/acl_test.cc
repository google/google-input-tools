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

#include "common/acl.h"
#define _ATL_NO_EXCEPTIONS
#include <atlbase.h>
#include <atlsecurity.h>
#include <gtest/gunit.h>

namespace ime_goopy {

TEST(AclTest, GetDefaultDacl) {
  ATL::CAccessToken token;
  ASSERT_TRUE(token.GetProcessToken(TOKEN_READ |
                                    TOKEN_WRITE |
                                    TOKEN_QUERY_SOURCE));
  scoped_ptr<TOKEN_DEFAULT_DACL> default_dacl;
  ASSERT_TRUE(GetDefaultDacl(token.GetHandle(), &default_dacl));
  EXPECT_TRUE(default_dacl->DefaultDacl);
}

TEST(AclTest, AddSidToDacl) {
  ATL::CAccessToken token;
  ASSERT_TRUE(token.GetProcessToken(TOKEN_READ |
                                    TOKEN_WRITE |
                                    TOKEN_QUERY_SOURCE));
  scoped_ptr<TOKEN_DEFAULT_DACL> default_dacl;
  ASSERT_TRUE(GetDefaultDacl(token.GetHandle(), &default_dacl));
  ACL* new_dacl = NULL;
  EXPECT_TRUE(AddSidToDacl(WinRestrictedCodeSid,
                           default_dacl->DefaultDacl,
                           GENERIC_ALL,
                           &new_dacl));
}

TEST(AclTest, AddSidToDefaultDacl) {
  ATL::CAccessToken token;
  ASSERT_TRUE(token.GetProcessToken(TOKEN_READ |
                                    TOKEN_WRITE |
                                    TOKEN_QUERY_SOURCE));
  EXPECT_TRUE(AddSidToDefaultDacl(token.GetHandle(),
                                  WinRestrictedCodeSid,
                                  GENERIC_READ));
}


TEST(AclTest, AddUserSidToDacl) {
  ATL::CAccessToken token;
  ASSERT_TRUE(token.GetProcessToken(TOKEN_READ |
                                    TOKEN_WRITE |
                                    TOKEN_QUERY_SOURCE));
  EXPECT_TRUE(AddUserSidToDefaultDacl(token.GetHandle(), GENERIC_READ));
}


}  // namespace ime_goopy
