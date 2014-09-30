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

#include "components/common/mock_decoder.h"

#include "i18n/input/engine/t13n/public/data_manager_interface.h"
#include "base/logging.h"

namespace {
using i18n_input::engine::t13n::DecodeRequest;
using i18n_input::engine::t13n::DecodeResponse;
}  // namespace

namespace ime_goopy {
namespace components {

static const int kCandidateCount = 10;

MockDecoder::MockDecoder() {
}

MockDecoder::~MockDecoder() {
}

void MockDecoder::Decode(const DecodeRequest& request,
    DecodeResponse* response) const {
  DCHECK_GT(request.source_segments.size(), 0);
  const DecodeRequest::SourceSegment& segment = request.source_segments[0];
  DCHECK(!segment.is_context);
  DecodeResponse::Candidate candidate;
  DecodeResponse::CandidateList candidate_list;
  std::string text;
  for (int i = 0; i < kCandidateCount; ++i) {
    text += segment.current_text;
    candidate.transliteration_text = text;
    candidate.score = 1.0;
    // 10 repeated candidate
    candidate_list.candidates.push_back(candidate);
  }
  candidate_list.source_segment_index = 0;
  response->candidate_lists.push_back(candidate_list);
}

}  // namespace components
}  // namespace ime_goopy
