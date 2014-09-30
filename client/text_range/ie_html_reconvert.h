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

#ifndef GOOPY_TEXT_RANGE_IE_HTML_RECONVERT_H_
#define GOOPY_TEXT_RANGE_IE_HTML_RECONVERT_H_

#include "common/framework_interface.h"
#include "text_range/html_text_range.h"

namespace ime_goopy {
// This class is called when reconversion happens in IE.
class IEHtmlReconvert {
 public:
  IEHtmlReconvert(Callback1<HtmlTextRange*> *on_reconvert,
                  EngineReconvertCallback* engine_reconvert_callback,
                  ShouldReconvertCallback* should_reconvert_callback,
                  HtmlTextRange* html_text_range);
  bool Reconvert(ReconvertAlignType align_type);
 private:
  bool CalculateMoveDistance(const wstring& html_text,
                             ReconvertAlignType align_type,
                             int* left_move,
                             int* right_move);
  void AdjustLeftText(const wstring& html_text, wstring* left_text);
  void AdjustRightText(const wstring& html_text, wstring* right_text);
  Callback1<HtmlTextRange*> *on_reconvert_;
  EngineReconvertCallback* engine_reconvert_callback_;
  ShouldReconvertCallback* should_reconvert_callback_;
  HtmlTextRange* html_text_range_;
  DISALLOW_COPY_AND_ASSIGN(IEHtmlReconvert);
};
}  // namespace ime_goopy
#endif  // GOOPY_TEXT_RANGE_IE_HTML_RECONVERT_H_
