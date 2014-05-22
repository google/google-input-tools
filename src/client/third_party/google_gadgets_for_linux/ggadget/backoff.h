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

#ifndef GGADGET_BACKOFF_H__
#define GGADGET_BACKOFF_H__

#include <string>
#include <ggadget/common.h>

namespace ggadget {

/**
 * @ingroup Utilities
 * Class to record service backoff information, to make sure the client
 * does not do a DDOS attack a service.
 * Uses modified truncated binary exponential backoff.
 * The wait unit count will be in the range
 * <code>2^(failure_count - 4) .. 2^(failure_count - 1)</code>
 * but with hard low and high limits.
 * For more information see:
 * http://en.wikipedia.org/wiki/Truncated_binary_exponential_backoff
 *
 * NOTE: The backoff and randomization features in this implementation is
 * very important for proper server-side operation. Please do *NOT* disable
 * or remove them.
 */
class Backoff {
 public:
  Backoff();
  ~Backoff();

  /**
   * Judges if it is safe to do a request now.
   * @param now the current time in milliseconds.
   * @param request identifier of the request. It can be the request URL or
   *     the host name of a URL. It can't contain '\n' or '\t' characters.
   * @return @c true if it's ok to do the request now.
   */
  bool IsOkToRequest(uint64_t now, const char *request);

  /**
   * Gets the next allowed time for a request.
   * If the request has not failed, returns 0.
   */
  uint64_t GetNextAllowedTime(const char *request);

  /**
   * Gets the number of continuous failures of a request.
   */
  int GetFailureCount(const char *request);

  enum ResultType {
    SUCCESS,
    EXPONENTIAL_BACKOFF,
    CONSTANT_BACKOFF,
  };

  /**
   * Indicate success or failure of a request. Always report after each
   * request to keep backoff data updated.
   * Returns @c true if the backoff data changed.
   */
  bool ReportRequestResult(uint64_t now, const char *request,
                           ResultType result_type);

  /**
   * Clear all backoff data (for testing only).
   */
  void Clear();

  /**
   * Gets/sets the string representation of the backoff data.
   * Useful to save/load backoff data into/from config file.
   * The data contains lines of records of service status.
   * Format of each line is like:
   * service\tlast_failure_time\tfailure_count
   */
  std::string GetData(uint64_t now) const;
  void SetData(uint64_t now, const char *data);

 private:
  DISALLOW_EVIL_CONSTRUCTORS(Backoff);
  class Impl;
  Impl *impl_;
};

} // namespace ggadget

#endif // GGADGET_BACKOFF_H__
