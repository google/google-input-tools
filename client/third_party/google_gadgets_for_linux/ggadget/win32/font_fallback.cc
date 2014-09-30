/*
  Copyright 2012 Google Inc.

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

#include "font_fallback.h"

#include <mlang.h>
#include <usp10.h>
#include <algorithm>
#include <hash_map>
#include <vector>

#include "ggadget/scoped_ptr.h"

namespace ggadget {
namespace win32 {
namespace {
const wchar_t kFontLinkKey[] =
    L"Software\\Microsoft\\Windows NT\\CurrentVersion\\FontLink\\SystemLink";

bool CurrentFontContainsCharacter(HDC hdc, wchar_t character) {
  scoped_array<char> buffer(new char[::GetFontUnicodeRanges(hdc, 0)]);
  GLYPHSET* glyphset = reinterpret_cast<GLYPHSET*>(buffer.get());
  ::GetFontUnicodeRanges(hdc, glyphset);
  for (unsigned i = 0; i < glyphset->cRanges; ++i) {
    if (glyphset->ranges[i].wcLow <= character &&
        glyphset->ranges[i].wcLow + glyphset->ranges[i].cGlyphs > character) {
      return true;
    }
  }
  return false;
}

IMLangFontLink2* GetFontLinkInterface() {
  static IMultiLanguage* multi_language = NULL;
  if (!multi_language) {
    if (::CoCreateInstance(CLSID_CMultiLanguage, 0, CLSCTX_ALL,
                           IID_IMultiLanguage,
                           reinterpret_cast<void**>(&multi_language)) != S_OK)
      return 0;
  }

  static IMLangFontLink2* font_link = NULL;
  if (!font_link) {
    if (multi_language->QueryInterface(&font_link) != S_OK)
      return 0;
  }

  return font_link;
}

int CALLBACK MetaFileEnumProc(
    HDC hdc,
    HANDLETABLE* table,
    CONST ENHMETARECORD* record,
    int table_entries,
    LPARAM log_font) {
  // Enumerate the MetaFile operations and find the last CreateFontIndirect call
  // to determined what font uniscribe used in the metafile.
  if (record->iType == EMR_EXTCREATEFONTINDIRECTW) {
    const EMREXTCREATEFONTINDIRECTW* create_font_record =
        reinterpret_cast<const EMREXTCREATEFONTINDIRECTW*>(record);
    *reinterpret_cast<LOGFONT*>(log_font) = create_font_record->elfw.elfLogFont;
  }
  return true;
}

LOGFONT CreateMLangFont(IMLangFontLink2* font_link,
                        HDC hdc,
                        DWORD code_pages,
                        wchar_t character) {
  HFONT mlang_font = NULL;
  LOGFONT lf = {0};
  if (SUCCEEDED(font_link->MapFont(hdc, code_pages, character, &mlang_font)) &&
      mlang_font) {
    ::GetObject(mlang_font, sizeof(LOGFONT), &lf);
    font_link->ReleaseFont(mlang_font);
  }
  return lf;
}

bool FontContainsCharacter(const LOGFONT& logfont,
                           HDC dc,
                           wchar_t character) {
  HFONT font = ::CreateFontIndirect(&logfont);
  bool contains = font;
  if (contains) {
    HGDIOBJ old_font = ::SelectObject(dc, font);
    contains = CurrentFontContainsCharacter(dc, character);
    ::SelectObject(dc, old_font);
  }
  ::DeleteObject(font);
  return contains;
}

const std::vector<std::wstring>* GetLinkedFonts(const std::wstring& family) {
  typedef stdext::hash_map<std::wstring, std::vector<std::wstring> >
          FontLinkMap;
  static FontLinkMap system_link_map;
  FontLinkMap::iterator it = system_link_map.find(family);
  if (it != system_link_map.end()) {
    return &it->second;
  }
  HKEY key = NULL;
  if (FAILED(::RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                            kFontLinkKey,
                            0,
                            KEY_READ,
                            &key))) {
    return NULL;
  }
  DWORD size = 0;
  RegQueryValueEx(key, family.c_str(), 0, NULL, NULL, &size);
  scoped_array<BYTE> buffer(new BYTE[size]);
  if (SUCCEEDED(RegQueryValueEx(key,
                                family.c_str(),
                                0,
                                NULL,
                                buffer.get(),
                                &size))) {
    // The value in the registry is a MULTI_SZ like:
    // <filename>,<font family>\0
    // for example:
    // MSMINCHO.TTC,MS Mincho\0
    // MINGLIU.TTC,MingLiU\0
    // SIMSUN.TTC,SimSun\0
    // And we need to find all the font families.
    std::vector<std::wstring>& linked_fonts = system_link_map[family];
    unsigned i = 0;
    unsigned length = size / sizeof(wchar_t);
    wchar_t* string_value = reinterpret_cast<wchar_t*>(buffer.get());
    while (i < length) {
      while (i < length && string_value[i] != ',')
        i++;
      i++;
      unsigned j = i;
      while (j < length && string_value[j])
        j++;
      linked_fonts.push_back(std::wstring(string_value + i, j - i));
      i = j + 1;
    }
    return &linked_fonts;
  }
  return NULL;
}


}  // namespace

bool FontFallback::ShouldFallback(HDC dc, wchar_t character) {
  if (IMLangFontLink2* font_link = GetFontLinkInterface()) {
    // Use MLang font linking to determined if we can display the text in given
    // font.
    DWORD font_code_pages = 0;
    DWORD char_code_pages = 0;
    font_link->GetCharCodePages(character, &char_code_pages);
    font_link->GetFontCodePages(
        dc,
        reinterpret_cast<HFONT>(::GetCurrentObject(dc, OBJ_FONT)),
        &font_code_pages);
    if (char_code_pages & font_code_pages)
      return false;
    if (char_code_pages != 0 || font_code_pages != 0)
      return true;
  }
  return !CurrentFontContainsCharacter(dc, character);
}

// Fallback font is determined by following steps:
// 1. Use MLang to map current font.
// 2. Try drawing with the mapped font and find which font uniscribe used.
// 3. if the font in 2 does not contain the character, try its linked fonts.
LOGFONT FontFallback::GetFallbackFont(HDC dc, wchar_t character) {
  LOGFONT fallback_font = {0};
  ::GetObject(::GetCurrentObject(dc, OBJ_FONT),
              sizeof(fallback_font),
              &fallback_font);
  if (IMLangFontLink2* font_link = GetFontLinkInterface()) {
    DWORD char_code_pages = 0;
    font_link->GetCharCodePages(character, &char_code_pages);
    fallback_font = CreateMLangFont(font_link, dc, char_code_pages,
                                    character);
  }
  // To find out what font Uniscribe would use, we make it draw into a
  // metafile and intercept calls to CreateFontIndirect().
  HDC meta_file_dc = ::CreateEnhMetaFile(dc, NULL, NULL, NULL);
  ::SelectObject(meta_file_dc, ::GetCurrentObject(dc, OBJ_FONT));

  bool success = false;
  SCRIPT_STRING_ANALYSIS ssa;

  if (SUCCEEDED(::ScriptStringAnalyse(
      meta_file_dc, &character, 1, 0, -1,
      SSA_METAFILE | SSA_FALLBACK | SSA_GLYPHS | SSA_LINK,
      0, NULL, NULL, NULL, NULL, NULL, &ssa))) {
    success = SUCCEEDED(::ScriptStringOut(ssa, 0, 0, 0, NULL, 0, 0, FALSE));
    ::ScriptStringFree(&ssa);
  }
  HENHMETAFILE meta_file = ::CloseEnhMetaFile(meta_file_dc);
  if (success)
    ::EnumEnhMetaFile(0, meta_file, MetaFileEnumProc, &fallback_font, NULL);
  ::DeleteEnhMetaFile(meta_file);
  if (FontContainsCharacter(fallback_font, dc, character))
    return fallback_font;
  const std::vector<std::wstring>* linked_fonts = GetLinkedFonts(
      fallback_font.lfFaceName);
  if (!linked_fonts)
    return fallback_font;
  for (size_t i = 0; i < linked_fonts->size(); ++i) {
    LOGFONT logfont = {0};
    logfont.lfCharSet = DEFAULT_CHARSET;
    ::wcscpy_s(logfont.lfFaceName, LF_FACESIZE, linked_fonts->at(i).c_str());
    if (FontContainsCharacter(logfont, dc, character))
      return logfont;
  }
  return fallback_font;
}

}  // namespace win32
}  // namespace ggadget
