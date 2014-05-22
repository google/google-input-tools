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

#ifndef GOOPY_TSF_TEXT_RANGE_H_
#define GOOPY_TSF_TEXT_RANGE_H_

#include <atlbase.h>
#include <atlcom.h>
#include <ctffunc.h>
#include <msctf.h>
#include <string>

#include "common/framework_interface.h"
#include "common/smart_com_ptr.h"

using std::wstring;

namespace ime_goopy {
namespace tsf {
class ContextEventSink;
class CompositionEventSink;

class TextRange : public TextRangeInterface {
 public:
  TextRange(ContextEventSink *context_event_sink, ITfRange *range);
  virtual wstring GetText();
  virtual void ShiftStart(int offset, int *actual_offset);
  virtual void ShiftEnd(int offset, int *actual_offset);
  virtual void CollapseToStart();
  virtual void CollapseToEnd();
  virtual bool IsEmpty();
  virtual bool IsContaining(TextRangeInterface *inner_range);
  virtual void Reconvert();
  virtual TextRangeInterface *Clone();
 private:
  class CompositionSink;
  TfEditCookie write_cookie();
  SmartComPtr<ITfRange> range_;
  ContextEventSink *context_event_sink_;

  DISALLOW_COPY_AND_ASSIGN(TextRange);
};
}  // namespace tsf
}  // namespace ime_goopy
#endif  // GOOPY_TSF_TEXT_RANGE_H_
