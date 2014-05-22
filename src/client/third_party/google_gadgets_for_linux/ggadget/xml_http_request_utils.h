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

#ifndef GGADGET_XML_HTTP_REQUEST_UTILS_H__
#define GGADGET_XML_HTTP_REQUEST_UTILS_H__

#include <string>

namespace ggadget {

/**
 * @defgroup XHRUtilities XML Http Request utilities
 * @ingroup Utilities
 * @{
 */

bool IsValidHTTPHeaderValue(const char *s);

bool IsValidHTTPToken(const char *s);
/** Checks if header is forbidden. */
bool IsForbiddenHeader(const char *header);

/** Checks if a header should be unique. */
bool IsUniqueHeader(const char *header);

/**
 * Splits status information from http response @response.
 * Status will be removed from @response and stored in @status_text.
 *
 * If @response is already splitted, nothing is done and this function
 * returns false.
 */
bool SplitStatusFromResponseHeaders(std::string *response,
                                    std::string *status_text);

/**
 * Parse response headers and setup @response_headers_map,
 * @response_content_type and @response_encoding.
 */
void ParseResponseHeaders(const std::string &response_headers,
                          CaseInsensitiveStringMap *response_headers_map,
                          std::string *response_content_type,
                          std::string *response_encoding);

/** Makes sure backoff options is created. */
bool EnsureXHRBackoffOptions(uint64_t now);

/** Saves backoff data into options. */
void SaveXHRBackoffData(uint64_t now);

/** Checks if @request is allowed by backoff policy. */
bool IsXHRBackoffRequestOK(uint64_t now, const char *request);

/** Reports if the request is failed of successful to backoff. */
bool XHRBackoffReportResult(uint64_t now, const char *request,
                            unsigned short status);
/** @} */

} // end of namespace ggadget

#endif // GGADGET_XML_HTTP_REQUEST_UTILS_H__
