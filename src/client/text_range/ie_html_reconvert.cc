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

#include "text_range/ie_html_reconvert.h"

#include "base/logging.h"
#include "common/debug.h"
#include "common/string_utils.h"
#include "locale/locale_utils.h"
#include "locale/text_utils.h"

static const wchar_t kInputHtmlTag[] = L"input";
static const wchar_t kInputHtmlElementValueAttr[] = L"value";
static const wchar_t kTagPrefix = L'<';
static const wchar_t kEmptyChar = ' ';

namespace ime_goopy {
IEHtmlReconvert::IEHtmlReconvert(
    Callback1<HtmlTextRange*> *on_reconvert,
    EngineReconvertCallback* engine_reconvert_callback,
    ShouldReconvertCallback* should_reconvert_callback,
    HtmlTextRange* html_text_range)
    : on_reconvert_(on_reconvert),
      engine_reconvert_callback_(engine_reconvert_callback),
      should_reconvert_callback_(should_reconvert_callback),
      html_text_range_(html_text_range) {
}

bool IEHtmlReconvert::Reconvert(ReconvertAlignType align_type) {
  if (!html_text_range_)
    return false;
  // Get IHTMLTxtRange parent elment to calculate txtrange element
  // move position.
  SmartComPtr<IHTMLElement> parent_html_element;
  if (FAILED(html_text_range_->txt_range()->parentElement(
          parent_html_element.GetAddress()))) {
    return false;
  }

#ifdef DEBUG
  CComBSTR html_text_bstr;
  parent_html_element->get_outerHTML(&html_text_bstr);
  DVLOG(3) << __SHORT_FUNCTION__ << L":ParentHTML:" << html_text_bstr;
  html_text_range_->txt_range()->get_text(&html_text_bstr);
  DVLOG(3) << __SHORT_FUNCTION__ << L":TEXT:" << html_text_bstr;
  html_text_range_->txt_range()->get_htmlText(&html_text_bstr);
  DVLOG(3) << __SHORT_FUNCTION__ << L":HTMLTEXT:" << html_text_bstr;
  wstring str = html_text_range_->GetText();
  DVLOG(3) << __SHORT_FUNCTION__ << L":WTEXT:" << str
           << L":Len:" << str.size();
#endif

  wstring element_text = L"";
  // Get html element tag first, then decide how to get the real text.
  CComBSTR element_tag_bstr;
  if (FAILED(parent_html_element->get_tagName(&element_tag_bstr)))
    return false;
  wstring element_tag(element_tag_bstr);
  LowerString(&element_tag);
  if (element_tag.empty())
    return false;
  DVLOG(3) << __SHORT_FUNCTION__ << L":HTML Tag:" << element_tag;

  // If it is a "input" html tag, get its text from 'value' attribute.
  if (element_tag == kInputHtmlTag) {
    CComBSTR text_value(kInputHtmlElementValueAttr);
    CComVariant attribute;
    if (FAILED(parent_html_element->getAttribute(text_value,
                                                 0,
                                                 &attribute))) {
      return false;
    }
    if (attribute.bstrVal == NULL)
      return false;
    element_text = attribute.bstrVal;
    if (element_text.empty())
      return false;
  } else {
    // Else, get the text of parent html element.
    CComBSTR html_text_bstr;
    if (FAILED(parent_html_element->get_outerText(&html_text_bstr)))
      return false;
    if (html_text_bstr.Length() == 0)
      return false;
    wstring html_text(html_text_bstr);
    RemoveWhiteSpaces(&html_text);
    if (html_text.empty())
      return false;
    element_text = static_cast<wstring>(html_text_bstr);
  }
  DVLOG(3) << __SHORT_FUNCTION__ << L"HTMLText:" << element_text;

  int left_move = 0;
  int right_move = 0;
  DVLOG(3) << __SHORT_FUNCTION__ << L"after HTMLText";
  if (!CalculateMoveDistance(element_text,
                             align_type,
                             &left_move,
                             &right_move)) {
    DVLOG(3) << __SHORT_FUNCTION__ << L"CalculateMoveDistance Failed";
    return false;
  }
  DVLOG(3) << __SHORT_FUNCTION__ << L"Left Move: " << left_move
                                 << L"Right Move: " << right_move;
  wstring text = html_text_range_->GetText();
  if (should_reconvert_callback_ &&
      !should_reconvert_callback_->Run(text)) {
    if (on_reconvert_ && !on_reconvert_->IsRepeatable()) {
      delete on_reconvert_;
    }
    if (engine_reconvert_callback_ &&
        !engine_reconvert_callback_->IsRepeatable()) {
      delete engine_reconvert_callback_;
    }
    return false;
  }
  // Html reconvert action just highlight the text.
  DVLOG(3) << __SHORT_FUNCTION__ << L"Final Text: "
                                 << html_text_range_->GetText();
  html_text_range_->Select();
  if (on_reconvert_)
    on_reconvert_->Run(html_text_range_);
  if (engine_reconvert_callback_)
    engine_reconvert_callback_->Run(text, left_move);
  return true;
}

bool IEHtmlReconvert::CalculateMoveDistance(const wstring& html_text,
                                            ReconvertAlignType align_type,
                                            int* left_move,
                                            int* right_move) {
  DVLOG(3) << __SHORT_FUNCTION__;
  int parent_text_len = html_text.length();
  int txt_range_text_len = html_text_range_->GetText().length();
  if (txt_range_text_len >= parent_text_len) {
    // If the text user selected longer than parent html element text
    // do nothing cause the selection made by user dose make sense.
    DVLOG(3) << __SHORT_FUNCTION__ << ":TRUE";
    return true;
  } else {
    // First, test the validation of the user selection.
    // assume parent html element is <*>test text</*>, and html_text would
    // be "test text". No matter what range text the user selected(usually,
    // user would not select any text), we move txt range for both
    // direction(left and right) and the offset would be html_text length.
    // After the movement, txt range text should contain html_text,
    // if not, something is wrong.
    // e.g:
    //    If user select "st", then txt range move 9 character for left
    // and right. The txt range text would be *****test text**.
    // Notice:
    //    Ususally, the txt range text movement does not mean txt range text
    // would get longer as much as the move offset, i.e move right 6 character,
    // but txt range text will not grow 6 character cause 6 characater
    // movement including HTML tag character and the pure text may not equal
    // 6 character.
    DVLOG(3) << __SHORT_FUNCTION__ << L"::CloneRange BeforeMove:"
                                   << html_text_range_->GetText();
    // Make left and right html range.
    scoped_ptr<HtmlTextRange> left_html_range(
        html_text_range_->CloneHtmlTextRange());
    left_html_range->CollapseToStart();
    left_html_range->ShiftStart(-parent_text_len, NULL);
    DVLOG(3) << __SHORT_FUNCTION__ << L"::MoveLeft:"
                                   << left_html_range->GetText()
                                   << L"::MovePos:"
                                   << parent_text_len;

    scoped_ptr<HtmlTextRange> right_html_range(
        html_text_range_->CloneHtmlTextRange());
    right_html_range->CollapseToEnd();
    right_html_range->ShiftEnd(parent_text_len, NULL);
    DVLOG(3) << __SHORT_FUNCTION__ << L"::MoveLeft:"
                                   << right_html_range->GetText()
                                   << L"::MovePos:"
                                   << parent_text_len;
    // Verify the validity of left and right text range.
    wstring left_text = left_html_range->GetText();
    wstring right_text = right_html_range->GetText();
    wstring selection_text = html_text_range_->GetText();
    wstring whole_text = left_text + selection_text + right_text;
    if (whole_text.find(html_text) == wstring::npos) {
      return false;
    }
    // Adjust left text and righ text.
    AdjustLeftText(html_text, &left_text);
    AdjustRightText(html_text, &right_text);

    // Calculate move offset.
    // Try expanding to an English word.
    {
      TextManipulatorEn en_manipulator_;
      // If the first character of selection_text is english character or
      // selection_text is empty, expanding to left direction.
      if (selection_text.empty() ||
          en_manipulator_.IsValidCharInWord(selection_text[0])) {
        *left_move = en_manipulator_.ExpandToWordBegin(
            left_text, left_text.length());
        if (*left_move > 0) {
          int actual_move = 0;
          html_text_range_->ShiftStart(-*left_move, &actual_move);
          if (actual_move != -*left_move)
            return false;
        }
      }

      if (selection_text.empty() ||
          en_manipulator_.IsValidCharInWord(
              selection_text[selection_text.length() - 1])) {
        *right_move = en_manipulator_.ExpandToWordEnd(right_text, 0);
        if (*right_move > 0) {
          int actual_move = 0;
          html_text_range_->ShiftEnd(*right_move, &actual_move);
          if (actual_move != *right_move)
            return false;
        }
      }
    }

    // If empty, try expanding to a Unicode word.
    if (html_text_range_->IsEmpty()) {
      DCHECK_EQ(0, *left_move);
      DCHECK_EQ(0, *right_move);
      TextManipulatorZhCn zh_manipulator_;
      *left_move = zh_manipulator_.ExpandToWordBegin(
          left_text, left_text.length());
      *right_move = zh_manipulator_.ExpandToWordEnd(right_text, 0);
      if (*left_move > 0) {
        int actual_move = 0;
        html_text_range_->ShiftStart(-*left_move, &actual_move);
        if (actual_move != -*left_move)
          return false;
      }
      if (*right_move > 0) {
        int actual_move = 0;
        html_text_range_->ShiftEnd(*right_move, &actual_move);
        if (actual_move != *right_move)
          return false;
      }
    }
  }
  return true;
}

void IEHtmlReconvert::AdjustLeftText(const wstring& html_text,
                                     wstring* left_text) {
  DCHECK(left_text);
  if (left_text->empty())
    return;

  // Adjust left.
  // For instance, html_text = "abaacde", and left_text is "ccab",
  // we need to cut left_text to "ab".
  int html_text_pos = html_text.length() - 1;
  int html_text_start_pos = html_text_pos;
  wchar_t left_text_last_char = (*left_text)[left_text->length() - 1];

  while (true) {
    // First, find first identical character pos.
    html_text_start_pos = html_text.rfind(left_text_last_char, html_text_pos);
    if (html_text_start_pos == wstring::npos) {
      *left_text = L"";
      return;
    }
    html_text_pos = html_text_start_pos;
    // Then, keep moving.
    int left_text_pos = left_text->length() -1;
    while (html_text[html_text_pos] == (*left_text)[left_text_pos]) {
      html_text_pos--;
      left_text_pos--;
      if (html_text_pos < 0 || left_text_pos < 0)
        break;
    }

    if (html_text_pos >= 0 && left_text_pos >= 0) {
      // Can not find sub string, change html_text start find pos, and go on.
      continue;
    }
    if (left_text_pos < 0) {
      // Left_text is considered as a sub string of html_text, no need to
      // adjust it.
      return;
    }
    if (html_text_pos < 0) {
      // Left_text has extra character which html_text doesn't contain, so cut
      // it down.
      *left_text = left_text->substr(
          left_text->length() - html_text_start_pos - 1);
      return;
    }
  }
}

void IEHtmlReconvert::AdjustRightText(const wstring& html_text,
                                      wstring* right_text) {
  DCHECK(right_text);
  if (right_text->empty())
    return;

  int html_text_pos = 0;
  int html_text_start_pos = 0;
  wchar_t right_text_first_char = (*right_text)[0];
  int html_text_size = html_text.length();
  int right_text_size = right_text->length();

  while (true) {
    html_text_start_pos = html_text.find(right_text_first_char, html_text_pos);
    if (html_text_start_pos == wstring::npos) {
      *right_text = L"";
      return;
    }

    int right_text_pos = 0;
    html_text_pos = html_text_start_pos;
    while (html_text[html_text_pos] == (*right_text)[right_text_pos]) {
      html_text_pos++;
      right_text_pos++;
      if (html_text_pos >= html_text_size ||
          right_text_pos >= right_text_size) {
        break;
      }
    }

    if (html_text_pos < html_text_size && right_text_pos < right_text_size) {
      continue;
    }
    if (right_text_pos == right_text_size) {
      return;
    }
    if (html_text_pos == html_text_size) {
      *right_text = right_text->substr(
          0, html_text_size - html_text_start_pos);
      return;
    }
  }
}

}  // namespace ime_goopy
