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

#ifndef URL_LOADER_UTIL_H_
#define URL_LOADER_UTIL_H_

#include <string>

using std::string;

namespace pp {
class Instance;
}  // namespace pp

// Callback function type of StartDownload.
typedef void (*DownloadCallback)(void *data_user,
                                 const string &url,
                                 bool result,
                                 string *data_buffer);

// Utility class to handle pp::URLLoader.
//
// Usage:
//  - Defines a callback function.
//   void OnDataLoaded(void *user_data, bool result, string *buffer) {
//     LOG(INFO) << "Result: " << result;
//     LOG(INFO) << "Data: " << *buffer;
//   }
//
//  - Calls URLLoaderUtil::StartDownload().
//   void SampleClass::DoLoad() {
//     URLLoaderUtil::StartDownload(
//         instance_,
//         "http//www.google.com/",
//         &OnDataLoaded,
//         this);
//   }
//
// Notes:
// - user_data which is set in the constructor will be passed to the callback.
// - URLLoaderHandler object will be deleted after OnDataLoaded() is called.
// - You can call data_buffer->swap() in the callback function to reduce memory
//   copy operation.

class URLLoaderUtil {
 public:
  static void StartDownload(pp::Instance *instance,
                            const string &url,
                            DownloadCallback callback,
                            void *data_user);
};

#endif  // URL_LOADER_UTIL_H_
