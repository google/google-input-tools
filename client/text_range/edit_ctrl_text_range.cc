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

#include "text_range/edit_ctrl_text_range.h"
#include <windows.h>
#include <atlbase.h>
#include <atlapp.h>
#include <atlctrls.h>
#include "base/logging.h"
#include "base/scoped_ptr.h"
#include "text_range/window_utils.h"

namespace ime_goopy {

EditCtrlTextRange *EditCtrlTextRange::CreateFromSelection(
    Callback1<EditCtrlTextRange*> *on_reconvert, HWND handle) {
  if (!IsWindow(handle)) {
    DCHECK(false);
    return NULL;
  }
  if (!IsEditControl(handle))
    return NULL;
  CEdit edit(handle);
  int begin = 0, end = 0;
  edit.GetSel(begin, end);
  if (begin > end)
    swap(begin, end);
  return new EditCtrlTextRange(on_reconvert, handle, begin, end);
}

EditCtrlTextRange::EditCtrlTextRange(
    Callback1<EditCtrlTextRange*> *on_reconvert,
    HWND handle, int begin, int end)
    : on_reconvert_(on_reconvert), handle_(handle), begin_(begin), end_(end) {
}

wstring EditCtrlTextRange::GetText() {
  CEdit edit(handle_);
  int length = edit.GetWindowTextLength();
  scoped_array<wchar_t> buffer(new wchar_t[length + 1]);
  edit.GetWindowText(buffer.get(), length + 1);
  return std::wstring(buffer.get() + begin_, end_ - begin_);
}

void EditCtrlTextRange::ShiftStart(int offset, int *actual_offset) {
  if (begin_ + offset < 0)
    offset = -begin_;
  if (begin_ + offset > end_)
    offset = end_ - begin_;
  begin_ += offset;
  if (actual_offset)
    *actual_offset = offset;
}

void EditCtrlTextRange::ShiftEnd(int offset, int *actual_offset) {
  CEdit edit(handle_);
  int length = edit.GetWindowTextLength();
  if (end_ + offset > length)
    offset = length - end_;
  if (end_ + offset < begin_)
    offset = begin_ - end_;
  end_ += offset;
  if (actual_offset)
    *actual_offset = offset;
}

void EditCtrlTextRange::CollapseToStart() {
  end_ = begin_;
}

void EditCtrlTextRange::CollapseToEnd() {
  begin_ = end_;
}

bool EditCtrlTextRange::IsEmpty() {
  return begin_ == end_;
}

bool EditCtrlTextRange::IsContaining(TextRangeInterface *inner_range) {
  if (!inner_range) {
    DCHECK(false);
    return false;
  }
  EditCtrlTextRange *casted_inner_range =
      down_cast<EditCtrlTextRange*>(inner_range);
  if (!casted_inner_range) {
    DCHECK(false);
    return false;
  }
  return casted_inner_range->begin_ >= begin_ &&
         casted_inner_range->end_ <= end_;
}

void EditCtrlTextRange::Reconvert() {
  CEdit edit(handle_);
  edit.SetSel(begin_, end_, FALSE);
  if (on_reconvert_)
    on_reconvert_->Run(this);
}

TextRangeInterface *EditCtrlTextRange::Clone() {
  return new EditCtrlTextRange(on_reconvert_, handle_, begin_, end_);
}

void EditCtrlTextRange::Select() {
  CEdit edit(handle_);
  edit.SetSel(begin_, end_, FALSE);
}

POINT EditCtrlTextRange::GetCompositionPos() {
  CEdit edit(handle_);
  POINT point = edit.PosFromChar(begin_);
  edit.ClientToScreen(&point);
  return point;
}
}  // namespace ime_goopy
