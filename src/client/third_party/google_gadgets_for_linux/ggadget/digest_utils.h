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

#ifndef GGADGET_DIGEST_UTILS_H__
#define GGADGET_DIGEST_UTILS_H__

#include <string>

namespace ggadget {

/**
 * @defgroup DigestUtilities Digest utilities
 * @ingroup Utilities
 * @{
 */

/**
 * Generate result using Secure Hash Algorithm-1.
 * @param input input data. Can contain arbitary binary data.
 * @param[out] result the SHA-1 result (20 bytes).
 * @return @c true if succeeds.
 */
bool GenerateSHA1(const std::string &input, std::string *result);

/**
 * Encode the input data into a base64 string.
 * @param input input data. Can contain arbitary binary data.
 * @param add_padding controls whether add paddings at the end of the result.
 * @param[out] result the Base64 result.
 * @return @c true if succeeds.
 */
bool EncodeBase64(const std::string &input, bool add_padding,
                  std::string *result);

/**
 * Decode a Base64 string. Paddings are not enforced.
 * @param input the input Base64 string.
 * @param[out] decoded result.
 * @return @c true if succeeds.
 */
bool DecodeBase64(const char *input, std::string *result);

/**
 * Encode the input data into a Base64 string in WebSafe format.
 * The WebSafe format use '-' instead of '+' and '_' instead of '/' so that
 * the string can be placed into URLs and MIME headers without having to
 * escape them.
 */
bool WebSafeEncodeBase64(const std::string &input, bool add_padding,
                         std::string *result);

/**
 * Decode a Base64 string in WebSafe format.
 */
bool WebSafeDecodeBase64(const char *input, std::string *result);

/** @} */

} // namespace ggadget

#endif  // GGADGET_DIGEST_UTILS_H__
