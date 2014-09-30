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

#ifndef GOOPY_TEXT_RANGE_TEXT_RANGE_RECONVERT_H_
#define GOOPY_TEXT_RANGE_TEXT_RANGE_RECONVERT_H_

#include "base/scoped_ptr.h"
#include "common/framework_interface.h"
#include "locale/text_utils.h"

namespace ime_goopy {
// This class is called when reconversion happens in normal text editor.
class TextRangeReconvert {
 public:
  TextRangeReconvert(TextRangeInterface* text_range,
                     EngineReconvertCallback* engine_reconvert_callback,
                     ContextReconvertCallback* context_reconvert_callback,
                     ShouldReconvertCallback *should_reconvert_callback);
  bool Reconvert(ReconvertAlignType align_type);
 private:
  bool IsTextCanBeReconverted(const wstring& text);
  scoped_ptr<TextManipulator> text_manipulator_;
  EngineReconvertCallback* engine_reconvert_callback_;
  ContextReconvertCallback* context_reconvert_callback_;
  ShouldReconvertCallback* should_reconvert_callback_;
  TextRangeInterface* text_range_;
  DISALLOW_COPY_AND_ASSIGN(TextRangeReconvert);
};
}  // namespace ime_goopy
#endif  // GOOPY_TEXT_RANGE_TEXT_RANGE_RECONVERT_H_
