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

#include <cstdlib>
#include "ggadget/backoff.h"
#include "ggadget/string_utils.h"
#include "unittest/gtest.h"

using namespace ggadget;

// The following constants should be kept the same values as in the impl.
// Not put them into header because they are impl specific.
static const int kBaseInterval = 30000; // 30 seconds.
static const int kMaxRetryInterval = 4 * 3600 * 1000; // 4 hours.
static const uint64_t kExpirationInterval = 24U * 3600U * 1000U; // 24 hours.

// Make sure that next request to IsOkToRequest fails on request by failing
// multiple times (required because of the randomization).
void FailMultipleTimes(uint64_t now, Backoff *backoff, const char *request,
                       Backoff::ResultType result_type) {
  for (int i = 0; i < 4; ++i)
    EXPECT_TRUE(backoff->ReportRequestResult(now, request, result_type));
}

TEST(Backoff, FirstRequest) {
  const char *kSite1 = "http://site.com/stuff";
  const char *kSite2 = "http://site.com";

  Backoff backoff;
  uint64_t now = UINT64_C(0x0001000200030004);
  EXPECT_TRUE(backoff.IsOkToRequest(now, kSite1));
  EXPECT_TRUE(backoff.IsOkToRequest(now, kSite2));
  FailMultipleTimes(now, &backoff, kSite2, Backoff::EXPONENTIAL_BACKOFF);
  EXPECT_TRUE(backoff.IsOkToRequest(now, kSite1));
  EXPECT_EQ(0, backoff.GetFailureCount(kSite1));
  EXPECT_FALSE(backoff.IsOkToRequest(now, kSite2));
  EXPECT_EQ(4, backoff.GetFailureCount(kSite2));
  EXPECT_TRUE(backoff.IsOkToRequest(now + 16 * kBaseInterval + 1, kSite2));
  EXPECT_TRUE(backoff.IsOkToRequest(backoff.GetNextAllowedTime(kSite2) + 1,
                                    kSite2));
  EXPECT_TRUE(backoff.IsOkToRequest(backoff.GetNextAllowedTime(kSite2), kSite2));
  EXPECT_FALSE(backoff.IsOkToRequest(backoff.GetNextAllowedTime(kSite2) - 1,
                                     kSite2));
  FailMultipleTimes(now, &backoff, kSite1, Backoff::EXPONENTIAL_BACKOFF);
  EXPECT_FALSE(backoff.IsOkToRequest(now, kSite1));
  EXPECT_FALSE(backoff.IsOkToRequest(now, kSite2));
  EXPECT_TRUE(backoff.ReportRequestResult(now, kSite1, Backoff::SUCCESS));
  EXPECT_TRUE(backoff.ReportRequestResult(now, kSite2, Backoff::SUCCESS));
  EXPECT_FALSE(backoff.ReportRequestResult(now, kSite2, Backoff::SUCCESS));
  EXPECT_FALSE(backoff.ReportRequestResult(now, "http://some.com",
                                           Backoff::SUCCESS));
  EXPECT_TRUE(backoff.IsOkToRequest(now, kSite1));
  EXPECT_TRUE(backoff.IsOkToRequest(now, kSite2));
  FailMultipleTimes(now, &backoff, kSite1, Backoff::EXPONENTIAL_BACKOFF);
  backoff.Clear();
  EXPECT_TRUE(backoff.IsOkToRequest(now, kSite1));
  EXPECT_TRUE(backoff.IsOkToRequest(now, kSite2));
}

// Make sure time_interval is a legal value.
// Valid Ranage: 2^(error_count - 4) .. 2^(error_count - 1) +- 20%
bool IsValidTimeout(uint64_t a_interval, int error_count, int *exp) {
  int interval = static_cast<int>(a_interval);
  if (error_count <= 3 && interval == 0) {
    *exp = -1;
    return true;
  }
  for (int i = std::max(error_count - 4, 0); i < error_count; i++) {
    int exp_interval = std::min(kMaxRetryInterval, kBaseInterval * (1 << i));
    int tolerance = exp_interval * 20 / 100;
    if (interval >= exp_interval - tolerance &&
        interval <= exp_interval + tolerance) {
      *exp = i;
      return true;
    }
  }
  *exp = -1;
  return false;
}

TEST(Backoff, TimeoutIntervalWithinRange) {
  Backoff backoff;
  uint64_t now = UINT64_C(0x0001000200030004);
  const char *kSite1 = "http://site.com/stuff";
  for (int i = 0; i < 1000; i++) {
    backoff.Clear();
    for (int error_count = 1; error_count <= 16; error_count++) {
      int exp;
      backoff.ReportRequestResult(now, kSite1, Backoff::EXPONENTIAL_BACKOFF);
      EXPECT_TRUE_M(IsValidTimeout(backoff.GetNextAllowedTime(kSite1) - now,
                                   error_count, &exp),
                    StringPrintf("IsValidTimeout: error_count %d exp %d "
                                 "actual interval %" PRIu64, error_count, exp,
                                 backoff.GetNextAllowedTime(kSite1) - now));
      now += 1000000;
    }
  }
}

void EnsureRandomization(int max_error_count) {
  Backoff backoff;
  uint64_t now = UINT64_C(0x0001000200030004);
  const int kMaxIterations = 10000;
  const int kTolerance = kMaxIterations / 50;  // 2%
  int distribution[4] = { 0 };
  const char *kSite1 = "http://site.com/stuff";
  for (int i = 0; i < kMaxIterations; ++i) {
    backoff.Clear();
    for (int error_count = 0; error_count < max_error_count; error_count++)
      backoff.ReportRequestResult(now, kSite1, Backoff::EXPONENTIAL_BACKOFF);
    uint64_t interval = backoff.GetNextAllowedTime(kSite1) - now;
    int exp;
    ASSERT_TRUE(IsValidTimeout(interval, max_error_count, &exp));
    ASSERT_GE(exp, max_error_count - 4);
    ASSERT_LE(exp, max_error_count - 1);
    distribution[exp - max_error_count + 4]++;
    now += 1000000;
  }
  printf("error_count %d distribution: %d %d %d %d\n", max_error_count,
         distribution[0], distribution[1], distribution[2], distribution[3]);
  if (max_error_count == 1) {
    EXPECT_EQ(0, distribution[0]); // -2
    EXPECT_EQ(0, distribution[1]); // -1
    EXPECT_EQ(kMaxIterations, distribution[2]); // 0: not backoff-ed.
    EXPECT_EQ(0, distribution[3]);
  } else if (max_error_count == 2) {
    EXPECT_EQ(0, distribution[0]); // -1
    EXPECT_EQ(kMaxIterations, distribution[1]); // 0: not backoff-ed.
    EXPECT_EQ(0, distribution[2]);
    EXPECT_EQ(0, distribution[3]);
  } else if (max_error_count == 3) {
    EXPECT_EQ(0, distribution[0]); // 0: no not backoff-ed
    EXPECT_GE(kTolerance, abs(distribution[1] - 2 * kMaxIterations / 4));
    EXPECT_GE(kTolerance, abs(distribution[2] - kMaxIterations / 4));
    EXPECT_GE(kTolerance, abs(distribution[3] - kMaxIterations / 4));
  } else {
    for (int i = 0; i < 4; i++) {
      EXPECT_GE(kTolerance, abs(distribution[i] - kMaxIterations / 4));
    }
  }
}

// Make sure that randomization is evenenly distributed.
TEST(Backoff, TimeoutRandomization) {
  for (int error_count = 1; error_count <= 10; error_count++)
    EnsureRandomization(error_count);
}

TEST(Backoff, GetSetData) {
  const char *kSite1 = "http://site.com/stuff";
  const char *kSite2 = "http://site.com";

  Backoff backoff;
  backoff.Clear();

  uint64_t now = UINT64_C(0x0001000200030004);
  FailMultipleTimes(now, &backoff, kSite1, Backoff::EXPONENTIAL_BACKOFF);
  FailMultipleTimes(now, &backoff, kSite2, Backoff::CONSTANT_BACKOFF);

  std::string expected_data(StringPrintf("%s\t%" PRIu64 "\t%d\n%s\t%" PRIu64
                                         "\t%d\n",
                                         kSite2, now, -4, kSite1, now, 4));
  EXPECT_EQ(expected_data, backoff.GetData(now));
  EXPECT_TRUE(backoff.GetData(
      now + kExpirationInterval + kBaseInterval * 16).empty());

  backoff.Clear();
  backoff.SetData(now, expected_data.c_str());
  EXPECT_FALSE(backoff.IsOkToRequest(now, kSite1));
  EXPECT_FALSE(backoff.IsOkToRequest(now, kSite2));
  backoff.SetData(now + kExpirationInterval, expected_data.c_str());
  EXPECT_TRUE(backoff.IsOkToRequest(0, kSite1));
  EXPECT_TRUE(backoff.IsOkToRequest(0, kSite2));
  EXPECT_TRUE(backoff.GetData(
      now + kExpirationInterval + kBaseInterval * 16).empty());
}

int main(int argc, char **argv) {
  testing::ParseGTestFlags(&argc, argv);
  return RUN_ALL_TESTS();
}
