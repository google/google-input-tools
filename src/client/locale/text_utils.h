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
//
// This class defines text manipulators and other text utilities which behalf
// differently according to the locale.

#ifndef GOOPY_LOCALE_TEXT_UTILS_H__
#define GOOPY_LOCALE_TEXT_UTILS_H__

#include <locale>
#include <string>
#include "base/basictypes.h"

namespace ime_goopy {

// Interface for manipulating plain text. The operations within this class may
// be varied for different locale. To add a new type of text manipulator,
// please also update the creation of manipulators in LocaleUtil class.
class TextManipulator {
 public:
  virtual ~TextManipulator() {}

  // Expands the selection to the beginning or end of the given word/sentence
  // boundary. The starting pos is given as the entry point. The return value
  // indicates the number of wchar_t to shift to reach the boundary.
  virtual int ExpandToWordBegin(const wstring& text, int pos) = 0;
  virtual int ExpandToWordEnd(const wstring& text, int pos) = 0;
  virtual int ExpandToSentenceBegin(const wstring& text, int pos) = 0;
  virtual int ExpandToSentenceEnd(const wstring& text, int pos) = 0;

  // Returns true if the text is starting a new sentence.
  virtual bool IsSentenceBegin(const wstring& text) = 0;
  // Converts the text into a proper sentence beginning style.
  // In English, it is to capitalize the first character of the given string.
  virtual void SetSentenceBegin(wstring* text) = 0;
  // Check ch if it is valid character based on current language in a word.
  virtual bool IsValidCharInWord(wchar_t ch) = 0;
  // Check ch if it is valid character based on current language in a sentence.
  virtual bool IsValidCharInSentence(wchar_t ch) = 0;
};

// An implementation for Simplified Chinese.
class TextManipulatorZhCn : public TextManipulator {
 public:
  TextManipulatorZhCn() {}
  virtual ~TextManipulatorZhCn() {}
  virtual int ExpandToWordBegin(const wstring& text, int pos);
  virtual int ExpandToWordEnd(const wstring& text, int pos);
  virtual int ExpandToSentenceBegin(const wstring& text, int pos);
  virtual int ExpandToSentenceEnd(const wstring& text, int pos);
  virtual bool IsSentenceBegin(const wstring& text);
  virtual void SetSentenceBegin(wstring* text);
  virtual bool IsValidCharInWord(wchar_t ch);
  virtual bool IsValidCharInSentence(wchar_t ch);
 private:
  std::locale locale_;
  DISALLOW_COPY_AND_ASSIGN(TextManipulatorZhCn);
};

// An implementation for English.
class TextManipulatorEn : public TextManipulator {
 public:
  TextManipulatorEn() {}
  virtual ~TextManipulatorEn() {}
  virtual int ExpandToWordBegin(const wstring& text, int pos);
  virtual int ExpandToWordEnd(const wstring& text, int pos);
  virtual int ExpandToSentenceBegin(const wstring& text, int pos);
  virtual int ExpandToSentenceEnd(const wstring& text, int pos);
  virtual bool IsSentenceBegin(const wstring& text);
  virtual void SetSentenceBegin(wstring* text);
  virtual bool IsValidCharInWord(wchar_t ch);
  virtual bool IsValidCharInSentence(wchar_t ch);
 private:
  DISALLOW_COPY_AND_ASSIGN(TextManipulatorEn);
};
}  // namespace ime_goopy

#endif  // GOOPY_LOCALE_TEXT_UTILS_H__
