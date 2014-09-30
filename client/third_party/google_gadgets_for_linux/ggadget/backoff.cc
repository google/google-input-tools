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
#include "backoff.h"

#include <map>
#include "logger.h"
#include "string_utils.h"
#include "small_object.h"

namespace ggadget {

// The basic interval of the Truncated Binary Exponential backoff algorithm.
static const int kBaseInterval = 30000; // 30 seconds.
// The maximum retry interval after repeated failures.
static const int kMaxRetryInterval = 4 * 3600 * 1000; // 4 hours.
// Backoff entry will be removed it has not been requested for this interval.
static const uint64_t kExpirationInterval = 24U * 3600U * 1000U; // 24 hours.

// NOTE: The backoff and randomization features in this implementation is
// very important for proper server-side operation. Please do *NOT* disable
// or remove them.
class Backoff::Impl : public SmallObject<> {
 public:
  bool IsOkToRequest(uint64_t now, const char *request) {
    ASSERT(request);
    BackoffInfoMap::const_iterator it = backoff_info_map_.find(request);
    if (it == backoff_info_map_.end())
      return true;

    // Quick sanity check to make sure the error occurred in the past if the
    // user has network problems and changes his clock (possibly had an
    // incorrect date set on his calendar) we want to make sure we continue to
    // do requests - instead of not doing http requests for days/weeks/months.
    if (it->second.last_failure_time > now)
      return true;

    // Now check if we have passed the set time limit.
    return now >= it->second.next_try_time;
  }

  uint64_t GetNextAllowedTime(const char *request) {
    ASSERT(request);
    BackoffInfoMap::const_iterator it = backoff_info_map_.find(request);
    return it == backoff_info_map_.end() ? 0 : it->second.next_try_time;
  }

  int GetFailureCount(const char *request) {
    ASSERT(request);
    BackoffInfoMap::const_iterator it = backoff_info_map_.find(request);
    return it == backoff_info_map_.end() ? 0 : it->second.failure_count;
  }

  // Return a value which is the input value +/- random(20%).
  static int Randomize(int input) {
    int variant = input * 20 / 100;
    return std::max(0, input + (rand() % (variant * 2)) - variant);
  }

  static int GetNextRequestInterval(int failure_count, ResultType result_type) {
    // Do not start backoff on the first two retries to tolerate some
    // temporary failure or the case that the server returns failure code
    // to let the client do something (e.g. refresh a security token).
    if (failure_count <= 2)
      return 0;

    if (result_type == CONSTANT_BACKOFF)
      return Randomize(kBaseInterval);

    ASSERT(result_type == EXPONENTIAL_BACKOFF);
    // wait_exp is failure_count - random(3 .. failure_count).
    int wait_exp = failure_count - (rand() / (0x7FFF / 4)) % 4;
    wait_exp = std::max(1, std::min(15, wait_exp));
    return Randomize(std::min(kMaxRetryInterval,
                              kBaseInterval * (1 << (wait_exp - 1))));
  }

  bool ReportRequestResult(uint64_t now, const char *request,
                           ResultType result_type) {
    ASSERT(request);
    if (result_type == SUCCESS) {
      BackoffInfoMap::iterator it = backoff_info_map_.find(request);
      if (it != backoff_info_map_.end()) {
        backoff_info_map_.erase(it);
        return true;
      }
    } else {
      BackoffInfo *backoff_info = &backoff_info_map_[request];
      backoff_info->failure_count++;
      backoff_info->last_failure_time = now;
      backoff_info->result_type = result_type;
      backoff_info->next_try_time = now + static_cast<uint64_t>(
          GetNextRequestInterval(backoff_info->failure_count, result_type));
      return true;
    }
    return false;
  }

  void SetData(uint64_t now, const char *data) {
    backoff_info_map_.clear();
    while (data && *data) {
      const char *p = strchr(data, '\t');
      if (p) {
        std::string request(data, static_cast<size_t>(p - data));
        BackoffInfo backoff_info;
        if (sscanf(p + 1, "%" PRIu64 "\t%d\n",
                   &backoff_info.last_failure_time,
                   &backoff_info.failure_count) == 2) {
          if (backoff_info.failure_count < 0) {
            // Use negative failure_count to indicate constant backoff, so that
            // compatibility of config file can be achived.
            backoff_info.failure_count = -backoff_info.failure_count;
            backoff_info.result_type = CONSTANT_BACKOFF;
          }
          backoff_info.next_try_time =
              backoff_info.last_failure_time +
              static_cast<uint64_t>(GetNextRequestInterval(
                  backoff_info.failure_count, backoff_info.result_type));
          if (backoff_info.next_try_time + kExpirationInterval > now)
            backoff_info_map_[request] = backoff_info;
          data = strchr(data, '\n');
          if (data)
            ++data;
        } else {
          DLOG("Invalid backoff data: %s", p);
          break;
        }
      } else {
        DLOG("Invalid backoff data: %s", data);
        break;
      }
    }
  }

  std::string GetData(uint64_t now) {
    std::string result;
    for (BackoffInfoMap::const_iterator it = backoff_info_map_.begin();
         it != backoff_info_map_.end(); ++it) {
      if (it->second.next_try_time + kExpirationInterval > now) {
        result.append(it->first);
        int failure_count = it->second.failure_count;
        // Use negative failure_count to indicate constant backoff, so that
        // compatibility of config file can be achived.
        if (it->second.result_type == CONSTANT_BACKOFF)
          failure_count = -failure_count;
        result.append(StringPrintf("\t%" PRIu64 "\t%d\n",
                                   it->second.last_failure_time,
                                   failure_count));
      }
    }
    return result;
  }

  struct BackoffInfo {
    BackoffInfo()
        : last_failure_time(0), failure_count(0), next_try_time(0),
          result_type(EXPONENTIAL_BACKOFF) { }
    uint64_t last_failure_time;
    int failure_count;
    uint64_t next_try_time;
    ResultType result_type;
  };

  typedef LightMap<std::string, BackoffInfo> BackoffInfoMap;
  BackoffInfoMap backoff_info_map_;
};

Backoff::Backoff() : impl_(new Impl()) {
}

Backoff::~Backoff() {
  delete impl_;
}

bool Backoff::IsOkToRequest(uint64_t now, const char *request) {
  return impl_->IsOkToRequest(now, request);
}

uint64_t Backoff::GetNextAllowedTime(const char *request) {
  return impl_->GetNextAllowedTime(request);
}

int Backoff::GetFailureCount(const char *request) {
  return impl_->GetFailureCount(request);
}

bool Backoff::ReportRequestResult(uint64_t now, const char *request,
                                  ResultType result_type) {
  return impl_->ReportRequestResult(now, request, result_type);
}

void Backoff::Clear() {
  impl_->backoff_info_map_.clear();
}

void Backoff::SetData(uint64_t now, const char *data) {
  impl_->SetData(now, data);
}

std::string Backoff::GetData(uint64_t now) const {
  return impl_->GetData(now);
}

} // namespace ggadget
