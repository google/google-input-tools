/*
  Copyright 2011 Google Inc.

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

// This file is mainly for two functions needed by xml parser.
// 1. Preprocess xml to replace some entities with plain text strings. The
//    replacement texts are stored in a specified StringMap, which usually
//    comes from strings.xml. This is used to support different locales.
// 2. Check whether a string is a valid xml name.

#include "xml_parser_int.h"

#include <algorithm>
#include <map>
#include <string>
#include <ggadget/light_map.h>
#include <ggadget/string_utils.h>
#include <ggadget/unicode_utils.h>

namespace {

typedef ggadget::LightMap<std::wstring, std::wstring> UnicodeStringMap;

// Character type functions below are based on
// http://www.w3.org/TR/2008/REC-xml-20081126/

bool IsBaseChar(const wchar_t c) {
  return c >= 0x0041 && c <= 0x005A || c >= 0x0061 && c <= 0x007A ||
      c >= 0x00C0 && c <= 0x00D6 || c >= 0x00D8 && c <= 0x00F6 ||
      c >= 0x00F8 && c <= 0x00FF || c >= 0x0100 && c <= 0x0131 ||
      c >= 0x0134 && c <= 0x013E || c >= 0x0141 && c <= 0x0148 ||
      c >= 0x014A && c <= 0x017E || c >= 0x0180 && c <= 0x01C3 ||
      c >= 0x01CD && c <= 0x01F0 || c >= 0x01F4 && c <= 0x01F5 ||
      c >= 0x01FA && c <= 0x0217 || c >= 0x0250 && c <= 0x02A8 ||
      c >= 0x02BB && c <= 0x02C1 || c == 0x0386 ||
      c >= 0x0388 && c <= 0x038A || c == 0x038C ||
      c >= 0x038E && c <= 0x03A1 || c >= 0x03A3 && c <= 0x03CE ||
      c >= 0x03D0 && c <= 0x03D6 || c == 0x03DA ||
      c == 0x03DC || c == 0x03DE || c == 0x03E0 ||
      c >= 0x03E2 && c <= 0x03F3 || c >= 0x0401 && c <= 0x040C ||
      c >= 0x040E && c <= 0x044F || c >= 0x0451 && c <= 0x045C ||
      c >= 0x045E && c <= 0x0481 || c >= 0x0490 && c <= 0x04C4 ||
      c >= 0x04C7 && c <= 0x04C8 || c >= 0x04CB && c <= 0x04CC ||
      c >= 0x04D0 && c <= 0x04EB || c >= 0x04EE && c <= 0x04F5 ||
      c >= 0x04F8 && c <= 0x04F9 || c >= 0x0531 && c <= 0x0556 ||
      c == 0x0559 || c >= 0x0561 && c <= 0x0586 ||
      c >= 0x05D0 && c <= 0x05EA || c >= 0x05F0 && c <= 0x05F2 ||
      c >= 0x0621 && c <= 0x063A || c >= 0x0641 && c <= 0x064A ||
      c >= 0x0671 && c <= 0x06B7 || c >= 0x06BA && c <= 0x06BE ||
      c >= 0x06C0 && c <= 0x06CE || c >= 0x06D0 && c <= 0x06D3 ||
      c == 0x06D5 || c >= 0x06E5 && c <= 0x06E6 ||
      c >= 0x0905 && c <= 0x0939 || c == 0x093D ||
      c >= 0x0958 && c <= 0x0961 || c >= 0x0985 && c <= 0x098C ||
      c >= 0x098F && c <= 0x0990 || c >= 0x0993 && c <= 0x09A8 ||
      c >= 0x09AA && c <= 0x09B0 || c == 0x09B2 ||
      c >= 0x09B6 && c <= 0x09B9 || c >= 0x09DC && c <= 0x09DD ||
      c >= 0x09DF && c <= 0x09E1 || c >= 0x09F0 && c <= 0x09F1 ||
      c >= 0x0A05 && c <= 0x0A0A || c >= 0x0A0F && c <= 0x0A10 ||
      c >= 0x0A13 && c <= 0x0A28 || c >= 0x0A2A && c <= 0x0A30 ||
      c >= 0x0A32 && c <= 0x0A33 || c >= 0x0A35 && c <= 0x0A36 ||
      c >= 0x0A38 && c <= 0x0A39 || c >= 0x0A59 && c <= 0x0A5C ||
      c == 0x0A5E || c >= 0x0A72 && c <= 0x0A74 ||
      c >= 0x0A85 && c <= 0x0A8B || c == 0x0A8D ||
      c >= 0x0A8F && c <= 0x0A91 || c >= 0x0A93 && c <= 0x0AA8 ||
      c >= 0x0AAA && c <= 0x0AB0 || c >= 0x0AB2 && c <= 0x0AB3 ||
      c >= 0x0AB5 && c <= 0x0AB9 || c == 0x0ABD ||
      c == 0x0AE0 || c >= 0x0B05 && c <= 0x0B0C ||
      c >= 0x0B0F && c <= 0x0B10 || c >= 0x0B13 && c <= 0x0B28 ||
      c >= 0x0B2A && c <= 0x0B30 || c >= 0x0B32 && c <= 0x0B33 ||
      c >= 0x0B36 && c <= 0x0B39 || c == 0x0B3D ||
      c >= 0x0B5C && c <= 0x0B5D || c >= 0x0B5F && c <= 0x0B61 ||
      c >= 0x0B85 && c <= 0x0B8A || c >= 0x0B8E && c <= 0x0B90 ||
      c >= 0x0B92 && c <= 0x0B95 || c >= 0x0B99 && c <= 0x0B9A ||
      c == 0x0B9C || c >= 0x0B9E && c <= 0x0B9F ||
      c >= 0x0BA3 && c <= 0x0BA4 || c >= 0x0BA8 && c <= 0x0BAA ||
      c >= 0x0BAE && c <= 0x0BB5 || c >= 0x0BB7 && c <= 0x0BB9 ||
      c >= 0x0C05 && c <= 0x0C0C || c >= 0x0C0E && c <= 0x0C10 ||
      c >= 0x0C12 && c <= 0x0C28 || c >= 0x0C2A && c <= 0x0C33 ||
      c >= 0x0C35 && c <= 0x0C39 || c >= 0x0C60 && c <= 0x0C61 ||
      c >= 0x0C85 && c <= 0x0C8C || c >= 0x0C8E && c <= 0x0C90 ||
      c >= 0x0C92 && c <= 0x0CA8 || c >= 0x0CAA && c <= 0x0CB3 ||
      c >= 0x0CB5 && c <= 0x0CB9 || c == 0x0CDE ||
      c >= 0x0CE0 && c <= 0x0CE1 || c >= 0x0D05 && c <= 0x0D0C ||
      c >= 0x0D0E && c <= 0x0D10 || c >= 0x0D12 && c <= 0x0D28 ||
      c >= 0x0D2A && c <= 0x0D39 || c >= 0x0D60 && c <= 0x0D61 ||
      c >= 0x0E01 && c <= 0x0E2E || c == 0x0E30 ||
      c >= 0x0E32 && c <= 0x0E33 || c >= 0x0E40 && c <= 0x0E45 ||
      c >= 0x0E81 && c <= 0x0E82 || c == 0x0E84 ||
      c >= 0x0E87 && c <= 0x0E88 || c == 0x0E8A ||
      c == 0x0E8D || c >= 0x0E94 && c <= 0x0E97 ||
      c >= 0x0E99 && c <= 0x0E9F || c >= 0x0EA1 && c <= 0x0EA3 ||
      c == 0x0EA5 || c == 0x0EA7 || c >= 0x0EAA && c <= 0x0EAB ||
      c >= 0x0EAD && c <= 0x0EAE || c == 0x0EB0 ||
      c >= 0x0EB2 && c <= 0x0EB3 || c == 0x0EBD ||
      c >= 0x0EC0 && c <= 0x0EC4 || c >= 0x0F40 && c <= 0x0F47 ||
      c >= 0x0F49 && c <= 0x0F69 || c >= 0x10A0 && c <= 0x10C5 ||
      c >= 0x10D0 && c <= 0x10F6 || c == 0x1100 ||
      c >= 0x1102 && c <= 0x1103 || c >= 0x1105 && c <= 0x1107 ||
      c == 0x1109 || c >= 0x110B && c <= 0x110C ||
      c >= 0x110E && c <= 0x1112 || c == 0x113C ||
      c == 0x113E || c == 0x1140 || c == 0x114C ||
      c == 0x114E || c == 0x1150 || c >= 0x1154 && c <= 0x1155 ||
      c == 0x1159 || c >= 0x115F && c <= 0x1161 ||
      c == 0x1163 || c == 0x1165 || c == 0x1167 ||
      c == 0x1169 || c >= 0x116D && c <= 0x116E ||
      c >= 0x1172 && c <= 0x1173 || c == 0x1175 ||
      c == 0x119E || c == 0x11A8 || c == 0x11AB ||
      c >= 0x11AE && c <= 0x11AF || c >= 0x11B7 && c <= 0x11B8 ||
      c == 0x11BA || c >= 0x11BC && c <= 0x11C2 ||
      c == 0x11EB || c == 0x11F0 || c == 0x11F9 ||
      c >= 0x1E00 && c <= 0x1E9B || c >= 0x1EA0 && c <= 0x1EF9 ||
      c >= 0x1F00 && c <= 0x1F15 || c >= 0x1F18 && c <= 0x1F1D ||
      c >= 0x1F20 && c <= 0x1F45 || c >= 0x1F48 && c <= 0x1F4D ||
      c >= 0x1F50 && c <= 0x1F57 || c == 0x1F59 ||
      c == 0x1F5B || c == 0x1F5D || c >= 0x1F5F && c <= 0x1F7D ||
      c >= 0x1F80 && c <= 0x1FB4 || c >= 0x1FB6 && c <= 0x1FBC ||
      c == 0x1FBE || c >= 0x1FC2 && c <= 0x1FC4 ||
      c >= 0x1FC6 && c <= 0x1FCC || c >= 0x1FD0 && c <= 0x1FD3 ||
      c >= 0x1FD6 && c <= 0x1FDB || c >= 0x1FE0 && c <= 0x1FEC ||
      c >= 0x1FF2 && c <= 0x1FF4 || c >= 0x1FF6 && c <= 0x1FFC ||
      c == 0x2126 || c >= 0x212A && c <= 0x212B ||
      c == 0x212E || c >= 0x2180 && c <= 0x2182 ||
      c >= 0x3041 && c <= 0x3094 || c >= 0x30A1 && c <= 0x30FA ||
      c >= 0x3105 && c <= 0x312C || c >= 0xAC00 && c <= 0xD7A3;
}

bool IsIdeographic(const wchar_t c) {
  return c >= 0x4E00 && c <= 0x9FA5 || c == 0x3007 ||
      c >= 0x3021 && c <= 0x3029;
}

bool IsLetter(const wchar_t c) {
  return IsBaseChar(c) || IsIdeographic(c);
}

bool IsCombiningChar(const wchar_t c) {
  return c >= 0x0300 && c <= 0x0345 || c >= 0x0360 && c <= 0x0361 ||
      c >= 0x0483 && c <= 0x0486 || c >= 0x0591 && c <= 0x05A1 ||
      c >= 0x05A3 && c <= 0x05B9 || c >= 0x05BB && c <= 0x05BD ||
      c == 0x05BF || c >= 0x05C1 && c <= 0x05C2 ||
      c == 0x05C4 || c >= 0x064B && c <= 0x0652 ||
      c == 0x0670 || c >= 0x06D6 && c <= 0x06DC ||
      c >= 0x06DD && c <= 0x06DF || c >= 0x06E0 && c <= 0x06E4 ||
      c >= 0x06E7 && c <= 0x06E8 || c >= 0x06EA && c <= 0x06ED ||
      c >= 0x0901 && c <= 0x0903 || c == 0x093C ||
      c >= 0x093E && c <= 0x094C || c == 0x094D ||
      c >= 0x0951 && c <= 0x0954 || c >= 0x0962 && c <= 0x0963 ||
      c >= 0x0981 && c <= 0x0983 || c == 0x09BC ||
      c == 0x09BE || c == 0x09BF || c >= 0x09C0 && c <= 0x09C4 ||
      c >= 0x09C7 && c <= 0x09C8 || c >= 0x09CB && c <= 0x09CD ||
      c == 0x09D7 || c >= 0x09E2 && c <= 0x09E3 ||
      c == 0x0A02 || c == 0x0A3C || c == 0x0A3E ||
      c == 0x0A3F || c >= 0x0A40 && c <= 0x0A42 ||
      c >= 0x0A47 && c <= 0x0A48 || c >= 0x0A4B && c <= 0x0A4D ||
      c >= 0x0A70 && c <= 0x0A71 || c >= 0x0A81 && c <= 0x0A83 ||
      c == 0x0ABC || c >= 0x0ABE && c <= 0x0AC5 ||
      c >= 0x0AC7 && c <= 0x0AC9 || c >= 0x0ACB && c <= 0x0ACD ||
      c >= 0x0B01 && c <= 0x0B03 || c == 0x0B3C ||
      c >= 0x0B3E && c <= 0x0B43 || c >= 0x0B47 && c <= 0x0B48 ||
      c >= 0x0B4B && c <= 0x0B4D || c >= 0x0B56 && c <= 0x0B57 ||
      c >= 0x0B82 && c <= 0x0B83 || c >= 0x0BBE && c <= 0x0BC2 ||
      c >= 0x0BC6 && c <= 0x0BC8 || c >= 0x0BCA && c <= 0x0BCD ||
      c == 0x0BD7 || c >= 0x0C01 && c <= 0x0C03 ||
      c >= 0x0C3E && c <= 0x0C44 || c >= 0x0C46 && c <= 0x0C48 ||
      c >= 0x0C4A && c <= 0x0C4D || c >= 0x0C55 && c <= 0x0C56 ||
      c >= 0x0C82 && c <= 0x0C83 || c >= 0x0CBE && c <= 0x0CC4 ||
      c >= 0x0CC6 && c <= 0x0CC8 || c >= 0x0CCA && c <= 0x0CCD ||
      c >= 0x0CD5 && c <= 0x0CD6 || c >= 0x0D02 && c <= 0x0D03 ||
      c >= 0x0D3E && c <= 0x0D43 || c >= 0x0D46 && c <= 0x0D48 ||
      c >= 0x0D4A && c <= 0x0D4D || c == 0x0D57 ||
      c == 0x0E31 || c >= 0x0E34 && c <= 0x0E3A ||
      c >= 0x0E47 && c <= 0x0E4E || c == 0x0EB1 ||
      c >= 0x0EB4 && c <= 0x0EB9 || c >= 0x0EBB && c <= 0x0EBC ||
      c >= 0x0EC8 && c <= 0x0ECD || c >= 0x0F18 && c <= 0x0F19 ||
      c == 0x0F35 || c == 0x0F37 || c == 0x0F39 ||
      c == 0x0F3E || c == 0x0F3F || c >= 0x0F71 && c <= 0x0F84 ||
      c >= 0x0F86 && c <= 0x0F8B || c >= 0x0F90 && c <= 0x0F95 ||
      c == 0x0F97 || c >= 0x0F99 && c <= 0x0FAD ||
      c >= 0x0FB1 && c <= 0x0FB7 || c == 0x0FB9 ||
      c >= 0x20D0 && c <= 0x20DC || c == 0x20E1 ||
      c >= 0x302A && c <= 0x302F || c == 0x3099 ||
      c == 0x309A;
}

bool IsDigit(const wchar_t c) {
  return c >= 0x0030 && c <= 0x0039 || c >= 0x0660 && c <= 0x0669 ||
      c >= 0x06F0 && c <= 0x06F9 || c >= 0x0966 && c <= 0x096F ||
      c >= 0x09E6 && c <= 0x09EF || c >= 0x0A66 && c <= 0x0A6F ||
      c >= 0x0AE6 && c <= 0x0AEF || c >= 0x0B66 && c <= 0x0B6F ||
      c >= 0x0BE7 && c <= 0x0BEF || c >= 0x0C66 && c <= 0x0C6F ||
      c >= 0x0CE6 && c <= 0x0CEF || c >= 0x0D66 && c <= 0x0D6F ||
      c >= 0x0E50 && c <= 0x0E59 || c >= 0x0ED0 && c <= 0x0ED9 ||
      c >= 0x0F20 && c <= 0x0F29;
}

bool IsExtender(const wchar_t c) {
  return c == 0x00B7 || c == 0x02D0 || c == 0x02D1 ||
      c == 0x0387 || c == 0x0640 || c == 0x0E46 ||
      c == 0x0EC6 || c == 0x3005 || c >= 0x3031 && c <= 0x3035 ||
      c >= 0x309D && c <= 0x309E || c >= 0x30FC && c <= 0x30FE;
}

// Convert entity table from UTF8 to UTF16
bool ConvertStringMapToUTF16(const ggadget::StringMap *utf8_entities,
                             UnicodeStringMap *unicode_entities) {
  using ggadget::win32::xml_parser_internal::EncodeXMLString;
  if (unicode_entities == NULL)
    return false;
  unicode_entities->clear();
  if (utf8_entities == NULL)
    return true;
  for (ggadget::StringMap::const_iterator iter = utf8_entities->begin();
       iter != utf8_entities->end();
       ++iter) {
    std::wstring entity_name, entity_value;
    ggadget::ConvertStringUTF8ToUTF16(ggadget::ToLower(iter->first),
                                      &entity_name);
    ggadget::ConvertStringUTF8ToUTF16(
        EncodeXMLString(iter->second.c_str()),
        &entity_value);
    (*unicode_entities)[entity_name] = entity_value;
  }
  return true;
}

#define DEFINE_XML_TAG_TO_SKIP(name, start_sign, end_sign)                     \
  const wchar_t* k##name##Start = (start_sign);                                \
  const wchar_t* k##name##End = (end_sign);                                    \
  const size_t k##name##StartLength = wcslen(start_sign);                      \
  const size_t k##name##EndLength = wcslen(end_sign)

DEFINE_XML_TAG_TO_SKIP(CDATA, L"<![CDATA[", L"]]>");
DEFINE_XML_TAG_TO_SKIP(Comment, L"<!--", L"-->");
DEFINE_XML_TAG_TO_SKIP(Entity, L"<!ENTITY", L">");
DEFINE_XML_TAG_TO_SKIP(ProcessInstruction, L"<?", L"?>");

// If [content, content_end) starts with a string enclosed by "start_sign" and
// "end_sign", return the ending position of "end_sign" in "content";
// Otherwise, return the begining position of "content".
const wchar_t* DetectAndSkip(const wchar_t *content,
                             const wchar_t *content_end,
                             const wchar_t *start_sign,
                             int start_sign_length,
                             const wchar_t *end_sign,
                             int end_sign_length) {
  ASSERT(content);
  ASSERT(content_end);
  ASSERT(start_sign && *start_sign);
  ASSERT(end_sign && *end_sign);
  if (content_end - content >= start_sign_length &&
      wcsncmp(content, start_sign, start_sign_length) == 0 ) {
    const wchar_t *end_sign_position = std::search(content + start_sign_length,
                                                   content_end,
                                                   end_sign,
                                                   end_sign + end_sign_length);
    if (end_sign_position == content_end) {
      // end sign is not found
      return content;
    }
    return end_sign_position + end_sign_length;
  }
  return content;
}

bool ReplaceEntityByText(const UnicodeStringMap* entities,
                         const wchar_t* orignal_string,
                         int orignal_length,
                         std::wstring* result_content) {
  ASSERT(entities != NULL);
  if (orignal_string == NULL || result_content == NULL)
    return false;
  const wchar_t* current = orignal_string;
  const wchar_t* end = current + orignal_length;

  while (current < end) {
    if (*current == L'<') {
      // Skip processing instructions, CDATAs, comments and entity definitions.
      const wchar_t *skip_from = current;
      current = DetectAndSkip(current, end,
                              kCDATAStart, kCDATAStartLength,
                              kCDATAEnd, kCDATAEndLength);
      current = DetectAndSkip(current, end,
                              kCommentStart, kCommentStartLength,
                              kCommentEnd, kCommentEndLength);
      current = DetectAndSkip(current, end,
                              kEntityStart, kEntityStartLength,
                              kEntityEnd, kEntityEndLength);
      current = DetectAndSkip(current, end,
                              kProcessInstructionStart,
                              kProcessInstructionStartLength,
                              kProcessInstructionEnd,
                              kProcessInstructionEndLength);
      if (skip_from == current) {
        // no character is skipped.
        *result_content += '<';
        ++current;
      } else {
        result_content->append(skip_from, current - skip_from);
      }
    } else if (*current == '&') {
      // It maybe a entity reference.
      const wchar_t *entity_start = current + 1;
      const wchar_t *entity_end = entity_start;
      while (entity_end < end && *entity_end != L'&' && *entity_end != L';' &&
             *entity_end != L'<' &&
             !ggadget::win32::xml_parser_internal::IsSpaceChar(*entity_end)) {
        ++entity_end;
      }
      if (entity_end < end && *entity_end == ';') {
        // Yes, it is a entity reference.
        std::wstring entity_name(entity_start, entity_end - entity_start);
        // transform entity_name to lower case
        std::transform(entity_start, entity_end, entity_name.begin(), towlower);
        if (wcscmp(entity_name.c_str(), L"amp") == 0 ||
            wcscmp(entity_name.c_str(), L"lt") == 0 ||
            wcscmp(entity_name.c_str(), L"gt") == 0 ||
            wcscmp(entity_name.c_str(), L"quot") == 0 ||
            wcscmp(entity_name.c_str(), L"apos") == 0) {
          // The entity is system defined.
          *result_content += L"&";
          *result_content += entity_name;
          *result_content += L";";
        } else {
          UnicodeStringMap::const_iterator iter = entities->find(entity_name);
          if (iter == entities->end()) {
            // The entity is not defined.
            *result_content += L"&";
            *result_content += entity_name;
            *result_content += L";";
          } else {
            result_content->append(iter->second);
          }
        }
        current = entity_end + 1;
      } else {
        // It's not a valid entity. Do nothing but copying the string.
        result_content->append(current, entity_end - current);
        current = entity_end;
      }
    } else {
      *result_content += *current;
      ++current;
    }
  }
  return true;
}

const wchar_t kSpaceChars[] = L" \r\n\t";

}  // namespace

namespace ggadget {
namespace win32 {
namespace xml_parser_internal {

bool ValidateXMLName(const char *name) {
  const char* current = name;
  if (name == NULL || *name == '\0')
    return false;

  // Check first character
  if (isalpha(*current) || *current == '_')
    ++current;

  // Check other characters
  while (isalpha(*current) || isdigit(*current) || *current == '_' ||
         *current == '-' || *current == '.' || *current == ':') {
    ++current;
  }

  if (*current == '\0')
    return true;

  // Can not start with "xml"
  if (StartWithNoCase(name, "xml"))
    return false;

  std::wstring utf16_str;
  ConvertStringUTF8ToUTF16(name, strlen(name), &utf16_str);
  const wchar_t *utf16 = utf16_str.c_str();
  if (!IsLetter(*utf16) && *utf16 != L'_' && *utf16 != L':')
    return false;
  ++utf16;
  while (IsLetter(*utf16) || IsDigit(*utf16) || *utf16 == L'.' ||
      *utf16 == L':' || *utf16 == L'-' || *utf16 == L'_' ||
      IsCombiningChar(*utf16) || IsExtender(*utf16)) {
    ++utf16;
  }
  if (*utf16 != 0)
    return false;
  return true;
}

bool IsSpaceChar(wchar_t c) {
  return wcschr(kSpaceChars, c) != NULL;
}

std::string EncodeXMLString(const char* src) {
  if (!src || !*src)
    return std::string();
  const char* cur = src;
  std::string result;
  while (*cur != '\0') {
    // By default one have to encode at least '<', '>', '"' and '&' !
    switch (*cur) {
      case '<':
        result += "&lt;";
        break;
      case '>':
        result += "&gt;";
        break;
      case '&':
        result += "&amp;";
        break;
      case '"':
        result += "&quot;";
        break;
      case '\r':
        result += "&#13;";
        break;
      case '\'':
        result += "&apos;";
        break;
      default:
        result += *cur;
    }
    cur++;
  }
  return result;
}

bool PreprocessXMLStringEntity(const StringMap* extra_entities,
                               const wchar_t* content,
                               int content_length,
                               std::wstring *result_content) {
  if (content == NULL || result_content == NULL)
    return false;
  result_content->clear();
  UnicodeStringMap entities;
  if (ConvertStringMapToUTF16(extra_entities, &entities) &&
      ReplaceEntityByText(&entities, content, content_length, result_content)) {
    return true;
  }
  return false;
}

}  // namespace xml_parser_internal
}  // namespace win32
}  // namespace ggadget
