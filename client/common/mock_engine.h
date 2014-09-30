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

#ifndef GOOPY_COMMON_MOCK_ENGINE_H__
#define GOOPY_COMMON_MOCK_ENGINE_H__

#include <atlbase.h>
#include <atltypes.h>

#include "base/basictypes.h"
#include "common/framework_interface.h"
//#include "imm/immdev.h"
//#include <gtest/gunit.h>

namespace ime_goopy {
class MockEngine : public EngineInterface {
 public:
  static const wchar_t kTestComposition[];
  static const int kTestCompositionLength;
  static const wchar_t kTestResult[];
  static const int kTestResultLength;
  static const wchar_t kTestCandidate1[];
  static const wchar_t kTestCandidate2[];
  static const int kTestCaret;

  MockEngine();
  virtual ~MockEngine();

  // EngineInterface Actions.
  bool ShouldProcessKey(const KeyStroke& key);
  bool ProcessCommand(const CommandInterface& command);

  // EngineInterface Information.
  const wstring& composition() { return composition_; }
  int caret() { return kTestCaret; }
  const wstring& result() { return result_; }
  int GetCandidateCount() { return static_cast<int>(candidates_.size()); }
  const wstring& GetCandidate(int index) { return candidates_[index]; }
  CandidatePage* GetCandidatePage() { return page_.get(); }
  DWORD GetStatus(Status status) { return 0; }
  void SetStatus(Status status, DWORD value) { }
  void RefreshQuickLocatingState() { }
  bool ImportDictionary(const wchar_t* file_name) { return false; }
  const vector<CommandInterface*>& commands() {
    return commands_;
  }

  void Reset();
 private:
  vector<wstring> candidates_;
  vector<CommandInterface*> commands_;
  wstring composition_;
  wstring result_;
  scoped_ptr<CandidatePage> page_;
  DISALLOW_EVIL_CONSTRUCTORS(MockEngine);
};
}  // namespace ime_goopy
#endif  // GOOPY_COMMON_MOCK_ENGINE_H__
