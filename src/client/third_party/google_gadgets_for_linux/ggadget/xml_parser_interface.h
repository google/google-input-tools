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

#ifndef GGADGET_XML_PARSER_INTERFACE_H__
#define GGADGET_XML_PARSER_INTERFACE_H__

#include <ggadget/string_utils.h>

namespace ggadget {

class DOMDocumentInterface;

/**
 * @defgroup XMLParserInterface XML Parser Interface
 * @ingroup Interfaces
 * @{
 */

/**
 * Interface class for real XML parser implementation.
 */
class XMLParserInterface {
 public:
  /**
   * Checks if an XML name is a valid name.
   * @param name the XML name in UTF8 encoding.
   * @return @c true if the name is valid.
   */
  virtual bool CheckXMLName(const char *name) = 0;

  /**
   * Checks if content has XML declaration at the beginning.
   */
  virtual bool HasXMLDecl(const std::string &content) = 0;

  /**
   * Creates a new blank @c DOMDocumentInterface instance.
   */
  virtual DOMDocumentInterface *CreateDOMDocument() = 0;

  /**
   * Convert input content into UTF8, according to the rules defined in the
   * XMLHttpRequestspecification
   * (http://www.w3.org/TR/2007/WD-XMLHttpRequest-20071026/).
   *
   * @param content the content of an XML file.
   * @param filename the name of the XML file (only for logging).
   * @param content_type the MIME content type of the input. Can be @c NULL or
   *     blank if the caller can ensure the content is XML.
   * @param encoding_hint the hint of encoding if the input xml has no
   *     Unicode BOF.  If @c NULL or blank, the parser will detect the
   *     encoding.
   * @param encoding_fallback the last fallback encoding if the hint or
   *     the declared encoding fails.
   * @param[out] encoding contains the encoding actually used. Can be @c NULL
   *     if the caller doesn't need it.
   * @param[out] utf8_content converted content into utf8, if encoding
   *     conversion is successful, otherwise blank.
   *     Can be @c NULL if the caller doesn't need it.
   * @return @c false if encoding conversion failed, or the content is XML
   *     and XML parsing failed. Note, even when @c false is returned,
   *     @c utf8_content is still available if encoding conversion is
   *     successful.
   */
  virtual bool ConvertContentToUTF8(const std::string &content,
                                    const char *filename,
                                    const char *content_type,
                                    const char *encoding_hint,
                                    const char *encoding_fallback,
                                    std::string *encoding,
                                    std::string *utf8_content) = 0;

  /**
   * Parses XML and build the DOM tree if the input is XML, and convert input
   * content into UTF8, according to the rules defined in the XMLHttpRequest
   * specification (http://www.w3.org/TR/2007/WD-XMLHttpRequest-20071026/).
   *
   * @param content the content of an XML file.
   * @param extra_entities extra entites defined in other places that this
   *     XML file may reference.
   * @param filename the name of the XML file (only for logging).
   * @param content_type the MIME content type of the input. Can be @c NULL or
   *     blank if the caller can ensure the content is XML.
   * @param encoding_hint the hint of encoding if the input xml has no
   *     Unicode BOF.  If @c NULL or blank, the parser will detect the
   *     encoding.
   * @param encoding_fallback the last fallback encoding if the hint or
   *     the declared encoding fails.
   * @param domdoc the DOM document. It must be blank before calling this
   *     function, and will be filled with DOM data if this function succeeds.
   * @param[out] encoding contains the encoding actually used. Can be @c NULL
   *     if the caller doesn't need it.
   * @param[out] utf8_content converted content into utf8, if encoding
   *     conversion is successful, otherwise blank.
   *     Can be @c NULL if the caller doesn't need it.
   * @return @c false if encoding conversion failed, or the content is XML
   *     and XML parsing failed. Note, even when @c false is returned,
   *     @c utf8_content is still available if encoding conversion is
   *     successful.
   */
  virtual bool ParseContentIntoDOM(const std::string &content,
                                   const StringMap *extra_entities,
                                   const char *filename,
                                   const char *content_type,
                                   const char *encoding_hint,
                                   const char *encoding_fallback,
                                   DOMDocumentInterface *domdoc,
                                   std::string *encoding,
                                   std::string *utf8_content) = 0;

  /**
   * Parses an XML file and store the result into a string map.
   *
   * The string map acts like a simple DOM that supporting XPath like queries.
   * When a key is given:
   *   - element_name: retreives the text content of the second level element
   *     named 'element_name' (the root element name is omitted);
   *   - element_name/subele_name: retrieves the text content of the third level
   *     element named 'subele_name' under the second level element named
   *     'element_name';
   *   - @@attr_name: retrives the value of attribute named 'attr_name' in the
   *     top level element;
   *   - element_name@@attr_name: retrieves the value of attribute named
   *     'attr_name' in the second level element named 'element_name'.
   *
   * If there are multiple elements with the same name under the same element,
   * the name of the elements from the second one will be postpended with "[n]"
   * where 'n' is the sequence of the element in the elements with the same name
   * (count from 1).
   *
   * @param xml the content of an XML file.
   * @param extra_entities extra entites defined in other places that this
   *     XML file may reference.
   * @param filename the name of the XML file (only for logging).
   * @param root_element_name expected name of the root element.
   * @param encoding_hint hints the parser of the encoding if the input xml
   *     has no Unicode BOF. If @c NULL or blank, the parser will detect the
   *     encoding.
   * @param encoding_fallback the last fallback encoding if the hint or
   *     the declared encoding fails.
   * @param table the string table to fill.
   * @return @c true if succeeds.
   */
  virtual bool ParseXMLIntoXPathMap(const std::string &xml,
                                    const StringMap *extra_entities,
                                    const char *filename,
                                    const char *root_element_name,
                                    const char *encoding_hint,
                                    const char *encoding_fallback,
                                    StringMap *table) = 0;

  /**
   * Encode a string into XML text by escaping special chars.
   */
  virtual std::string EncodeXMLString(const char *src) = 0;

 protected:
  virtual ~XMLParserInterface() { }
};

/**
 * @relates XMLParserInterface
 * Sets the global XMLParserInterface instance.
 * An XMLParser extension module can call this function in its @c Initialize()
 * function.
 */
bool SetXMLParser(XMLParserInterface *xml_parser);

/**
 * @relates XMLParserInterface
 * Gets the XMLParserInterface instance.
 * The returned instance is a singleton provided by an XMLParser
 * extension module, which is loaded into the global ExtensionManager in
 * advanced.
 */
XMLParserInterface *GetXMLParser();

/** @} */

} // namespace ggadget

#endif // GGADGET_XML_PARSER_INTERFACE_H__
