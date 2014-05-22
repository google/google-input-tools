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

#include "build_config.h"
#include "logger.h"

#if defined(OS_WIN)
#include "win32/thread_local_singleton_holder.h"
#endif // OS_WIN

#include "xml_parser_interface.h"

namespace ggadget {

#if !defined(OS_WIN)
static XMLParserInterface *g_xml_parser = NULL;
#endif // OS_WIN

bool SetXMLParser(XMLParserInterface *xml_parser) {
#if defined(OS_WIN)
  XMLParserInterface *old_xml_parser =
      win32::ThreadLocalSingletonHolder<XMLParserInterface>::GetValue();
  if (old_xml_parser && xml_parser)
    return false;
  return win32::ThreadLocalSingletonHolder<XMLParserInterface>::SetValue(
      xml_parser);
#else // OS_WIN
  ASSERT(!g_xml_parser && xml_parser);
  if (!g_xml_parser && xml_parser) {
    g_xml_parser = xml_parser;
    return true;
  }
  return false;
#endif // OS_WIN
}

XMLParserInterface *GetXMLParser() {
#if defined(OS_WIN)
  XMLParserInterface *xml_parser =
      win32::ThreadLocalSingletonHolder<XMLParserInterface>::GetValue();
  EXPECT_M(xml_parser,
           ("The global xml parser has not been set yet."));
  return xml_parser;
#else // OS_WIN
  EXPECT_M(g_xml_parser, ("The global xml parser has not been set yet."));
  return g_xml_parser;
#endif // OS_WIN
}

} // namespace ggadget
