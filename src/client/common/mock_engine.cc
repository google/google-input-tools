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

#include "common/mock_engine.h"

namespace ime_goopy {
const wchar_t MockEngine::kTestComposition[] = L"composition";
const int MockEngine::kTestCompositionLength = arraysize(kTestComposition) - 1;
const wchar_t MockEngine::kTestResult[] = L"result";
const int MockEngine::kTestResultLength = arraysize(kTestResult) - 1;
const wchar_t MockEngine::kTestCandidate1[] = L"candidate1";
const wchar_t MockEngine::kTestCandidate2[] = L"candidate2";
const int MockEngine::kTestCaret = 3;

MockEngine::MockEngine() : page_(new CandidatePage()) {
  Reset();
}

MockEngine::~MockEngine() {
  Reset();
}

// EngineInterface Actions.
bool MockEngine::ShouldProcessKey(const KeyStroke& key) {
  commands_.push_back(new ShouldProcessKeyCommand(key));
  return true;
}

bool MockEngine::ProcessCommand(const CommandInterface& command) {
  commands_.push_back(command.Clone());
  return true;
}

void MockEngine::Reset() {
  candidates_.clear();
  candidates_.push_back(kTestCandidate1);
  candidates_.push_back(kTestCandidate2);
  composition_ = kTestComposition;
  result_ = kTestResult;
  for (int i = 0; i < commands_.size(); i++) {
    delete commands_[i];
  }
  commands_.clear();
  page_->Reset(static_cast<int>(candidates_.size()));
}

}  // namespace ime_goopy
