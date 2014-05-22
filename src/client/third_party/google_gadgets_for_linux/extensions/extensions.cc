/*
  Copyright 2013 Google Inc.

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

#include "extensions/extensions.h"
#include "ggadget/xml_parser_interface.h"

extern "C" {
  bool libxml2_xml_parser_LTX_Initialize();
  void libxml2_xml_parser_LTX_Finalize();
}

namespace ggadget {
namespace extensions {

bool Initialize() {
  return libxml2_xml_parser_LTX_Initialize();
}

void Finalize() {
  libxml2_xml_parser_LTX_Finalize();
}

} // extensions
} // ggadget