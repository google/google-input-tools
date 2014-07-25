// Copyright 2014 The ChromeOS IME Authors. All Rights Reserved.
// limitations under the License.
// See the License for the specific language governing permissions and
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// distributed under the License is distributed on an "AS-IS" BASIS,
// Unless required by applicable law or agreed to in writing, software
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// You may obtain a copy of the License at
// you may not use this file except in compliance with the License.
// Licensed under the Apache License, Version 2.0 (the "License");
//
/*
 * Copyright 2013 Google Inc. All Rights Reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef UNICODE_UTIL_H_
#define UNICODE_UTIL_H_

#include <string>

using std::string;

typedef uint32_t ucschar;
typedef std::basic_string<ucschar> UCSString;

namespace unicode_util {

// Converts UCS4 string into UTF-8 format
// @param[in] ucs4    The UCS4 string.
// @param[in] length  The length of UCS4 string. If length is set to 0, ucs4
//                    will be considered as a C-style string ending by 0.
// @return            The converted UTF-8 string.
string Ucs4ToUtf8(const ucschar* ucs4, size_t length);

// Converts UTF-8 string into UCS4 format
// @param[in] utf8    The UTF-8 string.
// @param[in] length  The length of UTF-8 string. If length is set to 0, utf8
//                    will be considered as a C-style string ending by 0.
// @return            The converted UCS4 string.
UCSString Utf8ToUcs4(const char* utf8, size_t length);

}   // namespace unicode_util

#endif  // UNICODE_UTIL_H_
