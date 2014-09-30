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

#include "text_range/text_range_reconvert.h"

#include "base/logging.h"
#include "common/debug.h"

namespace ime_goopy {
static const int kReconvertBufSize = 256;

TextRangeReconvert::TextRangeReconvert(
    TextRangeInterface* text_range,
    EngineReconvertCallback* engine_reconvert_callback,
    ContextReconvertCallback* context_reconvert_callback,
    ShouldReconvertCallback* should_reconvert_callback)
    : text_range_(text_range),
      engine_reconvert_callback_(engine_reconvert_callback),
      context_reconvert_callback_(context_reconvert_callback),
      should_reconvert_callback_(should_reconvert_callback) {
}

bool TextRangeReconvert::IsTextCanBeReconverted(const wstring& text) {
  TextManipulatorEn en_manipulator;
  TextManipulatorZhCn zh_manipulator;
  for (int index = 0; index < text.size(); ++index) {
    if (!en_manipulator.IsValidCharInSentence(text[index])) {
      if (!zh_manipulator.IsValidCharInSentence(text[index])) {
        return false;
      }
    }
  }
  return true;
}

bool TextRangeReconvert::Reconvert(ReconvertAlignType align_type) {
  DVLOG(3) << __SHORT_FUNCTION__ << " align_type: " << align_type;

  // TODO(zengjian): Implement sentense alignment.
  DCHECK_EQ(ALIGN_TO_WORD, align_type);

  if (!text_range_)
    return false;

  wstring selection_text = text_range_->GetText();
  // Check validity of selection text.
  if (!selection_text.empty() && !IsTextCanBeReconverted(selection_text)) {
    return false;
  }

  int left_offset = 0;
  int right_offset = 0;

  scoped_ptr<TextRangeInterface> left_range(text_range_->Clone());
  left_range->CollapseToStart();
  left_range->ShiftStart(-kReconvertBufSize, NULL);
  wstring left_text = left_range->GetText();

  scoped_ptr<TextRangeInterface> right_range(text_range_->Clone());
  right_range->CollapseToEnd();
  right_range->ShiftEnd(kReconvertBufSize, NULL);
  wstring right_text = right_range->GetText();

  // Try expanding to an English word.
  {
    TextManipulatorEn en_manipulator_;
    if (selection_text.empty() ||
        en_manipulator_.IsValidCharInWord(selection_text[0])) {
      left_offset = en_manipulator_.ExpandToWordBegin(
          left_text, left_text.size());
    }
    if (selection_text.empty() ||
        en_manipulator_.IsValidCharInWord(
            selection_text[selection_text.length() - 1])) {
      right_offset = en_manipulator_.ExpandToWordEnd(right_text, 0);
    }
    if (left_offset > 0 )
      text_range_->ShiftStart(-left_offset, NULL);
    if (right_offset > 0 )
      text_range_->ShiftEnd(right_offset, NULL);
  }

  // If empty, try expanding to a Unicode word.
  if (text_range_->IsEmpty()) {
    DCHECK_EQ(0, left_offset);
    DCHECK_EQ(0, right_offset);
    TextManipulatorZhCn zh_manipulator_;
    left_offset = zh_manipulator_.ExpandToWordBegin(
        left_text, left_text.size());
    right_offset = zh_manipulator_.ExpandToWordEnd(right_text, 0);
    text_range_->ShiftStart(-left_offset, NULL);
    text_range_->ShiftEnd(right_offset, NULL);
  }

  wstring text = text_range_->GetText();
  DVLOG(3) << __SHORT_FUNCTION__ << L"range->GetText: " << text;
  DVLOG(3) << __SHORT_FUNCTION__ << L"left_offset: " << left_offset;

  if (should_reconvert_callback_ &&
      !should_reconvert_callback_->Run(text)) {
    // Should delete callbacks that are not invoked.
    if (context_reconvert_callback_ &&
        !context_reconvert_callback_->IsRepeatable()) {
      delete context_reconvert_callback_;
    }
    if (engine_reconvert_callback_ &&
        !engine_reconvert_callback_->IsRepeatable()) {
      delete engine_reconvert_callback_;
    }
    return false;
  }

  if (context_reconvert_callback_)
    context_reconvert_callback_->Run();
  if (engine_reconvert_callback_)
    engine_reconvert_callback_->Run(text, left_offset);
  return true;
}
}  // namespace ime_goopy
