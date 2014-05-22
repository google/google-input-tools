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

#ifndef GGADGET_WIN32_XML_PARSER_INT_H__
#define GGADGET_WIN32_XML_PARSER_INT_H__

#include <string>
#include <ggadget/string_utils.h>

namespace ggadget {
namespace win32 {
namespace xml_parser_internal {

// Repalce references to extra_entities in content.
bool PreprocessXMLStringEntity(const StringMap* extra_entities,
                               const wchar_t* content,
                               int content_length,
                               std::wstring* result_content);

// Check whether is a valid xml name.
bool ValidateXMLName(const char* name);

std::string EncodeXMLString(const char* src);

bool IsSpaceChar(wchar_t c);

}  // namespace xml_parser_internal
}  // namespace win32
}  // namespace ggadget

#endif  // GGADGET_WIN32_XML_PARSER_INT_H__
