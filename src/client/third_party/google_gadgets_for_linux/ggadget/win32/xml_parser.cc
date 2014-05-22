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

// This is a windows-based implementation of xml parser using MSXML lib.
// It implements xml_parser_interface.
//
// Notice:
// 1. Currently it supports only "UTF8", "UTF16LE" and "UTF16BE" encoding.
//    Other encoding types will fail.
// 2. This xml parser disables Document Type Definition and it should not be
//    used in xml_http_request in future. Any xml files with DTD will be
//    failed to parse.
// 3. The extra_entities passed into the xml parser will be treated as plain
//    text. That means some special characters in entity values, such as '<'
//    and '&', will be escaped before the replacement.

#include "xml_parser.h"

#include <objbase.h>
#include <msxml2.h>
#include <windows.h>
#include <cstring>
#include <cmath>

#include <ggadget/logger.h>
#include <ggadget/string_utils.h>
#include <ggadget/xml_parser_interface.h>
#include <ggadget/xml_dom.h>
#include <ggadget/win32/xml_parser_int.h>

namespace ggadget {
namespace win32 {

namespace {
const wchar_t kProhibitDTD[] = L"ProhibitDTD";

const char kXMLTag[] = {
  '<', '?', 'x', 'm', 'l', ' '
};
const char kXMLTagUTF8[] = {
  '\xEF', '\xBB', '\xBF', '<', '?', 'x', 'm', 'l', ' '
};
const char kXMLTagUTF16LE[] = {
  '\xFF', '\xFE', '<', 0, '?', 0, 'x', 0, 'm', 0, 'l', 0, ' ', 0
};
const char kXMLTagUTF16BE[] = {
  '\xFE', '\xFF', 0, '<', 0, '?', 0, 'x', 0, 'm', 0, 'l', 0, ' '
};
const char kXMLTagBOMLessUTF16LE[] = {
  '<', 0, '?', 0, 'x', 0, 'm', 0, 'l', 0, ' ', 0
};
const char kXMLTagBOMLessUTF16BE[] = {
  0, '<', 0, '?', 0, 'x', 0, 'm', 0, 'l', 0, ' '
};
const char kXMLTagUTF32LE[] = {
  '\xFF', '\xFE', 0, 0, '<', 0, 0, 0, '?', 0, 0, 0,
  'x', 0, 0, 0, 'm', 0, 0, 0, 'l', 0, 0, 0, ' ', 0, 0, 0
};
const char kXMLTagUTF32BE[] = {
  0, 0, '\xFE', '\xFF', 0, 0, 0, '<', 0, 0, 0, '?',
  0, 0, 0, 'x', 0, 0, 0, 'm', 0, 0, 0, 'l', 0, 0, 0, ' '
};
// BOM-less UTF32 is seldom used, so we won't check.

#define STARTS_WITH(content_ptr, content_size, pattern) \
    ((content_size) >= sizeof(pattern) && \
     memcmp((content_ptr), (pattern), sizeof(pattern)) == 0)

const int32_t kUnknown = -1;
const int32_t kUTF8 = 65001;
const int32_t kUTF16LE = 12000;
const int32_t kUTF16BE = 12001;

int32_t GetCodepageByEncodingString(const char* encoding) {
  if (StartWithNoCase(encoding, "utf")) {
    encoding += 3;
    if (*encoding == '-' || *encoding == '_')
      ++encoding;
    if (_stricmp(encoding, "8") == 0)
      return kUTF8;
    else if (_stricmp(encoding, "16") == 0 || _stricmp(encoding, "16le") == 0)
      return kUTF16LE;
    else if (_stricmp(encoding, "16be") == 0)
      return kUTF16BE;
  }
  return kUnknown;
}

bool ConvertStringToUTF8AndUTF16(const char* content,
                                 int content_length,
                                 const char* encoding,
                                 std::wstring* utf16_content,
                                 std::string* utf8_content) {
  if (content == NULL || encoding == NULL ||
      (utf16_content == NULL && utf8_content == NULL)) {
    return false;
  }
  if (utf16_content != NULL) utf16_content->clear();
  if (utf8_content != NULL) utf8_content->clear();
  int32_t codepage = GetCodepageByEncodingString(encoding);
  if (codepage == kUTF8) {
    if (utf16_content != NULL)
      ConvertStringUTF8ToUTF16(content, content_length, utf16_content);
    if (utf8_content != NULL)
      utf8_content->assign(content, content_length);
  } else if(codepage == kUTF16LE) {
    // An encoding unit of utf16le is 2 bytes
    if (content_length % 2 != 0) return false;
    const wchar_t* utf16_content_ptr =
        reinterpret_cast<const wchar_t*>(content);
    const int utf16_length = content_length / 2;
    if (utf16_content != NULL)
      utf16_content->assign(utf16_content_ptr, utf16_length);
    if (utf8_content != NULL)
      ConvertStringUTF16ToUTF8(utf16_content_ptr, utf16_length, utf8_content);
  } else if(codepage == kUTF16BE) {
    // An encoding unit of utf16be is 2 bytes
    if (content_length % 2 != 0) return false;
    std::wstring utf16_string;
    utf16_string.reserve(content_length / 2);
    for (int i = 0; i < content_length; i += 2) {
      wchar_t utf16_char = static_cast<wchar_t>(content[i]) << 8 |
                           static_cast<wchar_t>(content[i + 1]);
      utf16_string.push_back(utf16_char);
    }
    if (utf8_content != NULL)
      ConvertStringUTF16ToUTF8(utf16_string, utf8_content);
    if (utf16_content != NULL)
      utf16_string.swap(*utf16_content);
  } else {
    // Others are not supported
    LOG( "Encoding %s is not supprted.", encoding);
    return false;
  }

  if ((utf16_content != NULL && !IsLegalUTF16String(*utf16_content)) ||
      (utf8_content != NULL && !IsLegalUTF8String(*utf8_content))) {
    if (utf16_content) utf16_content->clear();
    if (utf8_content) utf8_content->clear();
    return false;
  }
  return true;
}

std::string GetXMLEncodingDecl(const std::string& xml) {
  std::string result;
  if (!STARTS_WITH(xml.c_str(), xml.size(), kXMLTag) &&
      !STARTS_WITH(xml.c_str(), xml.size(), kXMLTagUTF8)) {
    return result;
  }
  size_t end_decl_pos = xml.find("?>");
  if (end_decl_pos == std::string::npos)
    return result;
  size_t encoding_pos = xml.rfind(" encoding=\"", end_decl_pos);
  if (encoding_pos == std::string::npos)
    return result;
  encoding_pos += 11;
  size_t end_encoding_pos = xml.find('"', encoding_pos);
  if (end_encoding_pos == std::string::npos)
    return result;

  return xml.substr(encoding_pos, end_encoding_pos - encoding_pos);
}

void ReplaceXMLEncodingDecl(std::wstring* xml) {
  if (!STARTS_WITH(xml->c_str(), xml->size(), kXMLTag) &&
      !STARTS_WITH(xml->c_str(), xml->size(), kXMLTagUTF8)) {
    return;
  }
  size_t end_decl_pos = xml->find(L"?>");
  if (end_decl_pos == std::string::npos)
    return;
  size_t encoding_pos = xml->rfind(L" encoding=\"", end_decl_pos);
  if (encoding_pos == std::string::npos)
    return;
  size_t end_encoding_pos = xml->find(L'"', encoding_pos + 11);
  if (end_encoding_pos == std::string::npos)
    return;
  xml->replace(encoding_pos, end_encoding_pos - encoding_pos + 1,
               L" encoding=\"UTF-16\"");
}

bool IsBlankText(const wchar_t* text) {
  for (const wchar_t* p = text; *p; p++) {
    if (!xml_parser_internal::IsSpaceChar(*p))
      return false;
  }
  return true;
}

::DOMNodeType GetNodeType(::IXMLDOMNode* xml_node) {
  ::DOMNodeType node_type;
  if (xml_node && SUCCEEDED(xml_node->get_nodeType(&node_type)))
    return node_type;
  return NODE_INVALID;
}

typedef HRESULT (STDMETHODCALLTYPE ::IXMLDOMNode::*GetBStrProperty)(::BSTR*);

template<GetBStrProperty get_bstr>
bool GetProperty(::IXMLDOMNode* xml_node, std::string* property_value) {
  ::BSTR bstr = NULL;
  if (!property_value)
    return false;
  property_value->clear();
  if (SUCCEEDED((xml_node->*get_bstr)(&bstr)) && bstr != NULL) {
    ConvertStringUTF16ToUTF8(static_cast<wchar_t*>(bstr),
                             SysStringLen(bstr),
                             property_value);
    SysFreeString(bstr);
    return true;
  }
  return false;
}

bool IsTextNode(::IXMLDOMNode* xml_node) {
  return xml_node != NULL && (GetNodeType(xml_node) == NODE_TEXT ||
                              GetNodeType(xml_node) == NODE_ENTITY_REFERENCE);
}

bool HasTextNodeSibling(::IXMLDOMNode* xml_node) {
  ::IXMLDOMNode* previous_sibling = NULL;
  bool result = false;
  if (SUCCEEDED(xml_node->get_previousSibling(&previous_sibling)) &&
      previous_sibling != NULL) {
    if (IsTextNode(previous_sibling))
      result = true;
    previous_sibling->Release();
  }
  if (result) return true;
  ::IXMLDOMNode* next_sibling = NULL;
  if (SUCCEEDED(xml_node->get_nextSibling(&next_sibling)) &&
      next_sibling != NULL) {
    if (IsTextNode(next_sibling))
      result = true;
    next_sibling->Release();
  }
  return result;
}

void ConvertCharacterDataIntoDOM(DOMDocumentInterface* domdoc,
                                 DOMNodeInterface* parent,
                                 ::IXMLDOMNode* xml_node) {

  UTF16String utf16_text;
  ::DOMNodeType node_type = GetNodeType(xml_node);
  ::BSTR text = NULL;
  if (node_type == NODE_ENTITY_REFERENCE) {
    ::IXMLDOMNode* child_node = NULL;
    if (SUCCEEDED(xml_node->get_firstChild(&child_node)) &&
        child_node != NULL) {
      child_node->get_text(&text);
      child_node->Release();
    }
  } else if (node_type == NODE_COMMENT || node_type == NODE_CDATA_SECTION) {
    xml_node->get_text(&text);
  } else {
    xml_node->get_text(&text);
  }
  if (text) {
    if (domdoc->PreservesWhiteSpace() ||
        node_type != NODE_TEXT ||
        HasTextNodeSibling(xml_node) ||
        !IsBlankText(text)) {
      // Don't trim the text. The caller can trim based on their own
      // requirements.
      utf16_text.assign(text);
    }
    SysFreeString(text);
  }

  DOMCharacterDataInterface* data = NULL;
  switch (node_type) {
    case NODE_TEXT:
      // Don't create empty text nodes.
      if (!utf16_text.empty())
        data = domdoc->CreateTextNode(utf16_text.c_str());
      break;
    case NODE_ENTITY_REFERENCE:
      data = domdoc->CreateTextNode(utf16_text.c_str());
      break;
    case NODE_CDATA_SECTION:
      data = domdoc->CreateCDATASection(utf16_text.c_str());
      break;
    case NODE_COMMENT:
      data = domdoc->CreateComment(utf16_text.c_str());
      break;
    default:
      ASSERT(false);
      break;
  }
  if (data) {
    //Row number of node is not supported
    parent->AppendChild(data);
  }
}

void ConvertPIIntoDOM(DOMDocumentInterface* domdoc,
                      DOMNodeInterface* parent,
                      ::IXMLDOMNode* xmlpi) {
  std::string target;
  if (GetProperty<&::IXMLDOMNode::get_nodeName>(xmlpi, &target) &&
      GadgetStrCmp(target.c_str(), "xml") == 0 ) {
    // ignore xml declarnode
    return;
  }
  std::string node_content;
  GetProperty<&::IXMLDOMNode::get_text>(xmlpi, &node_content);

  DOMProcessingInstructionInterface* processing_instruction;
  domdoc->CreateProcessingInstruction(target.c_str(),
                                      node_content.c_str(),
                                      &processing_instruction);
  if (processing_instruction)
    parent->AppendChild(processing_instruction);
}

void ConvertElementIntoDOM(DOMDocumentInterface* domdoc,
                           DOMNodeInterface* parent,
                           ::IXMLDOMNode* xml_element);

void ConvertChildrenIntoDOM(DOMDocumentInterface* domdoc,
                            DOMNodeInterface* parent,
                            ::IXMLDOMNode* xml_node) {
  ::IXMLDOMNode* child_xmlnode = NULL;
  HRESULT hr = xml_node->get_firstChild(&child_xmlnode);
  while (SUCCEEDED(hr) && child_xmlnode) {
    ::DOMNodeType node_type;
    if (SUCCEEDED(child_xmlnode->get_nodeType(&node_type))) {
      switch (node_type) {
        case NODE_ELEMENT:
          ConvertElementIntoDOM(domdoc, parent, child_xmlnode);
          break;
        case NODE_TEXT:
        case NODE_ENTITY_REFERENCE:
        case NODE_CDATA_SECTION:
        case NODE_COMMENT:
          ConvertCharacterDataIntoDOM(domdoc, parent, child_xmlnode);
          break;
        case NODE_PROCESSING_INSTRUCTION:
          ConvertPIIntoDOM(domdoc, parent, child_xmlnode);
          break;
        case NODE_DOCUMENT_TYPE:
          break;
        default:
          DLOG("Ignore XML Node of type %d", GetNodeType(child_xmlnode));
          break;
      }
    }
    ::IXMLDOMNode* sibling_xmlnode = NULL;
    hr = child_xmlnode->get_nextSibling(&sibling_xmlnode);
    child_xmlnode->Release();
    child_xmlnode = sibling_xmlnode;
  }
  if (child_xmlnode)
    child_xmlnode->Release();
}

void ConvertElementIntoDOM(DOMDocumentInterface* domdoc,
                           DOMNodeInterface* parent,
                           ::IXMLDOMNode* xml_element) {
  DOMElementInterface* element;
  std::string name;
  GetProperty<&::IXMLDOMNode::get_nodeName>(xml_element, &name);
  domdoc->CreateElement(name.c_str(), &element);
  if (!element || DOM_NO_ERR != parent->AppendChild(element)) {
    // Unlikely to happen.
    DLOG("Failed to create DOM element or to add it to parent");
    delete element;
    return;
  }

  // We don't support full DOM2 namespaces, but we must keep all namespace
  // related information in the result DOM.
  std::string element_ns;
  GetProperty<&::IXMLDOMNode::get_namespaceURI>(xml_element, &element_ns);
  std::string element_prefix;
  if (GetProperty<&::IXMLDOMNode::get_prefix>(xml_element, &element_prefix))
    element->SetPrefix(element_prefix.c_str());

  ::IXMLDOMNamedNodeMap* named_node_map = NULL;
  if (SUCCEEDED(xml_element->get_attributes(&named_node_map)) &&
      named_node_map != NULL) {
    ::IXMLDOMNode* attribute_node = NULL;
    ::IXMLDOMNode* last_attribute_node = NULL;
    while (SUCCEEDED(named_node_map->nextNode(&attribute_node)) &&
           attribute_node != NULL) {
      if (last_attribute_node)
        last_attribute_node->Release();
      std::string attribute_name;
      if (GetProperty<&::IXMLDOMNode::get_nodeName>(attribute_node,
                                                    &attribute_name)) {
        DOMAttrInterface* attribute = NULL;
        domdoc->CreateAttribute(attribute_name.c_str(), &attribute);
        if (!attribute || DOM_NO_ERR != element->SetAttributeNode(attribute)) {
          // Unlikely to happen.
          DLOG("Failed to create DOM attribute or to add it to element");
          delete attribute;
          continue;
        }
        std::string value;
        if (GetProperty<&::IXMLDOMNode::get_text>(attribute_node, &value))
          attribute->SetValue(value.c_str());
        if (!element_prefix.empty())
          attribute->SetPrefix(element_prefix.c_str());
      }
      last_attribute_node = attribute_node;
    }
    if (last_attribute_node)
      last_attribute_node->Release();
    named_node_map->Release();
  }
  ConvertChildrenIntoDOM(domdoc, element, xml_element);
}

const char* SkipSpaces(const char* str) {
  while (*str && isspace(*str))
    str++;
  return str;
}

const int kMaxDetectionDepth = 2048;
const char kMetaTag[] = "meta";
const char kHttpEquivAttrName[] = "http-equiv";
const char kHttpContentType[] = "content-type";
const char kContentAttrName[] = "content";
const char kCharsetPrefix[] = "charset=";

std::string GetHTMLCharset(const char* html_content) {
  std::string charset;
  const char* cursor = html_content;
  while (cursor - html_content < kMaxDetectionDepth) {
    cursor = strchr(cursor, '<');
    if (!cursor)
      break;

    if (strncmp(cursor, "<!--", 3) == 0) {
      cursor = strstr(cursor, "-->");
      if (!cursor)
        break;
      continue;
    }

    cursor = SkipSpaces(cursor + 1);
    if (!strncasecmp(cursor, kMetaTag, arraysize(kMetaTag) - 1)) {
      const char* element_end = strchr(cursor, '>');
      if (!element_end)
        break;

      std::string meta_content(cursor, element_end - cursor);
      meta_content = ToLower(meta_content);
      if (meta_content.find(kHttpEquivAttrName) != meta_content.npos &&
          meta_content.find(kHttpContentType) != meta_content.npos &&
          meta_content.find(kContentAttrName) != meta_content.npos) {
        size_t charset_pos = meta_content.find(kCharsetPrefix);
        if (charset_pos != meta_content.npos) {
          const char* charset_start = meta_content.c_str() + charset_pos +
                                      arraysize(kCharsetPrefix) - 1;
          charset_start = SkipSpaces(charset_start);
          const char* charset_end = charset_start;
          while (isalnum(*charset_end) || *charset_end == '_' ||
                 *charset_end == '.' || *charset_end == '-') {
            charset_end++;
          }
          charset.assign(charset_start, charset_end - charset_start);
        }
        // Don't try to find another, because there should be only one
        // <meta http-equiv="content-type" ...>.
        break;
      }
    }
  }
  return charset;
}

void ConvertElementIntoXPathMap(::IXMLDOMNode* xml_element,
                                const std::string& prefix,
                                StringMap* table) {
  ::IXMLDOMNamedNodeMap* named_node_map = NULL;
  if (SUCCEEDED(xml_element->get_attributes(&named_node_map)) &&
      named_node_map != NULL) {
    ::IXMLDOMNode* attribute_node = NULL;
    ::IXMLDOMNode* last_attribute_node = NULL;
    while (SUCCEEDED(named_node_map->nextNode(&attribute_node)) &&
           attribute_node != NULL) {
      if (last_attribute_node) last_attribute_node->Release();
      std::string attribute_name;
      if (GetProperty<&::IXMLDOMNode::get_nodeName>(attribute_node,
                                                    &attribute_name)) {
        std::string attribute_value;
        GetProperty<&::IXMLDOMNode::get_text>(attribute_node, &attribute_value);
        (*table)[prefix + '@' + attribute_name] = attribute_value;
      }
      last_attribute_node = attribute_node;
    }
    if (last_attribute_node)
      last_attribute_node->Release();
    named_node_map->Release();
  }

  std::map<std::string, int> child_node_name_count;
  ::IXMLDOMNode* child_element = NULL;
  HRESULT hr = xml_element->get_firstChild(&child_element);
  while (SUCCEEDED(hr) && child_element) {
    ::DOMNodeType node_type;
    if (SUCCEEDED(child_element->get_nodeType(&node_type)) &&
        node_type == NODE_ELEMENT) {
      std::string node_name;
      GetProperty<&::IXMLDOMNode::get_nodeName>(child_element, &node_name);
      std::string node_content;
      GetProperty<&::IXMLDOMNode::get_text>(child_element, &node_content);

      std::string key(prefix);
      if (!prefix.empty()) key += '/';
      key += node_name;

      if (table->find(key) != table->end()) {
        // Postpend the sequence if there are multiple elements with the same
        // name.
        int count = ++child_node_name_count[node_name];
        char buf[20];
        snprintf(buf, sizeof(buf), "[%d]", count);
        key += buf;
      } else {
        child_node_name_count[node_name] = 1;
      }
      (*table)[key] = node_content;
      ConvertElementIntoXPathMap(child_element, key, table);
    }
    ::IXMLDOMNode* sibling_element = NULL;
    hr = child_element->get_nextSibling(&sibling_element);
    child_element->Release();
    child_element = sibling_element;
  }
  if (child_element) child_element->Release();
}

// Check if the content is XML according to XMLHttpRequest standard rule.
bool ContentTypeIsXML(const char* content_type) {
  size_t content_type_len = content_type ? strlen(content_type) : 0;
  return content_type_len == 0 ||
         _stricmp(content_type, "text/xml") == 0 ||
         _stricmp(content_type, "application/xml") == 0 ||
         (content_type_len > 4 &&
          _stricmp(content_type + content_type_len - 4, "+xml") == 0);
}

int GetTextBOMLength(const std::string &content) {
  const char* content_ptr = content.c_str();
  int content_length = content.length();
  int offset = 0;
  if (STARTS_WITH(content_ptr, content_length, kUTF8BOM)) {
    offset = sizeof(kUTF8BOM) / sizeof(char);
  } else if (STARTS_WITH(content_ptr, content_length, kUTF16LEBOM)) {
    offset = sizeof(kUTF16LEBOM) / sizeof(char);
  } else if (STARTS_WITH(content_ptr, content_length, kUTF16BEBOM)) {
    offset = sizeof(kUTF16BEBOM) / sizeof(char);
  } else if (STARTS_WITH(content_ptr, content_length, kUTF32LEBOM)) {
    offset = sizeof(kUTF32LEBOM) / sizeof(char);
  } else if (STARTS_WITH(content_ptr, content_length, kUTF32BEBOM)) {
    offset = sizeof(kUTF32BEBOM) / sizeof(char);
  }
  return offset;
}

bool DetectEncoding(const std::string& xml,
                    std::string* encoding,
                    int* bom_length) {
  if (bom_length) *bom_length = 0;
  if (DetectUTFEncoding(xml, encoding)) {
    if (bom_length)
      *bom_length = GetTextBOMLength(xml);
    return true;
  }
  return false;
}

const char kUTF8BOM[] = "\xEF\xBB\xBF";

bool ConvertContentToUnicode(const std::string& content,
                             const char* content_type,
                             const char* encoding_hint,
                             const char* encoding_fallback,
                             std::string* encoding,
                             std::wstring* utf16_content,
                             std::string* utf8_content) {
  if (encoding == NULL && utf16_content == NULL && utf8_content == NULL)
    return true;
  if (utf8_content != NULL) utf8_content->clear();
  if (utf16_content != NULL) utf16_content->clear();
  if (encoding != NULL) encoding->clear();

  bool result = true;
  std::string encoding_to_use;
  const char* content_ptr = content.c_str();
  int content_length = content.size();
  bool prefix = false;
  if (!DetectUTFEncoding(content, &encoding_to_use)) {
    if (encoding_hint && *encoding_hint) {
      encoding_to_use = encoding_hint;
    } else if (STARTS_WITH(content.c_str(), content.size(),
                           kXMLTagBOMLessUTF16LE)) {
      encoding_to_use = "UTF-16LE";
    } else if (STARTS_WITH(content.c_str(), content.size(),
                           kXMLTagBOMLessUTF16BE)) {
      encoding_to_use = "UTF-16BE";
    } else {
      // Try to find encoding declaration from the content.
      if (ContentTypeIsXML(content_type) ||
          STARTS_WITH(content.c_str(), content.size(), kXMLTag)) {
        encoding_to_use = GetXMLEncodingDecl(content);
      } else if (content_type && _stricmp(content_type, "text/html") == 0) {
        encoding_to_use = GetHTMLCharset(content.c_str());
      }

      if (encoding_to_use.empty()) {
        encoding_to_use = "UTF-8";
      } else if (ToLower(encoding_to_use).find("utf") == 0 &&
                 (encoding_to_use.find("16") != std::string::npos ||
                  encoding_to_use.find("32") != std::string::npos)) {
        // UTF-16 and UTF-32 makes no sense here. Because if the content is
        // in UTF-16 or UTF-32 encoding, then it's impossible to find the
        // charset tag by parsing it as char string directly.
        // In this case, assuming UTF-8 will be the best choice, and will
        // fallback to ISO8859-1 if it's not UTF-8.
        encoding_to_use = "UTF-8";
      }
    }
  } else {
    int offset = GetTextBOMLength(content);
    if (offset != 0 ) {
      prefix = true;
      content_ptr += offset;  // Skip BOM.
      content_length -= offset;
    }
  }
  std::wstring utf16_string;
  result = ConvertStringToUTF8AndUTF16(content_ptr, content_length,
                                       encoding_to_use.c_str(),
                                       &utf16_string, utf8_content);
  if (!result && encoding_fallback && *encoding_fallback) {
    encoding_to_use = encoding_fallback;
    result = ConvertStringToUTF8AndUTF16(content_ptr, content_length,
                                         encoding_fallback,
                                         &utf16_string, utf8_content);
  }
  if (result) {
    if (utf8_content != NULL && prefix)
      *utf8_content = kUTF8BOM + *utf8_content; // Add utf8 BOM.
    if (utf16_content != NULL)
      utf16_content->swap(utf16_string);
    if (encoding != NULL)
      *encoding = encoding_to_use;
  }
  return result;
}

bool TryToParseXML(const wchar_t* xml_content,
                   ::IXMLDOMDocument2** result) {
  ::IXMLDOMDocument2* xml_document;
  HRESULT hr;
  hr = ::CoCreateInstance(__uuidof(::DOMDocument),
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          __uuidof(::IXMLDOMDocument2),
                          (void**)&xml_document);
  if (FAILED(hr)) {
    LOG("Failed to create IXMLDOMDocument");
    return false;
  }

  if (SUCCEEDED(xml_document->put_preserveWhiteSpace(VARIANT_TRUE)) &&
      SUCCEEDED(xml_document->put_validateOnParse(VARIANT_FALSE)) &&
      SUCCEEDED(xml_document->put_resolveExternals(VARIANT_FALSE))) {
    ::BSTR prohibitDTD = SysAllocString(kProhibitDTD);
    ::VARIANT variant;
    variant.vt = VT_BOOL;
    variant.boolVal = VARIANT_TRUE;
    if (FAILED(xml_document->setProperty(prohibitDTD, variant))) {
      LOG( "Can't set ProhibitDTD");
      ::SysFreeString(prohibitDTD);
      xml_document->Release();
      return false;
    }
    ::SysFreeString(prohibitDTD);

    ::BSTR xml_content_bstr = SysAllocString(xml_content);
    ::VARIANT_BOOL status;
    hr = xml_document->loadXML(xml_content_bstr, &status);
    ::SysFreeString(xml_content_bstr);

    if (status != VARIANT_TRUE) {
      ::IXMLDOMParseError* obje_error = NULL;
      if (SUCCEEDED(xml_document->get_parseError(&obje_error))) {
        ::BSTR bstr = NULL;
        if (SUCCEEDED(obje_error->get_reason(&bstr))) {
          std::string reason;
          ggadget::ConvertStringUTF16ToUTF8(std::wstring(bstr), &reason);
          LOG("Cannot load DOM from xml: %s", reason.c_str());
          ::SysFreeString(bstr);
        }
        obje_error->Release();
      }
      xml_document->Release();
      return false;
    }
  } else {
    LOG("Cannot to set IXMLDOMDocument properties");
    xml_document->Release();
    return false;
  }

  if (result)
    *result = xml_document;
  else
    xml_document->Release();
  return true;
}

bool ParseXML(const std::string& xml,
              const StringMap* extra_entities,
              const char* filename,
              const char* encoding_hint,
              const char* encoding_fallback,
              ::IXMLDOMDocument2** xml_document,
              std::string* encoding,
              std::string* utf8_content) {
  using xml_parser_internal::PreprocessXMLStringEntity;
  if (xml_document == NULL)
    return false;
  if (encoding)
    encoding->clear();
  std::wstring converted_xml;
  std::wstring processed_xml;
  std::string use_encoding;
  int bom_length = 0;

  if (!DetectEncoding(xml, &use_encoding, &bom_length) &&
      encoding_hint && *encoding_hint) {
    use_encoding = encoding_hint;
  }

  if (!use_encoding.empty()) {
    const char* xml_ptr = xml.c_str() + bom_length;
    int xml_length = xml.length() - bom_length;
    if (ConvertStringToUTF8AndUTF16(xml_ptr, xml_length, use_encoding.c_str(),
                                    &converted_xml, utf8_content) &&
        PreprocessXMLStringEntity(extra_entities, converted_xml.c_str(),
                                  converted_xml.size(), &processed_xml) &&
        TryToParseXML(processed_xml.c_str(), xml_document)) {
      if (utf8_content != NULL && bom_length != 0) {
        // If original content has BOM, outputed utf8 string will also have it.
        *utf8_content = kUTF8BOM + *utf8_content;
      }
      if (encoding)
        *encoding = use_encoding;
      return true;
    }
  } else {
    use_encoding = GetXMLEncodingDecl(xml);
    if (use_encoding.empty()) use_encoding = "UTF-8";
    if (ConvertStringToUTF8AndUTF16(xml.c_str(), xml.length(),
                                    use_encoding.c_str(),
                                    &converted_xml, utf8_content) &&
        PreprocessXMLStringEntity(extra_entities, converted_xml.c_str(),
                                  converted_xml.size(), &processed_xml) &&
        TryToParseXML(processed_xml.c_str(), xml_document)) {
      if (encoding)
        *encoding = use_encoding;
      return true;
    }
  }

  if (encoding_fallback && use_encoding != encoding_fallback) {
    if (ConvertStringToUTF8AndUTF16(xml.c_str(), xml.length(),
                                    encoding_fallback,
                                    &converted_xml, utf8_content) &&
        PreprocessXMLStringEntity(extra_entities, converted_xml.c_str(),
                                  converted_xml.size(), &processed_xml) &&
        TryToParseXML(processed_xml.c_str(), xml_document)) {
      if (encoding)
        *encoding = encoding_fallback;
      return true;
    }
  }
  return false;
}

::IXMLDOMNode* GetRootElement(::IXMLDOMDocument2* xml_document) {
  ::IXMLDOMElement* root = NULL;
  if (xml_document && SUCCEEDED(xml_document->get_documentElement(&root)))
    return root;
  return NULL;
}

}  // namespace

// XMLParser uses DOMDocument which is not free threaded. So by default, it
// initializes Com Library with COINIT_APARTMENTTHREADED to allow it be used
// in multiple threads. But because ggadget makes xml parser be thread-local
// singleton, even if Com Library was initialized with COINIT_MULTITHREADED
// before, this xml parser can also work safely in ggadget.
XMLParser::XMLParser()
    : coinitialize_result_(::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED)) {
  if (FAILED(coinitialize_result_) &&
      // Consider as success if initialized with multithread mode before.
      coinitialize_result_ != RPC_E_CHANGED_MODE) {
    is_valid_ = false;
    LOG("Failed to initialize COM library, %d", coinitialize_result_);
  } else {
    is_valid_ = true;
  }
}

XMLParser::~XMLParser() {
  if (SUCCEEDED(coinitialize_result_))
    ::CoUninitialize();
}

bool XMLParser::CheckXMLName(const char* name) {
  return xml_parser_internal::ValidateXMLName(name);
}

bool XMLParser::HasXMLDecl(const std::string &content) {
  const char* content_ptr = content.c_str();
  size_t content_size = content.size();
  return STARTS_WITH(content_ptr, content_size, kXMLTag) ||
         STARTS_WITH(content_ptr, content_size, kXMLTagUTF8) ||
         STARTS_WITH(content_ptr, content_size, kXMLTagUTF16LE) ||
         STARTS_WITH(content_ptr, content_size, kXMLTagUTF16BE) ||
         STARTS_WITH(content_ptr, content_size, kXMLTagBOMLessUTF16LE) ||
         STARTS_WITH(content_ptr, content_size, kXMLTagBOMLessUTF16BE) ||
         STARTS_WITH(content_ptr, content_size, kXMLTagUTF32LE) ||
         STARTS_WITH(content_ptr, content_size, kXMLTagUTF32BE);
}

DOMDocumentInterface* XMLParser::CreateDOMDocument() {
  return ::ggadget::CreateDOMDocument(this, false, false);
}

bool XMLParser::ConvertContentToUTF8(const std::string& content,
                                     const char* filename,
                                     const char* content_type,
                                     const char* encoding_hint,
                                     const char* encoding_fallback,
                                     std::string* encoding,
                                     std::string* utf8_content) {
  GGL_UNUSED(filename);
  return ConvertContentToUnicode(content, content_type, encoding_hint,
                                 encoding_fallback, encoding,
                                 NULL, utf8_content);
}

bool XMLParser::ParseContentIntoDOM(const std::string& content,
                                    const StringMap* extra_entities,
                                    const char* filename,
                                    const char* content_type,
                                    const char* encoding_hint,
                                    const char* encoding_fallback,
                                    DOMDocumentInterface* domdoc,
                                    std::string* encoding,
                                    std::string* utf8_content) {
  if (!is_valid_) {
    DLOG("XML parser is not initialized successfully.");
    return false;
  }
  bool result = true;
  if (ContentTypeIsXML(content_type) ||
      // However, some XML documents is returned when Content-Type is
      // text/html or others, so detect from the contents.
      HasXMLDecl(content)) {
    ASSERT(!domdoc || !domdoc->HasChildNodes());
    ::IXMLDOMDocument2* xml_document = NULL;
    if (!ParseXML(content, extra_entities, filename,
                  encoding_hint, encoding_fallback,
                  &xml_document, encoding, utf8_content)) {
      result = false;
    } else {
      ::IXMLDOMNode* root = GetRootElement(xml_document);
      if (root != NULL) {
        ConvertChildrenIntoDOM(domdoc, domdoc, xml_document);
        domdoc->Normalize();
        root->Release();
      } else {
        LOG("No root element in XML file: %s", filename);
        result = false;
      }
    }
    if (xml_document)
      xml_document->Release();
  } else {
    result = ConvertContentToUTF8(content, filename, content_type,
                                  encoding_hint, encoding_fallback,
                                  encoding, utf8_content);
  }
  return result;
}

bool XMLParser::ParseXMLIntoXPathMap(const std::string& xml,
                                     const StringMap* extra_entities,
                                     const char* filename,
                                     const char* root_element_name,
                                     const char* encoding_hint,
                                     const char* encoding_fallback,
                                     StringMap* table) {
  if (!is_valid_) {
    DLOG("XML parser is not initialized successfully.");
    return false;
  }
  ::IXMLDOMDocument2* xml_document = NULL;
  std::string encoding, utf8_content;
  if (!ParseXML(xml, extra_entities, filename,
                encoding_hint, encoding_fallback,
                &xml_document, &encoding, &utf8_content)) {
    return false;
  }

  ::IXMLDOMNode* root = GetRootElement(xml_document);
  std::string root_name;
  if (!GetProperty<&::IXMLDOMNode::get_nodeName>(root, &root_name) ||
      GadgetStrCmp(root_name.c_str(), root_element_name) != 0) {
    LOG("No valid root element %s in XML file: %s",
        root_element_name, filename);
    if (root != NULL) root->Release();
    xml_document->Release();
    return false;
  }

  ConvertElementIntoXPathMap(root, "", table);
  root->Release();
  xml_document->Release();
  return true;
}

std::string XMLParser::EncodeXMLString(const char* src) {
  return xml_parser_internal::EncodeXMLString(src);
}

}  // namespace win32
}  // namespace ggadget
