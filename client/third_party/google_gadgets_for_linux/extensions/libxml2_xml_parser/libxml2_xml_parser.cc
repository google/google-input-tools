/*
  Copyright 2008 Google Inc.

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

#include <cstring>
#include <cmath>
#include <libxml/encoding.h>
#include <libxml/parser.h>
// For xmlCreateMemoryParserCtxt and xmlParseName.
#include <libxml/parserInternals.h>
#include <libxml/tree.h>

#include <ggadget/logger.h>
#include <ggadget/string_utils.h>
#include <ggadget/xml_parser_interface.h>
#include <ggadget/xml_dom.h>

namespace ggadget {
namespace libxml2 {

// Entity will be ignored if size bigger than this limit.
static const size_t kMaxEntitySize = 65536U;

static inline char *FromXmlCharPtr(xmlChar *xml_char_ptr) {
  return reinterpret_cast<char *>(xml_char_ptr);
}

static inline const char *FromXmlCharPtr(const xmlChar *xml_char_ptr) {
  return reinterpret_cast<const char *>(xml_char_ptr);
}

static inline const xmlChar *ToXmlCharPtr(const char *char_ptr) {
  return reinterpret_cast<const xmlChar *>(char_ptr);
}
// There is no non-const version of ToXmlChar *because it's not allowed to
// transfer a non-const char * to libxml.

static const char kXMLTag[] = {
  '<', '?', 'x', 'm', 'l', ' '
};
static const char kXMLTagUTF8[] = {
  '\xEF', '\xBB', '\xBF', '<', '?', 'x', 'm', 'l', ' '
};
static const char kXMLTagUTF16LE[] = {
  '\xFF', '\xFE', '<', 0, '?', 0, 'x', 0, 'm', 0, 'l', 0, ' ', 0
};
static const char kXMLTagUTF16BE[] = {
  '\xFE', '\xFF', 0, '<', 0, '?', 0, 'x', 0, 'm', 0, 'l', 0, ' '
};
static const char kXMLTagBOMLessUTF16LE[] = {
  '<', 0, '?', 0, 'x', 0, 'm', 0, 'l', 0, ' ', 0
};
static const char kXMLTagBOMLessUTF16BE[] = {
  0, '<', 0, '?', 0, 'x', 0, 'm', 0, 'l', 0, ' '
};
static const char kXMLTagUTF32LE[] = {
  '\xFF', '\xFE', 0, 0, '<', 0, 0, 0, '?', 0, 0, 0,
  'x', 0, 0, 0, 'm', 0, 0, 0, 'l', 0, 0, 0, ' ', 0, 0, 0
};
static const char kXMLTagUTF32BE[] = {
  0, 0, '\xFE', '\xFF', 0, 0, 0, '<', 0, 0, 0, '?',
  0, 0, 0, 'x', 0, 0, 0, 'm', 0, 0, 0, 'l', 0, 0, 0, ' '
};
// BOM-less UTF32 is seldom used, so we won't check.

#define STARTS_WITH(content_ptr, content_size, pattern) \
    ((content_size) >= sizeof(pattern) && \
     memcmp((content_ptr), (pattern), sizeof(pattern)) == 0)

// Used in ConvertStringToUTF8 to detect errors during conversion,
// and in ParseXML to let the XML error go into our LOG pipe.
// FIXME: Using global error reporter may have side-effect if another module
// linked to our binary also uses libxml2, especially in other threads.
static bool g_error_occurred = false;
static std::string g_error_buffer;
static void ErrorFunc(void *ctx, const char *msg, ...) {
  GGL_UNUSED(ctx);
  va_list ap;
  va_start(ap, msg);
  StringAppendVPrintf(&g_error_buffer, msg, ap);
  va_end(ap);
  g_error_occurred = true;
  if (g_error_buffer.size() &&
      g_error_buffer[g_error_buffer.size() - 1] == '\n') {
    // Only send to our log when a line break is received.
    g_error_buffer.erase(g_error_buffer.size() - 1);
    LOG("%s", g_error_buffer.c_str());
    g_error_buffer.clear();
  }
}

// Converts a string in given encoding to a utf8.
// Here use libxml routines instead of normal iconv routines to simplify
// compile-time dependencies.
static bool ConvertStringToUTF8(const std::string &content,
                                const char *encoding,
                                std::string *utf8_content) {
  ASSERT(encoding);
  if (utf8_content)
    utf8_content->clear();
  if (content.empty())
    return true;

  xmlCharEncodingHandler *handler;
  if (strcasecmp(encoding, "GB2312") == 0) {
    // Many XML documents declared GB2312 actually contain characters out
    // of GB2312 range. Use GB18030 or GBK to prevent decoding errors.
    handler = xmlFindCharEncodingHandler("GB18030");
    if (!handler) {
      // In case that GB18030 is not supported.
      handler = xmlFindCharEncodingHandler("GBK");
    }
  } else {
    handler = xmlFindCharEncodingHandler(encoding);
  }

  if (!handler)
    return false;

  // xmlCharEncInFunc's result > 0 even if encoding error occurred, so use
  // ErrorFunc to detect errors.
  xmlGenericErrorFunc old_error_func = xmlGenericError;
  xmlSetGenericErrorFunc(NULL, ErrorFunc);
  xmlBuffer *input_buffer = xmlBufferCreateStatic(
      const_cast<char *>(content.c_str()), content.length());
  int error_count = 0;
  const int max_errors = std::max(2, static_cast<int>(content.length() / 100));
  while (error_count <= max_errors && xmlBufferLength(input_buffer) > 0) {
    xmlBuffer *output_buffer = xmlBufferCreate();
    g_error_occurred = false;
    int converted = xmlCharEncInFunc(handler, output_buffer, input_buffer);
    if (converted > 0) {
      ASSERT(converted == xmlBufferLength(output_buffer));
      const char *output = FromXmlCharPtr(xmlBufferContent(output_buffer));
      if (IsLegalUTF8String(output, converted)) {
        if (utf8_content)
          utf8_content->append(output, converted);
        if (g_error_occurred) {
          error_count++;
          if (utf8_content)
            utf8_content->append("?");
          xmlBufferShrink(input_buffer, 1);
        }
      } else {
        error_count = max_errors + 1;
      }
    } else {
      error_count++;
      if (utf8_content)
        utf8_content->append("?");
      xmlBufferShrink(input_buffer, 1);
    }
    xmlBufferFree(output_buffer);
  }

  xmlBufferFree(input_buffer);
  xmlSetGenericErrorFunc(NULL, old_error_func);
  xmlCharEncCloseFunc(handler);
  return error_count <= max_errors;
}

static std::string GetXMLEncodingDecl(const std::string &xml) {
  std::string result;
  if (!STARTS_WITH(xml.c_str(), xml.size(), kXMLTag) &&
      !STARTS_WITH(xml.c_str(), xml.size(), kXMLTagUTF8))
    return result;
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

static void ReplaceXMLEncodingDecl(std::string *xml) {
  if (!STARTS_WITH(xml->c_str(), xml->size(), kXMLTag) &&
      !STARTS_WITH(xml->c_str(), xml->size(), kXMLTagUTF8))
    return;

  size_t end_decl_pos = xml->find("?>");
  if (end_decl_pos == std::string::npos)
    return;
  size_t encoding_pos = xml->rfind(" encoding=\"", end_decl_pos);
  if (encoding_pos == std::string::npos)
    return;
  size_t end_encoding_pos = xml->find('"', encoding_pos + 11);
  if (end_encoding_pos == std::string::npos)
    return;
  xml->replace(encoding_pos, end_encoding_pos - encoding_pos + 1,
               " encoding=\"UTF-8\"");
}

struct ContextData {
  const StringMap *extra_entities;
  getEntitySAXFunc original_get_entity_handler;
  entityDeclSAXFunc original_entity_decl_handler;
};

static void EntityDeclHandler(void *ctx, const xmlChar *name, int type,
                              const xmlChar *public_id,
                              const xmlChar *system_id,
                              xmlChar *content) {
  if (type == 1 && system_id == NULL) {
    // Only handle internal entities.
    xmlParserCtxt *ctxt = static_cast<xmlParserCtxt *>(ctx);
    ASSERT(ctxt && ctxt->_private);
    ContextData *data = static_cast<ContextData *>(ctxt->_private);
    data->original_entity_decl_handler(ctx, name, type, public_id,
                                       system_id, content);
  } else {
    DLOG("External or bad entity decl ignored: %d %s %s %s %s",
         type, name, public_id, system_id, content);
  }
}

// Expand the entity and check the length.
static void ExpandEntity(xmlEntity *entity) {
  if (entity->children && (entity->children->next ||
                           entity->children->type != XML_TEXT_NODE)) {
    xmlNode *text = xmlNewText(ToXmlCharPtr(""));
    size_t size = 0;
    for (xmlNode *child = entity->children; child; child = child->next) {
      xmlChar *child_content = xmlNodeGetContent(child);
      size_t child_size = strlen(FromXmlCharPtr(child_content));
      size += child_size;
      if (size > kMaxEntitySize) {
        LOG("Entity '%s' is too long, truncated", entity->name);
        xmlFree(child_content);
        break;
      }
      xmlNodeAddContentLen(text, child_content, static_cast<int>(child_size));
      xmlFree(child_content);
    }
    xmlFreeNodeList(entity->children);
    entity->children = NULL;
    xmlAddChild(reinterpret_cast<xmlNode *>(entity), text);
    entity->length = static_cast<int>(size);
  }
}

static xmlEntity *GetEntityHandler(void *ctx, const xmlChar *name) {
  xmlParserCtxt *ctxt = static_cast<xmlParserCtxt *>(ctx);
  ASSERT(ctxt && ctxt->_private);
  ContextData *data = static_cast<ContextData *>(ctxt->_private);
  xmlEntity *result = data->original_get_entity_handler(ctx, name);
  if (result) {
    ExpandEntity(result);
  } else if (ctxt->myDoc) {
    if (!ctxt->myDoc->intSubset) {
      ctxt->myDoc->intSubset =
          xmlCreateIntSubset(ctxt->myDoc, NULL, NULL, NULL);
    }
    StringMap::const_iterator it =
        data->extra_entities->find(FromXmlCharPtr(name));
    if (it != data->extra_entities->end()) {
      xmlChar *encoded_value =
          xmlEncodeSpecialChars(NULL, ToXmlCharPtr(it->second.c_str()));
      result = xmlAddDocEntity(ctxt->myDoc, name, XML_INTERNAL_GENERAL_ENTITY,
                               NULL, NULL, encoded_value);
      xmlFree(encoded_value);
    } else {
      LOG("Entity '%s' not defined.", name);
      // If the entity is not defined, just use it's name.
      result = xmlAddDocEntity(ctxt->myDoc, name, XML_INTERNAL_GENERAL_ENTITY,
                               NULL, NULL, name);
    }
  }
  return result;
}

static bool IsBlankText(const char *text) {
  for (const char *p = text; *p; p++) {
    if (strchr(" \r\n\t", *p) == NULL)
      return false;
  }
  return true;
}

static bool IsTextNode(xmlNode *xmlnode) {
  return xmlnode && (xmlnode->type == XML_TEXT_NODE ||
                     xmlnode->type == XML_ENTITY_REF_NODE);
}

static void ConvertCharacterDataIntoDOM(DOMDocumentInterface *domdoc,
                                        DOMNodeInterface *parent,
                                        xmlNode *xmlnode) {
  char *text = FromXmlCharPtr(xmlNodeGetContent(xmlnode));
  UTF16String utf16_text;
  if (text) {
    if (domdoc->PreservesWhiteSpace() ||
        xmlnode->type != XML_TEXT_NODE ||
        IsTextNode(xmlnode->prev) || IsTextNode(xmlnode->next) ||
        !IsBlankText(text)) {
      // Don't trim the text. The caller can trim based on their own
      // requirements.
      ConvertStringUTF8ToUTF16(text, strlen(text), &utf16_text);
    }
    xmlFree(text);
  }

  DOMCharacterDataInterface *data = NULL;
  switch (xmlnode->type) {
    case XML_TEXT_NODE:
      // Don't create empty text nodes.
      if (!utf16_text.empty())
        data = domdoc->CreateTextNode(utf16_text.c_str());
      break;
    case XML_ENTITY_REF_NODE:
      data = domdoc->CreateTextNode(utf16_text.c_str());
      break;
    case XML_CDATA_SECTION_NODE:
      data = domdoc->CreateCDATASection(utf16_text.c_str());
      break;
    case XML_COMMENT_NODE:
      data = domdoc->CreateComment(utf16_text.c_str());
      break;
    default:
      ASSERT(false);
      break;
  }
  if (data) {
    data->SetRow(static_cast<int>(xmlGetLineNo(xmlnode)));
    parent->AppendChild(data);
  }
}

static void ConvertPIIntoDOM(DOMDocumentInterface *domdoc,
                             DOMNodeInterface *parent,
                             xmlNode *xmlpi) {
  const char *target = FromXmlCharPtr(xmlpi->name);
  char *data = FromXmlCharPtr(xmlNodeGetContent(xmlpi));
  DOMProcessingInstructionInterface *pi;
  domdoc->CreateProcessingInstruction(target, data, &pi);
  if (pi) {
    pi->SetRow(static_cast<int>(xmlGetLineNo(xmlpi)));
    parent->AppendChild(pi);
  }
  if (data)
    xmlFree(data);
}

static void ConvertElementIntoDOM(DOMDocumentInterface *domdoc,
                                  DOMNodeInterface *parent,
                                  xmlNode *xmlele);

static void ConvertChildrenIntoDOM(DOMDocumentInterface *domdoc,
                                   DOMNodeInterface *parent,
                                   xmlNode *xmlnode) {
  for (xmlNode *child = xmlnode->children; child != NULL; child = child->next) {
    switch (child->type) {
      case XML_ELEMENT_NODE:
        ConvertElementIntoDOM(domdoc, parent, child);
        break;
      case XML_TEXT_NODE:
      case XML_ENTITY_REF_NODE:
      case XML_CDATA_SECTION_NODE:
      case XML_COMMENT_NODE:
        ConvertCharacterDataIntoDOM(domdoc, parent, child);
        break;
      case XML_PI_NODE:
        ConvertPIIntoDOM(domdoc, parent, child);
        break;
      case XML_DTD_NODE:
        break;
      default:
        DLOG("Ignore XML Node of type %d", child->type);
        break;
    }
  }
}

static void ConvertElementIntoDOM(DOMDocumentInterface *domdoc,
                                  DOMNodeInterface *parent,
                                  xmlNode *xmlele) {
  DOMElementInterface *element;
  domdoc->CreateElement(FromXmlCharPtr(xmlele->name), &element);
  if (!element || DOM_NO_ERR != parent->AppendChild(element)) {
    // Unlikely to happen.
    DLOG("Failed to create DOM element or to add it to parent");
    delete element;
    return;
  }

  // We don't support full DOM2 namespaces, but we must keep all namespace
  // related information in the result DOM.
  if (xmlele->ns && xmlele->ns->prefix)
    element->SetPrefix(FromXmlCharPtr(xmlele->ns->prefix));
  for (xmlNsPtr ns = xmlele->nsDef; ns; ns = ns->next) {
    DOMAttrInterface *attr;
    if (ns->prefix && *ns->prefix) {
      // xmlns:prefix="uri" case.
      domdoc->CreateAttribute(FromXmlCharPtr(ns->prefix), &attr);
      if (attr)
        attr->SetPrefix("xmlns");
    } else {
      // xmlns="uri" case.
      domdoc->CreateAttribute("xmlns", &attr);
    }
    if (!attr || DOM_NO_ERR != element->SetAttributeNode(attr)) {
      // Unlikely to happen.
      DLOG("Failed to create xmlns attribute or to add it to element");
      delete attr;
      continue;
    }
    attr->SetValue(FromXmlCharPtr(ns->href));
  }

  // libxml2 doesn't support node column position for now.
  element->SetRow(static_cast<int>(xmlGetLineNo(xmlele)));
  for (xmlAttr *xmlattr = xmlele->properties; xmlattr != NULL;
       xmlattr = xmlattr->next) {
    const char *name = FromXmlCharPtr(xmlattr->name);
    DOMAttrInterface *attr;
    domdoc->CreateAttribute(name, &attr);
    if (!attr || DOM_NO_ERR != element->SetAttributeNode(attr)) {
      // Unlikely to happen.
      DLOG("Failed to create DOM attribute or to add it to element");
      delete attr;
      continue;
    }

    char *value = FromXmlCharPtr(
        xmlNodeGetContent(reinterpret_cast<xmlNode *>(xmlattr)));
    attr->SetValue(value);
    if (xmlattr->ns)
      attr->SetPrefix(FromXmlCharPtr(xmlattr->ns->prefix));
    if (value)
      xmlFree(value);
  }

  ConvertChildrenIntoDOM(domdoc, element, xmlele);
}

static const char* SkipSpaces(const char* str) {
  while (*str && isspace(*str))
    str++;
  return str;
}

static const int kMaxDetectionDepth = 2048;
static const char kMetaTag[] = "meta";
static const char kHttpEquivAttrName[] = "http-equiv";
static const char kHttpContentType[] = "content-type";
static const char kContentAttrName[] = "content";
static const char kCharsetPrefix[] = "charset=";

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
                 *charset_end == '.' || *charset_end == '-')
            charset_end++;
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

// Count the sequence of a child in the elements of the same tag name.
static int CountTagSequence(const xmlNode *child, const char *tag) {
  static xmlNode *last_parent = NULL;
  static int last_count = 1;
  static std::string last_tag;

  if (last_parent == child->parent &&
      GadgetStrCmp(last_tag.c_str(), tag) == 0) {
    return ++last_count;
  }

  last_parent = child->parent;
  last_count = 1;
  last_tag = tag;
  for (const xmlNode *node = child->prev; node != NULL; node = node->prev) {
    if (node->type == XML_ELEMENT_NODE &&
        GadgetStrCmp(tag, FromXmlCharPtr(node->name)) == 0)
      last_count++;
  }
  return last_count;
}

static void ConvertElementIntoXPathMap(const xmlNode *element,
                                       const std::string &prefix,
                                       StringMap *table) {
  for (xmlAttr *attribute = element->properties;
       attribute != NULL; attribute = attribute->next) {
    const char *name = FromXmlCharPtr(attribute->name);
    char *value = FromXmlCharPtr(
        xmlNodeGetContent(reinterpret_cast<xmlNode *>(attribute)));
    (*table)[prefix + '@' + name] = std::string(value ? value : "");
    if (value)
      xmlFree(value);
  }

  for (xmlNode *child = element->children; child != NULL; child = child->next) {
    if (child->type == XML_ELEMENT_NODE) {
      const char *tag = FromXmlCharPtr(child->name);
      char *text = FromXmlCharPtr(xmlNodeGetContent(child));
      std::string key(prefix);
      if (!prefix.empty()) key += '/';
      key += tag;

      if (table->find(key) != table->end()) {
        // Postpend the sequence if there are multiple elements with the same
        // name.
        char buf[20];
        snprintf(buf, sizeof(buf), "[%d]", CountTagSequence(child, tag));
        key += buf;
      }
      (*table)[key] = text ? text : "";
      if (text) xmlFree(text);

      ConvertElementIntoXPathMap(child, key, table);
    }
  }
}

// Check if the content is XML according to XMLHttpRequest standard rule.
static bool ContentTypeIsXML(const char *content_type) {
  size_t content_type_len = content_type ? strlen(content_type) : 0;
  return content_type_len == 0 ||
         strcasecmp(content_type, "text/xml") == 0 ||
         strcasecmp(content_type, "application/xml") == 0 ||
         (content_type_len > 4 &&
          strcasecmp(content_type + content_type_len - 4, "+xml") == 0);
}

static bool ConvertToUTF8(const std::string &content, const char *filename,
                          const char *content_type, const char *encoding_hint,
                          const char *encoding_fallback,
                          std::string *encoding, std::string *utf8_content) {
  GGL_UNUSED(filename);
  // The caller wants nothing?
  if (!encoding && !utf8_content)
    return true;

  bool result = true;
  std::string encoding_to_use;
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
      } else if (content_type && strcasecmp(content_type, "text/html") == 0) {
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
  }

  result = ConvertStringToUTF8(content, encoding_to_use.c_str(),
                               utf8_content);
  if (!result && encoding_fallback && *encoding_fallback) {
    encoding_to_use = encoding_fallback;
    result = ConvertStringToUTF8(content, encoding_fallback, utf8_content);
  }
  if (encoding)
    *encoding = result ? encoding_to_use : "";
  return result;
}

static xmlDoc *ParseXML(const std::string &xml,
                        const StringMap *extra_entities,
                        const char *filename,
                        const char *encoding_hint,
                        const char *encoding_fallback,
                        std::string *encoding,
                        std::string *utf8_content) {
  xmlDoc *xmldoc = NULL;

  if (encoding)
    encoding->clear();
  if (utf8_content)
    utf8_content->clear();

  std::string converted_xml;
  std::string use_encoding;

  // Convert encoding before we let libxml2 parse the document, to make it
  // possible to recover from encoding conversion failures.
  if (!ConvertToUTF8(xml, filename, NULL, encoding_hint, encoding_fallback,
                     &use_encoding, &converted_xml)) {
    return false;
  }

  if (utf8_content)
    *utf8_content = converted_xml;
  // We have successfully converted the encoding to UTF8, insert a BOM and
  // remove the original encoding declaration to prevent libxml2 from
  // converting again.
  ReplaceXMLEncodingDecl(&converted_xml);

  xmlParserCtxt *ctxt = xmlCreateMemoryParserCtxt(
      converted_xml.c_str(), static_cast<int>(converted_xml.length()));
  if (!ctxt)
    return NULL;

  ASSERT(ctxt->sax);
  ContextData data;
  ctxt->_private = &data;
  if (extra_entities) {
    // Hook getEntity handler to provide extra entities.
    data.extra_entities = extra_entities;
    data.original_get_entity_handler = ctxt->sax->getEntity;
    ctxt->sax->getEntity = GetEntityHandler;
  }

  // Disable external entities to avoid security troubles.
  data.original_entity_decl_handler = ctxt->sax->entityDecl;
  ctxt->sax->entityDecl = EntityDeclHandler;
  ctxt->sax->resolveEntity = NULL;

  // Let the built-in libxml2 error reporter print the correct filename.
  ctxt->input->filename = xmlMemStrdup(filename);

  xmlGenericErrorFunc old_error_func = xmlGenericError;
  xmlSetGenericErrorFunc(NULL, ErrorFunc);
  xmlParseDocument(ctxt);
  xmlSetGenericErrorFunc(NULL, old_error_func);

  if (ctxt->wellFormed) {
    // Successfully parsed the document.
    xmldoc = ctxt->myDoc;
  } else {
    xmlFreeDoc(ctxt->myDoc);
    ctxt->myDoc = NULL;
  }
  xmlFreeParserCtxt(ctxt);

  if (encoding)
    *encoding = use_encoding;
  return xmldoc;
}

class XMLParser : public XMLParserInterface {
 public:
  virtual bool CheckXMLName(const char *name) {
    return name && *name && xmlValidateName(ToXmlCharPtr(name), 0) == 0;
  }

  virtual bool HasXMLDecl(const std::string &content) {
    const char *content_ptr = content.c_str();
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

  virtual DOMDocumentInterface *CreateDOMDocument() {
    return ::ggadget::CreateDOMDocument(this, false, false);
  }


  virtual bool ConvertContentToUTF8(const std::string &content,
                                    const char *filename,
                                    const char *content_type,
                                    const char *encoding_hint,
                                    const char *encoding_fallback,
                                    std::string *encoding,
                                    std::string *utf8_content) {
    return ConvertToUTF8(content, filename, content_type, encoding_hint,
                         encoding_fallback, encoding, utf8_content);
  }

  virtual bool ParseContentIntoDOM(const std::string &content,
                                   const StringMap *extra_entities,
                                   const char *filename,
                                   const char *content_type,
                                   const char *encoding_hint,
                                   const char *encoding_fallback,
                                   DOMDocumentInterface *domdoc,
                                   std::string *encoding,
                                   std::string *utf8_content) {
#ifdef _DEBUG
    int original_ref_count = domdoc ? domdoc->GetRefCount() : 0;
#endif
    bool result = true;
    xmlLineNumbersDefault(1);
    if (ContentTypeIsXML(content_type) ||
        // However, some XML documents is returned when Content-Type is
        // text/html or others, so detect from the contents.
        HasXMLDecl(content)) {
      ASSERT(!domdoc || !domdoc->HasChildNodes());
      xmlDoc *xmldoc = ParseXML(content, extra_entities, filename,
                                encoding_hint, encoding_fallback,
                                encoding, utf8_content);
      if (!xmldoc) {
        result = false;
      } else {
        if (!xmlDocGetRootElement(xmldoc)) {
          LOG("No root element in XML file: %s", filename);
          result = false;
        } else {
          ConvertChildrenIntoDOM(domdoc, domdoc,
                                 reinterpret_cast<xmlNode *>(xmldoc));
          domdoc->Normalize();
        }
        xmlFreeDoc(xmldoc);
      }
    } else {
      result = ConvertToUTF8(content, filename, content_type, encoding_hint,
                             encoding_fallback, encoding, utf8_content);
    }
#ifdef _DEBUG
    ASSERT(!domdoc || domdoc->GetRefCount() == original_ref_count);
#endif
    return result;
  }

  virtual bool ParseXMLIntoXPathMap(const std::string &xml,
                                    const StringMap *extra_entities,
                                    const char *filename,
                                    const char *root_element_name,
                                    const char *encoding_hint,
                                    const char *encoding_fallback,
                                    StringMap *table) {
    xmlDoc *xmldoc = ParseXML(xml, extra_entities, filename, encoding_hint,
                              encoding_fallback, NULL, NULL);
    if (!xmldoc)
      return false;

    xmlNode *root = xmlDocGetRootElement(xmldoc);
    if (!root ||
        GadgetStrCmp(FromXmlCharPtr(root->name), root_element_name) != 0) {
      LOG("No valid root element %s in XML file: %s",
          root_element_name, filename);
      xmlFreeDoc(xmldoc);
      return false;
    }

    ConvertElementIntoXPathMap(root, "", table);
    xmlFreeDoc(xmldoc);
    return true;
  }

  virtual std::string EncodeXMLString(const char *src) {
    if (!src || !*src)
      return std::string();

    char *result = FromXmlCharPtr(xmlEncodeSpecialChars(NULL,
                                                        ToXmlCharPtr(src)));
    std::string result_str(result ? result : "");
    if (result)
      xmlFree(result);
    return result_str;
  }
};

static XMLParser *g_xml_parser = NULL;

} // namespace libxml2
} // namespace ggadget

#define Initialize libxml2_xml_parser_LTX_Initialize
#define Finalize libxml2_xml_parser_LTX_Finalize

extern "C" {
  bool Initialize() {
    LOGI("Initialize libxml2_xml_parser extension.");

    // Many files declared as GB2312 encoding contain chararacters outside
    // of standard GB2312 range. Tolerate this by using superset GB18030 or GBK.
    xmlCharEncodingHandler *handler = xmlFindCharEncodingHandler("GB18030");
    if (handler) {
      xmlAddEncodingAlias("GB18030", "GB2312");
      xmlCharEncCloseFunc(handler);
    } else {
      DLOG("libxml2 doesn't support GB18030, try GBK");
      handler = xmlFindCharEncodingHandler("GBK");
      if (handler) {
        xmlAddEncodingAlias("GBK", "GB2312");
        xmlCharEncCloseFunc(handler);
      }
    }

    if (!ggadget::libxml2::g_xml_parser)
      ggadget::libxml2::g_xml_parser = new ggadget::libxml2::XMLParser;

    return ggadget::SetXMLParser(ggadget::libxml2::g_xml_parser);
  }

  void Finalize() {
    LOGI("Finalize libxml2_xml_parser extension.");
    delete ggadget::libxml2::g_xml_parser;
  }
}
