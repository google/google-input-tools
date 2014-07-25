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

#include <iostream>
#include <algorithm>

#include <ppapi/cpp/instance.h>

#include "hanja.h"
#include "url_loader_util.h"

namespace {


// The number of items of Hanja Table. This is not needed to be a precise number
// It is just used as a hint when allocating memory
const int kTableNumItemsEstimation = 305000;

static void OnDataLoaded(void *hanja_lookup_ptr,
                         const string &url,
                         bool result,
                         string *buffer) {
  HanjaLookup *hanja_lookup = static_cast<HanjaLookup *>(hanja_lookup_ptr);
  hanja_lookup->LoadFromMemory(*buffer);
  // Release memory of buffer
  string("").swap(*buffer);
}

static bool ItemCmp(const HanjaLookup::Item &a, const HanjaLookup::Item &b) {
  return a.hangul < b.hangul;
}

}  // namespace

void HanjaLookup::LoadFromURL(const char *url) {
  loaded_ = false;
  URLLoaderUtil::StartDownload(
      instance_,
      url,
      &OnDataLoaded,
      this);
}

void HanjaLookup::LoadFromMemory(const string &memory) {
  loaded_ = false;
  if (items_.size() == 0) {
    // Reserve memory for faster loading
    items_.reserve(kTableNumItemsEstimation);
  }
  for (int offset = 0;;) {
    size_t new_line_pos = memory.find('\n', offset);
    // Skip empty lines or lines with start of #
    if (memory[offset] != '#' && memory[offset] != '\n') {
      bool append_succuss = AppendItem(memory, offset, new_line_pos);
      if (!append_succuss) {
        std::cerr << "Invalid hanja table: "
             << memory.substr(offset, new_line_pos - offset) << std::endl;
        return;
      }
    }
    // End of memory chunk
    if (new_line_pos == string::npos) {
      break;
    }
    offset = new_line_pos + 1;
  }
  std::sort(items_.begin(), items_.end(), ItemCmp);
  loaded_ = true;
}

bool HanjaLookup::AppendItem(const string &memory, int start, int end) {
  // Find first separator
  size_t sep_pos = memory.find(':', start);
  if (sep_pos == string::npos) {
    return false;
  }
  // Find second separator
  size_t sep_pos_2 = memory.find(':', sep_pos + 1);
  if (sep_pos_2 == string::npos) {
    return false;
  }
  // Split line
  Item item;
  item.hangul = memory.substr(start, sep_pos - start);
  item.hanja = memory.substr(sep_pos + 1, sep_pos_2 - sep_pos - 1);
  item.comment = memory.substr(sep_pos_2 + 1, end - sep_pos_2 - 1);
  items_.push_back(item);
  return true;
}

void HanjaLookup::Match(const string &hangul,
  std::vector<Item>::const_iterator *begin,
  std::vector<Item>::const_iterator *end) const {
  Item pattern;
  pattern.hangul = hangul;
  // Binary search: find the first item not less than the pattern
  *begin = std::lower_bound(items_.begin(), items_.end(), pattern, ItemCmp);
  // Binary search: find the first item greater than the pattern
  *end = std::upper_bound(items_.begin(), items_.end(), pattern, ItemCmp);
}
