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

#include "ggadget/script_context_interface.h"
#include "ggadget/scriptable_helper.h"
#include "ggadget/xml_dom_interface.h"
#include "ggadget/xml_parser_interface.h"
#include "ggadget/tests/init_extensions.h"
#include "../js_script_context.h"

using namespace ggadget;

class GlobalObject : public ScriptableHelperNativeOwnedDefault {
 public:
  DEFINE_CLASS_ID(0x7067c76cc0d84d22, ScriptableInterface);
  GlobalObject()
      : xml_parser_(GetXMLParser()) {
  }
  ~GlobalObject() {
  }

  virtual bool IsStrict() const { return false; }

  DOMDocumentInterface *CreateDOMDocument() {
    return xml_parser_->CreateDOMDocument();
  }

  XMLParserInterface *xml_parser_;
};

static GlobalObject *global;

// Called by the initialization code in js_shell.cc.
bool InitCustomObjects(ScriptContextInterface *context) {
  static const char *kExtensions[] = {
    "libxml2_xml_parser/libxml2-xml-parser",
  };
  int argc = 0;
  const char **argv = NULL;
  INIT_EXTENSIONS(argc, argv, kExtensions);

  global = new GlobalObject();
  context->SetGlobalObject(global);
  context->RegisterClass("DOMDocument",
                         NewSlot(global, &GlobalObject::CreateDOMDocument));

  return true;
}

void DestroyCustomObjects(ScriptContextInterface *context) {
  delete global;
}
