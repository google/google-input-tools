/*
  Copyright 2014 Google Inc.

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

#include <stddef.h>
#include <windows.h>  // windows.h must be included before strsafe.h
#include <strsafe.h>

#include "imm/candidate_info.h"

namespace ime_goopy {
namespace imm {
void CandidateInfo::set_count(int count) {
  list.info.dwCount = count;
  list.info.dwPageSize = count;
}

void CandidateInfo::set_selection(int index) {
  list.info.dwSelection = index;
}

void CandidateInfo::set_candidate(int index, const wstring& value) {
  assert(index < kMaxCandidateCount);
  if (index < kMaxCandidateCount) {
    StringCchCopyN(list.text[index],
                   kMaxCandidateLength,
                   value.c_str(),
                   kMaxCandidateLength);
  }
}

bool CandidateInfo::Initialize() {
  ZeroMemory(this, sizeof(*this));
  info.dwSize = sizeof(CandidateInfo);
  info.dwCount = 1;
  info.dwOffset[0] = offsetof(CandidateInfo, list);

  list.info.dwSize = sizeof(list);
  list.info.dwStyle = IME_CAND_READ;
  list.info.dwPageSize = kMaxCandidateCount;
  for (int i = 0; i < kMaxCandidateCount; i++) {
    if (i == 0) {
      list.info.dwOffset[i] = offsetof(CandidateInfo::CandidateList, text);
    } else {
      list.info.dwOffset[i] =
        list.info.dwOffset[i - 1] + sizeof(wchar_t) * kMaxCandidateLength;
    }
  }
  return true;
}

}  // namespace imm
}  // namespace ime_goopy
