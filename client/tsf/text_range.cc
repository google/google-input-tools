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

#include "tsf/text_range.h"

#include "base/logging.h"
#include "common/debug.h"
#include "common/singleton.h"
#include "tsf/composition_event_sink.h"
#include "tsf/context_event_sink.h"
#include "tsf/edit_session.h"

namespace ime_goopy {
namespace tsf {

TextRange::TextRange(ContextEventSink *context_event_sink, ITfRange *range)
    : context_event_sink_(context_event_sink), range_(range) {
}

wstring TextRange::GetText() {
  if (write_cookie() == TF_INVALID_COOKIE) {
    DCHECK(false);
    return L"";
  }
  static const int kBufferLength = 1000;
  wchar_t buffer[kBufferLength + 1] = { 0 };
  ULONG length = 0;
  HRESULT hr = range_->GetText(write_cookie(),
                               0, buffer, kBufferLength, &length);
  if (FAILED(hr)) {
    DVLOG(1) << "ITfRange::GetText failed in "
             << __SHORT_FUNCTION__;
    return L"";
  }
  return wstring(buffer, length);
}

void TextRange::ShiftStart(int offset, int *actual_offset) {
  if (write_cookie() == TF_INVALID_COOKIE) {
    DCHECK(false);
    return;
  }
  LONG actual_offset_long = 0;
  HRESULT hr = range_->ShiftStart(
      write_cookie(), offset, &actual_offset_long, NULL);
  if (FAILED(hr)) {
    DVLOG(1) << "ITfRange::ShiftStart failed in "
             << __SHORT_FUNCTION__;
    return;
  }
  if (actual_offset)
    *actual_offset = actual_offset_long;
}

void TextRange::ShiftEnd(int offset, int *actual_offset) {
  if (write_cookie() == TF_INVALID_COOKIE) {
    DCHECK(false);
    return;
  }
  LONG actual_offset_long = 0;
  HRESULT hr = range_->ShiftEnd(
      write_cookie(), offset, &actual_offset_long, NULL);
  if (FAILED(hr)) {
    DVLOG(1) << "ITfRange::ShiftEnd failed in "
             << __SHORT_FUNCTION__;
    return;
  }
  if (actual_offset)
    *actual_offset = actual_offset_long;
}

void TextRange::CollapseToStart() {
  if (write_cookie() == TF_INVALID_COOKIE) {
    DCHECK(false);
    return;
  }
  HRESULT hr = range_->Collapse(write_cookie(), TF_ANCHOR_START);
  if (FAILED(hr)) {
    DVLOG(1) << "ITfRange::Collapse failed in "
             << __SHORT_FUNCTION__;
  }
}

void TextRange::CollapseToEnd() {
  if (write_cookie() == TF_INVALID_COOKIE) {
    DCHECK(false);
    return;
  }
  HRESULT hr = range_->Collapse(write_cookie(), TF_ANCHOR_END);
  if (FAILED(hr)) {
    DVLOG(1) << "ITfRange::Collapse failed in "
             << __SHORT_FUNCTION__;
  }
}

bool TextRange::IsEmpty() {
  if (write_cookie() == TF_INVALID_COOKIE) {
    DCHECK(false);
    return true;
  }
  BOOL empty = FALSE;
  HRESULT hr = range_->IsEmpty(write_cookie(), &empty);
  if (FAILED(hr)) {
    DVLOG(1) << "ITfRange::IsEmpty failed in "
             << __SHORT_FUNCTION__;
    return true;
  }
  return empty != FALSE;
}

bool TextRange::IsContaining(TextRangeInterface *inner_range) {
  if (write_cookie() == TF_INVALID_COOKIE || !inner_range) {
    DCHECK(false);
    return false;
  }
  ITfRange *inner_tsf_range =
      static_cast<TextRange*>(inner_range)->range_;
  LONG comp_start = 0;
  HRESULT hr = range_->CompareStart(
      write_cookie(), inner_tsf_range, TF_ANCHOR_START, &comp_start);
  if (FAILED(hr)) {
    DVLOG(1) << "ITfRange::CompareStart failed in "
             << __SHORT_FUNCTION__;
    return false;
  }
  if (comp_start > 0)
    return false;

  LONG comp_end = 0;
  hr = range_->CompareEnd(
      write_cookie(), inner_tsf_range, TF_ANCHOR_END, &comp_end);
  if (FAILED(hr)) {
    DVLOG(1) << "ITfRange::CompareEnd failed in "
             << __SHORT_FUNCTION__;
    return false;
  }
  if (comp_end < 0)
    return false;

  return true;
}

void TextRange::Reconvert() {
  if (write_cookie() == TF_INVALID_COOKIE ||
      context_event_sink_->composition_event_sink_ == NULL) {
    DCHECK(false);
    return;
  }
  context_event_sink_->composition_event_sink_->Reconvert(
      write_cookie(), range_);
}

TextRangeInterface *TextRange::Clone() {
  SmartComPtr<ITfRange> cloned_range;
  HRESULT hr = range_->Clone(cloned_range.GetAddress());
  if (FAILED(hr) || !cloned_range) {
    DVLOG(1) << "ITfRange::Clone failed in " << __SHORT_FUNCTION__;
    return NULL;
  }
  return new TextRange(context_event_sink_, cloned_range);
}

TfEditCookie TextRange::write_cookie() {
  if (!context_event_sink_) {
    DCHECK(false);
    return TF_INVALID_COOKIE;
  }
  return context_event_sink_->write_cookie();
}
}  // namespace tsf
}  // namespace ime_goopy
