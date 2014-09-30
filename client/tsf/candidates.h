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

#ifndef GOOPY_TSF_CANDIDATES_H_
#define GOOPY_TSF_CANDIDATES_H_

#include <atlbase.h>
#include <atlcom.h>
#include <msctf.h>

#include <string>
#include "base/basictypes.h"
#include "common/smart_com_ptr.h"
#include "ipc/protos/ipc.pb.h"

namespace ime_goopy {
class EngineInterface;
class UIManagerInterface;
namespace tsf {
class ExternalCandidateUI;
class Candidates {
 public:
  Candidates(ITfThreadMgr* thread_manager,
             EngineInterface* engine,
             UIManagerInterface* ui_manager);
  ~Candidates();

  HRESULT Update(const ipc::proto::CandidateList& candidate_list);
  bool ShouldShow() { return status_ == STATUS_SHOWN_UI_MANAGER; }
  bool IsEmpty();
  const ipc::proto::CandidateList& candidate_list() { return candidate_list_; }

 private:
  enum Status {
    STATUS_HIDDEN,
    STATUS_SHOWN_EXTERNAL,    // Candidates are shown by tsf external ui.
    STATUS_SHOWN_UI_MANAGER,  // Candidates are shown by ui_manager.
  };
  EngineInterface* engine_;
  UIManagerInterface* ui_manager_;
  SmartComObjPtr<ExternalCandidateUI> external_candidate_ui_;
  Status status_;
  ipc::proto::CandidateList candidate_list_;
  DISALLOW_EVIL_CONSTRUCTORS(Candidates);
};
}  // namespace tsf
}  // namespace ime_goopy
#endif  // GOOPY_TSF_CANDIDATES_H_
