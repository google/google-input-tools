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

#ifndef HANJA_H_
#define HANJA_H_

#include <string>
#include <vector>

using std::string;

class HanjaLookup {
 public:
  // A struct describing item in Hanja Table
  // hangul is the key of the table, which means korean character
  // hanja means Chinese characters
  // comments contains the description of this item
  struct Item {
    string hangul;
    string hanja;
    string comment;
  };

  explicit HanjaLookup(pp::Instance *instance) {
    instance_ = instance;
    loaded_ = false;
  }
  virtual ~HanjaLookup() {}

  // Returns true if dictionary is loaded
  bool loaded() const {
    return loaded_;
  }

  // Loads dictionary from url
  // Note that this is a asynchronous method which will return immediately when
  // calling. You must wait for the return value of loaded() to be true before
  // you use other methods.
  // @param[in] url The URL of dictionary file. If url is set to NULL, default
  //                URL will be used.
  void LoadFromURL(const char *url);

  // Loads dictionary from memory chunk
  // @param[in] memory The memory chunk which points to dictionary text.
  void LoadFromMemory(const string &memory);

  // Matches hanja candidates with hangul characters
  // @param[in]  hangul The key of hanja table.
  // @param[out] begin  The begin iterator of matched results.
  // @param[out] end    The end iterator.
  void Match(const string &hangul,
             std::vector<Item>::const_iterator *begin,
             std::vector<Item>::const_iterator *end) const;

 private:
  bool AppendItem(const string &memory, int start, int end);

  bool loaded_;
  std::vector<Item> items_;
  pp::Instance *instance_;
};

#endif  // HANJA_H_
