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

#ifndef GGADGET_WIN32_MSXML_XML_PARSER_H__
#define GGADGET_WIN32_MSXML_XML_PARSER_H__

#include <string>

#include <ggadget/xml_parser_interface.h>

namespace ggadget {
namespace win32 {

class XMLParser : public XMLParserInterface {
 public:
  XMLParser();
  virtual ~XMLParser();

  virtual bool CheckXMLName(const char* name);

  virtual bool HasXMLDecl(const std::string& content);

  virtual DOMDocumentInterface* CreateDOMDocument();

  virtual bool ConvertContentToUTF8(const std::string& content,
                                    const char* filename,
                                    const char* content_type,
                                    const char* encoding_hint,
                                    const char* encoding_fallback,
                                    std::string* encoding,
                                    std::string* utf8_content);

  virtual bool ParseContentIntoDOM(const std::string& content,
                                   const StringMap* extra_entities,
                                   const char* filename,
                                   const char* content_type,
                                   const char* encoding_hint,
                                   const char* encoding_fallback,
                                   DOMDocumentInterface* domdoc,
                                   std::string* encoding,
                                   std::string* utf8_content);

  virtual bool ParseXMLIntoXPathMap(const std::string& xml,
                                    const StringMap* extra_entities,
                                    const char* filename,
                                    const char* root_element_name,
                                    const char* encoding_hint,
                                    const char* encoding_fallback,
                                    StringMap* table);

  virtual std::string EncodeXMLString(const char* src);
 private:
  HRESULT coinitialize_result_;
  bool is_valid_;
};

}  // namespace win32
}  // namespace ggadget

#endif  // GGADGET_WIN32_MSXML_XML_PARSER_H__
