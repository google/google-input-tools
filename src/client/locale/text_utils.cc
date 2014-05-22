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

#include "locale/text_utils.h"

#include <locale>
#include "base/logging.h"

namespace ime_goopy {

static const wchar_t kEnglishLimit = 0x7F;
static const wchar_t kUnicodeApostrophe = 0x2019;
static const wchar_t kUnicodeChMin = 0x4E00;
static const wchar_t kUnicodeChMax = 0X9FA5;

int TextManipulatorZhCn::ExpandToWordBegin(const wstring& text, int pos) {
  DCHECK_LE(pos, text.size());
  int index = 0;
  for (index = pos - 1; index >= 0; --index) {
    if (!IsValidCharInWord(text[index]))
      break;
  }
  return pos - 1 - index;
}

int TextManipulatorZhCn::ExpandToWordEnd(const wstring& text, int pos) {
  int length = text.size();
  DCHECK_LE(pos, length);
  int index = 0;
  for (index = pos; index < length; ++index) {
    if (!IsValidCharInWord(text[index]))
      break;
  }
  return index - pos;
}

int TextManipulatorZhCn::ExpandToSentenceBegin(const wstring& text, int pos) {
  int length = text.size();
  DCHECK_LE(pos, length);
  int index = 0;
  for (index = pos - 1; index >= 0; --index)
    if (!IsValidCharInSentence(text[index]))
      break;
  for (++index; index < length; ++index)
    if (IsValidCharInWord(text[index]))
      break;
  --index;
  return pos - 1 - index;
}

int TextManipulatorZhCn::ExpandToSentenceEnd(const wstring& text, int pos) {
  int length = text.size();
  DCHECK_LE(pos, length);
  int index = 0;
  for (index = pos; index < length; ++index)
    if (!IsValidCharInSentence(text[index]))
      break;
  for (--index; index >= 0; --index)
    if (IsValidCharInWord(text[index]))
      break;
  ++index;
  return index - pos;
}

bool TextManipulatorZhCn::IsSentenceBegin(const wstring& text) {
  DCHECK(!text.empty());
  return isupper(text[0], locale_);
}

void TextManipulatorZhCn::SetSentenceBegin(wstring* text) {
  DCHECK(!text->empty());
  (*text)[0] = toupper((*text)[0], locale_);
}

bool TextManipulatorZhCn::IsValidCharInWord(wchar_t ch) {
  return ch >= kUnicodeChMin && ch <= kUnicodeChMax;
}

bool TextManipulatorZhCn::IsValidCharInSentence(wchar_t ch) {
  return IsValidCharInWord(ch) || ch == L'-';
}

int TextManipulatorEn::ExpandToWordBegin(const wstring& text, int pos) {
  DCHECK_LE(pos, text.size());
  int index = 0;
  for (index = pos - 1; index >= 0; --index) {
    if (!IsValidCharInWord(text[index]))
      break;
  }
  return pos - 1 - index;
}

int TextManipulatorEn::ExpandToWordEnd(const wstring& text, int pos) {
  int length = text.size();
  DCHECK_LE(pos, length);
  int index = 0;
  for (index = pos; index < length; ++index) {
    if (!IsValidCharInWord(text[index]))
      break;
  }
  return index - pos;
}

int TextManipulatorEn::ExpandToSentenceBegin(const wstring& text, int pos) {
  int length = text.size();
  DCHECK_LE(pos, length);
  int index = 0;
  for (index = pos - 1; index >= 0; --index)
    if (!IsValidCharInSentence(text[index]))
      break;
  for (++index; index < length; ++index)
    if (IsValidCharInWord(text[index]))
      break;
  --index;
  return pos - 1 - index;
}

int TextManipulatorEn::ExpandToSentenceEnd(const wstring& text, int pos) {
  int length = text.size();
  DCHECK_LE(pos, length);
  int index = 0;
  for (index = pos; index < length; ++index)
    if (!IsValidCharInSentence(text[index]))
      break;
  for (--index; index >= 0; --index)
    if (IsValidCharInWord(text[index]))
      break;
  ++index;
  return index - pos;
}

bool TextManipulatorEn::IsSentenceBegin(const wstring& text) {
  DCHECK(!text.empty());
  return text[0] <= kEnglishLimit && isupper(text[0]);
}

void TextManipulatorEn::SetSentenceBegin(wstring* text) {
  DCHECK(!text->empty());
  if ((*text)[0] <= kEnglishLimit)
    (*text)[0] = toupper((*text)[0]);
}

bool TextManipulatorEn::IsValidCharInWord(wchar_t ch) {
  if (ch == kUnicodeApostrophe)
    return true;
  if (ch > kEnglishLimit)
    return false;
  return isalnum(ch) || ch == L'\'';
}

bool TextManipulatorEn::IsValidCharInSentence(wchar_t ch) {
  if (ch > kEnglishLimit && ch != kUnicodeApostrophe)
    return false;
  return IsValidCharInWord(ch) || ch == L'-' ||
         (!ispunct(ch) && !iscntrl(ch));
}

}  // namespace ime_goopy
