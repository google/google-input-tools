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
#ifndef GOOPY_COMMON_LANGUAGE_UTILS_H__
#define GOOPY_COMMON_LANGUAGE_UTILS_H__

#include <string>

namespace ime_goopy {
typedef struct {
  std::string language;
  std::string locale;
  int id;
  const wchar_t* guid;
} LanguageInfo;

static const LanguageInfo kLanguageInfo[] = {
  { "hindi", "hi", 0x0439, L"{A2B2E551-9FB0-4095-A4D9-BD8AE7297296}" },
  { "arabic", "ar", 0x0401, L"{C58258B8-BA98-4367-B14C-8417B32E9032}" },
  { "bengali", "bn", 0x0445, L"{3F7884E0-1BE0-48C3-8F70-52D94498FBDD}" },
  { "farsi", "fa", 0x0429, L"{CFDB00EA-4CAC-4E2C-A66A-0D7BF856410B}" },
  { "greek", "el", 0x0408, L"{84BFD315-595E-4560-BA89-8C86A1E20622}" },
  { "gujarati", "gu", 0x0447, L"{5AB5ACE1-3592-4831-B54B-AD876B9C16E4}" },
  { "kannada", "kn", 0x044b, L"{97DC6F28-523D-4F2F-9F60-AEEA2F3E944E}" },
  { "malayalam", "ml", 0x044c, L"{40365F79-CD47-4A90-A098-E73EB6A88163}" },
  { "marathi", "mr", 0x044e, L"{C7C7A202-7042-4FC0-863A-3259598FB8EA}" },
  { "nepali", "ne", 0x0461, L"{851ED2ED-6F1D-4307-BD23-A77F9034703A}" },
  { "punjabi", "pa", 0x0446, L"{10D0324F-5151-4530-B205-226B03B31A66}" },
  { "tamil", "ta", 0x0449, L"{E968E5F8-5311-43DD-8346-0F98CC4D6CE9}" },
  { "telugu", "te", 0x044a, L"{BEA60BED-18F2-4508-9F09-FB8B1BDE647E}" },
  { "urdu", "ur", 0x0420, L"{4F3AC476-94BC-479A-B69B-C0B3F7AB2BE4}" },
  { "russian", "ru", 0x0419, L"{430E91F0-85E3-449E-8C01-E9681A3A68B0}" },
  { "serbian", "sr", 0x0c1a, L"{A790DE02-8A9F-4DF3-9FB0-9D1079716A5F}" },
  { "sanskrit", "sa", 0x044f, L"{C0937767-2BFB-44F1-9BE1-E94A3E8E5945}" },
  { "amharic", "am", 0x045e, L"{A7D1CE4B-B134-4E87-92BD-E97E966371D4}" },
  { "tigrinya", "ti", 0x0473, L"{C15DBD17-69C3-4AAE-99BD-873E20E08453}" },
  { "hebrew", "he", 0x040d, L"{D242F102-5938-4611-AE26-F9B8D4BBD634}" },
  { "sinhalese", "si", 0x045b, L"{1DB10FC8-A15F-46DF-B72E-81C51BE12FA2}" },
  { "oriya", "or", 0x0448, L"{413F9661-7E16-4BB8-A1C6-4516D80AF04A}" },
};

static const LanguageInfo kEnglishInfo =
    { "english", "en", 0x0409, L"{8FC8BDCA-0805-48A3-B051-F65EF41E12C0}" };

// Find the LanguageInfo object whose language equals |language|, and fill it
// in |language_info|. return false if the language is not supported.

bool GetLanguageInfoByLanguage(const std::string& language,
                               LanguageInfo* language_info);

// Find the LanguageInfo object whose locale equals |locale|, and fill it
// in |language_info|. return false if the language is not supported.
bool GetlanguageInfoByLocale(const std::string& locale,
                             LanguageInfo* language_info);
}  // namespace ime_goopy

#endif  // GOOPY_COMMON_LANGUAGE_UTILS_H__
