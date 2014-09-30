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

#ifndef TESTING_GTEST_MAC_H_
#define TESTING_GTEST_MAC_H_

#include <gtest/internal/gtest-port.h>
#include <gtest/gtest.h>

#ifdef GTEST_OS_MAC

#import <Foundation/Foundation.h>

namespace testing {
namespace internal {

// This overloaded version allows comparison between ObjC objects that conform
// to the NSObject protocol. Used to implement {ASSERT|EXPECT}_EQ().
GTEST_API_ AssertionResult CmpHelperNSEQ(const char* expected_expression,
                                         const char* actual_expression,
                                         id<NSObject> expected,
                                         id<NSObject> actual);

// This overloaded version allows comparison between ObjC objects that conform
// to the NSObject protocol. Used to implement {ASSERT|EXPECT}_NE().
GTEST_API_ AssertionResult CmpHelperNSNE(const char* expected_expression,
                                         const char* actual_expression,
                                         id<NSObject> expected,
                                         id<NSObject> actual);

}  // namespace internal
}  // namespace testing

// Tests that [expected isEqual:actual].
#define EXPECT_NSEQ(expected, actual) \
  EXPECT_PRED_FORMAT2(::testing::internal::CmpHelperNSEQ, expected, actual)
#define EXPECT_NSNE(val1, val2) \
  EXPECT_PRED_FORMAT2(::testing::internal::CmpHelperNSNE, val1, val2)

#define ASSERT_NSEQ(expected, actual) \
  ASSERT_PRED_FORMAT2(::testing::internal::CmpHelperNSEQ, expected, actual)
#define ASSERT_NSNE(val1, val2) \
  ASSERT_PRED_FORMAT2(::testing::internal::CmpHelperNSNE, val1, val2)

#endif  // GTEST_OS_MAC

#endif  // TESTING_GTEST_MAC_H_