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

#include "text_range/html_text_range.h"
#include <atlbase.h>
#include <atlapp.h>
#include <atlwin.h>
#include <atlctrls.h>
#include "base/logging.h"
#include "base/scoped_ptr.h"
#include "text_range/window_utils.h"

namespace ime_goopy {

HRESULT DocumentFromIEWindow(HWND window, IHTMLDocument2 **doc) {
  if (!doc) {
    NOTREACHED();
    return E_INVALIDARG;
  }
  HMODULE lib_handle = ::LoadLibrary(L"oleacc.dll");
  if (!lib_handle)
    return E_FAIL;
  UINT wm_html_getobject = RegisterWindowMessageW(L"WM_HTML_GETOBJECT");
  LRESULT result;
  if (SendMessageTimeout(window, wm_html_getobject, 0L, 0L, SMTO_ABORTIFHUNG,
                         1000, reinterpret_cast<DWORD_PTR *>(&result)) == 0) {
    return E_FAIL;
  }
  LPFNOBJECTFROMLRESULT func = reinterpret_cast<LPFNOBJECTFROMLRESULT>(
      GetProcAddress(lib_handle, "ObjectFromLresult"));
  if (!func) return E_FAIL;
  SmartComPtr<IHTMLDocument> doc1;
  if (FAILED((*func)(result, IID_IHTMLDocument2, 0,
                     reinterpret_cast<void **>(doc1.GetAddress())))) {
    return E_FAIL;
  }
  SmartComPtr<IHTMLDocument2> doc2(doc1);
  if (!doc2)
    return E_FAIL;
  *doc = doc2.Detach();
  return S_OK;
}

HtmlTextRange *HtmlTextRange::CreateFromSelection(
    Callback1<HtmlTextRange*> *on_reconvert, HWND handle) {
  if (!IsWindow(handle)) {
    DCHECK(false);
    return NULL;
  }
  if (!IsBrowserControl(handle))
    return NULL;
  SmartComPtr<IHTMLDocument2> doc;
  if (FAILED(DocumentFromIEWindow(handle, doc.GetAddress())))
    return NULL;
  SmartComPtr<IHTMLSelectionObject> selection;
  if (FAILED(doc->get_selection(selection.GetAddress())))
    return NULL;
  SmartComPtr<IDispatch> range_dispatch;
  if (FAILED(selection->createRange(range_dispatch.GetAddress())))
    return NULL;
  SmartComPtr<IHTMLTxtRange> range(range_dispatch);
  if (!range)
    return NULL;
  return new HtmlTextRange(on_reconvert, handle, range);
}

HtmlTextRange::HtmlTextRange(
    Callback1<HtmlTextRange*> *on_reconvert, HWND handle, IHTMLTxtRange *range)
    : on_reconvert_(on_reconvert), handle_(handle), range_(range) {
}

wstring HtmlTextRange::GetText() {
  CComBSTR str;
  if (SUCCEEDED(range_->get_text(&str)) && str)
    return wstring(str);
  else
    return wstring();
}

void HtmlTextRange::ShiftStart(int offset, int *actual_offset) {
  LONG long_offset = offset, long_actual_offset = 0;
  range_->moveStart(CComBSTR("character"), long_offset, &long_actual_offset);
  if (actual_offset)
    *actual_offset = static_cast<int>(long_actual_offset);
}

void HtmlTextRange::ShiftEnd(int offset, int *actual_offset) {
  LONG long_offset = offset, long_actual_offset = 0;
  range_->moveEnd(CComBSTR("character"), long_offset, &long_actual_offset);
  if (actual_offset)
    *actual_offset = static_cast<int>(long_actual_offset);
}

void HtmlTextRange::CollapseToStart() {
  range_->collapse(VARIANT_TRUE);
}

void HtmlTextRange::CollapseToEnd() {
  range_->collapse(VARIANT_FALSE);
}

bool HtmlTextRange::IsEmpty() {
  return GetText().empty();
}

bool HtmlTextRange::IsContaining(TextRangeInterface *inner_range) {
  if (!inner_range) {
    DCHECK(false);
    return false;
  }
  HtmlTextRange *casted_inner_range =
      down_cast<HtmlTextRange*>(inner_range);
  if (!casted_inner_range) {
    DCHECK(false);
    return false;
  }
  VARIANT_BOOL in_range = VARIANT_FALSE;
  if (FAILED(range_->inRange(casted_inner_range->range_, &in_range)))
    return false;
  return in_range == VARIANT_TRUE;
}

void HtmlTextRange::Reconvert() {
  Select();
  if (on_reconvert_)
    on_reconvert_->Run(this);
}

TextRangeInterface *HtmlTextRange::Clone() {
  SmartComPtr<IHTMLTxtRange> cloned_range;
  if (FAILED(range_->duplicate(cloned_range.GetAddress())))
    return NULL;
  return new HtmlTextRange(on_reconvert_, handle_, cloned_range);
}

void HtmlTextRange::SetText(const wchar_t *text) {
  range_->put_text(CComBSTR(text));
}

void HtmlTextRange::Select() {
  range_->select();
}

POINT HtmlTextRange::GetCompositionPos() {
  POINT result = {0, 0};
  SmartComPtr<IHTMLTextRangeMetrics> metrics(range_);
  if (!metrics)
    return result;
  LONG left = 0, top = 0;
  if (FAILED(metrics->get_boundingLeft(&left)) ||
      FAILED(metrics->get_boundingTop(&top))) {
    return result;
  }
  result.x = left;
  result.y = top;
  ClientToScreen(handle_, &result);
  return result;
}
}  // namespace ime_goopy
