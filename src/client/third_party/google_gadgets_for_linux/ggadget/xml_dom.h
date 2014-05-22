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

#ifndef GGADGET_XML_DOM_H__
#define GGADGET_XML_DOM_H__

#include <ggadget/xml_dom_interface.h>

namespace ggadget {

class XMLParserInterface;

/**
 * @ingroup XMLDOMInterfaces
 *
 * Create a new document. In most cases, you should create
 * DOMDocumentInterface instances with the global XML parser, which is
 * provided by some extension module.
 */
DOMDocumentInterface *CreateDOMDocument(XMLParserInterface *xml_parser,
                                        bool allow_load_http,
                                        bool allow_load_file);

} // namespace ggadget

#endif // GGADGET_XML_DOM_H__
