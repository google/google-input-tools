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

#include <cstring>
#include <algorithm>

#include <ggadget/backoff.h>
#include <ggadget/common.h>
#include <ggadget/string_utils.h>
#include <ggadget/options_interface.h>
#include "xml_http_request_utils.h"

namespace ggadget {

// field-value    = *( field-content | LWS )
// field-content  = <the OCTETs making up the field-value
//                  and consisting of either *TEXT or combinations
//                  of token, separators, and quoted-string>
// TEXT           = <any OCTET except CTLs, but including LWS>
bool IsValidHTTPHeaderValue(const char *s) {
  if (s == NULL) return true;
  while (*s) {
    if ( (*s > 0 && *s<=31
#if 0 // Disallow \r \n \t in header values.
         && *s != '\r' && *s != '\n' && *s != '\t'
#endif
         ) ||
         *s == 127)
      return false;
    ++s;
  }
  return true;
}

// field-name     = token
// token          = 1*<any CHAR except CTLs or separators>
// separators     = "(" | ")" | "<" | ">" | "@"
//                | "," | ";" | ":" | "\" | <">
//                | "/" | "[" | "]" | "?" | "="
//                | "{" | "}" | SP | HT
bool IsValidHTTPToken(const char *s) {
  if (s == NULL) return false;
  while (*s) {
    if (*s > 32 && *s < 127 &&
        (isalnum(*s) || strchr("!#$%&'*+ -.^_`~", *s) != NULL)) {
      //valid case
    } else {
      return false;
    }
    ++s;
  }
  return true;
}

static const char *kForbiddenHeaders[] = {
  "Accept-Charset",
  "Accept-Encoding",
  "Connection",
  "Content-Length",
  "Content-Transfer-Encoding",
  "Date",
  "Expect",
  "Host",
  "Keep-Alive",
  "Referer",
  "TE",
  "Trailer",
  "Transfer-Encoding",
  "Upgrade",
  "Via"
};

static const char *kUniqueHeaders[] = {
  "Content-Type"
};

bool IsForbiddenHeader(const char *header) {
  if (strncasecmp("Proxy-", header, 6) == 0 ||
      strncasecmp("Sec-", header, 4) == 0) {
    return true;
  }

  const char **found = std::lower_bound(
      kForbiddenHeaders, kForbiddenHeaders + arraysize(kForbiddenHeaders),
      header, CaseInsensitiveCharPtrComparator());
  if (found != kForbiddenHeaders + arraysize(kForbiddenHeaders) &&
      strcasecmp(*found, header) == 0)
    return true;
  return false;
}

bool IsUniqueHeader(const char *header) {
  const char **found = std::lower_bound(
      kUniqueHeaders, kUniqueHeaders + arraysize(kUniqueHeaders),
      header, CaseInsensitiveCharPtrComparator());
  if (found != kUniqueHeaders + arraysize(kUniqueHeaders) &&
      strcasecmp(*found, header) == 0)
    return true;
  return false;
}

bool SplitStatusFromResponseHeaders(std::string *response_headers,
                                    std::string *status_text) {
  // RFC 2616 doesn't mentioned if HTTP/1.1 is case-sensitive, so implies
  // it is case-insensitive.
  // We only support HTTP version 1.0 or above.
  if (strncasecmp(response_headers->c_str(), "HTTP/", 5) == 0) {
    // First split the status line and headers.
    std::string::size_type end_of_status = response_headers->find("\r\n", 0);
    if (end_of_status == std::string::npos) {
      *status_text = *response_headers;
      response_headers->clear();
    } else {
      *status_text = response_headers->substr(0, end_of_status);
      response_headers->erase(0, end_of_status + 2);
      size_t header_size = response_headers->size();
      // Remove ending extra "\r\n".
      if (header_size > 4 &&
          strcmp(response_headers->c_str() + header_size - 4, "\r\n\r\n") == 0)
        response_headers->erase(header_size - 2);
    }

    // Then extract the status text from the status line.
    std::string::size_type space_pos = status_text->find(' ', 0);
    if (space_pos != std::string::npos) {
      space_pos = status_text->find(' ', space_pos + 1);
      if (space_pos != std::string::npos)
        status_text->erase(0, space_pos + 1);
    }

    // Otherwise, just leave the whole status line in status.
    return true;
  }
  // Otherwise already splitted.
  return false;
}

void ParseResponseHeaders(const std::string &response_headers,
                          CaseInsensitiveStringMap *response_headers_map,
                          std::string *response_content_type,
                          std::string *response_encoding) {
  // http://www.w3.org/Protocols/rfc2616/rfc2616-sec4.html#sec4.2
  // http://www.w3.org/TR/XMLHttpRequest
  std::string::size_type pos = 0;
  while (pos != std::string::npos) {
    std::string::size_type new_pos = response_headers.find("\r\n", pos);
    std::string line;
    if (new_pos == std::string::npos) {
      line = response_headers.substr(pos);
      pos = new_pos;
    } else {
      line = response_headers.substr(pos, new_pos - pos);
      pos = new_pos + 2;
    }

    std::string name, value;
    std::string::size_type pos = line.find(':');
    if (pos != std::string::npos) {
      name = TrimString(line.substr(0, pos));
      value = TrimString(line.substr(pos + 1));
      if (!name.empty()) {
        CaseInsensitiveStringMap::iterator it =
            response_headers_map->find(name);
        if (it == response_headers_map->end()) {
          response_headers_map->insert(std::make_pair(name, value));
        } else if (!value.empty()) {
          // According to XMLHttpRequest specification, the values of multiple
          // headers with the same name should be concentrated together.
          if (!it->second.empty())
            it->second += ", ";
          it->second += value;
        }
      }

      if (strcasecmp(name.c_str(), "Content-Type") == 0) {
        // Extract content type and encoding from the headers.
        pos = value.find(';');
        if (pos != std::string::npos) {
          *response_content_type = TrimString(value.substr(0, pos));
          pos = value.find("charset");
          if (pos != std::string::npos) {
            pos += 7;
            while (pos < value.length() &&
                   (isspace(value[pos]) || value[pos] == '=')) {
              pos++;
            }
            std::string::size_type pos1 = pos;
            while (pos1 < value.length() && !isspace(value[pos1]) &&
                   value[pos1] != ';') {
              pos1++;
            }
            *response_encoding = value.substr(pos, pos1 - pos);
          }
        } else {
          *response_content_type = value;
        }
      }
    }
  }
}

// The name of the options to store backoff data.
static const char kBackoffOptions[] = "backoff";
// The name of the options item to store backoff data.
static const char kBackoffDataOption[] = "backoff";
static Backoff g_backoff;
static OptionsInterface *g_backoff_options = NULL;

static Backoff::ResultType GetBackoffType(unsigned short status) {
  // status == 0: network error, don't do exponential backoff.
  return status == 0 ? Backoff::CONSTANT_BACKOFF :
      (status >= 100 && status < 500) ? Backoff::SUCCESS :
      Backoff::EXPONENTIAL_BACKOFF;
}

bool EnsureXHRBackoffOptions(uint64_t now) {
  if (!g_backoff_options) {
    g_backoff_options = CreateOptions(kBackoffOptions);
    if (g_backoff_options) {
      std::string data;
      Variant value = g_backoff_options->GetValue(kBackoffDataOption);
      if (value.ConvertToString(&data))
        g_backoff.SetData(now, data.c_str());
    }
  }
  return g_backoff_options != NULL;
}

void SaveXHRBackoffData(uint64_t now) {
  if (EnsureXHRBackoffOptions(now)) {
    g_backoff_options->PutValue(kBackoffDataOption,
                                Variant(g_backoff.GetData(now)));
    g_backoff_options->Flush();
  }
}

bool IsXHRBackoffRequestOK(uint64_t now, const char *request) {
  return g_backoff.IsOkToRequest(now, request);
}

bool XHRBackoffReportResult(uint64_t now,
                            const char *request,
                            unsigned short status) {
  return g_backoff.ReportRequestResult(now, request, GetBackoffType(status));
}

} // end of namespace ggadget
