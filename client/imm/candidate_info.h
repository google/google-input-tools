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

#ifndef GOOPY_IMM_CANDIDATE_INFO_H__
#define GOOPY_IMM_CANDIDATE_INFO_H__

#include "base/basictypes.h"
#include "imm/context_locker.h"
#include "imm/immdev.h"
#include <gtest/gtest_prod.h>

namespace ime_goopy {
namespace imm {
struct CandidateInfo {
 public:
  bool Initialize();
  void set_selection(int index);
  void set_count(int count);
  int count() { return list.info.dwCount; }
  void set_candidate(int index, const wstring& value);

 private:
  static const int kMaxCandidateLength = 500;
  static const int kMaxCandidateCount = 10;

  struct CandidateList {
    CANDIDATELIST info;
    DWORD offsets[kMaxCandidateCount - 1];
    wchar_t text[kMaxCandidateCount][kMaxCandidateLength];
  };

  CANDIDATEINFO info;
  CandidateList list;

  FRIEND_TEST(CandidateInfo, Test);
  FRIEND_TEST(ContextTest, Update);
};

}  // namespace imm
}  // namespace ime_goopy
#endif  // GOOPY_IMM_CANDIDATE_INFO_H__
