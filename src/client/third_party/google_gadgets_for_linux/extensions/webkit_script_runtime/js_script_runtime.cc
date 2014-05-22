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

#include "js_script_runtime.h"
#include <map>
#include <utility>
#include <string>
#include <cstring>
#include <ggadget/logger.h>
#include <ggadget/small_object.h>
#include "js_script_context.h"
#include "json.h"

namespace ggadget {
namespace webkit {

class JSScriptRuntime::Impl : public SmallObject<> {
 public:
  typedef std::vector<std::pair<const JSClassDefinition *, JSClassRef> >
      ClassRefVector;

  ClassRefVector classes;
};

JSScriptRuntime::JSScriptRuntime() : impl_(new Impl) {
}

JSScriptRuntime::~JSScriptRuntime() {
  // Release the references to all JSClassRef objects created by this runtime.
  // The objects might not be released immediately, because of references from
  // other objects.
  Impl::ClassRefVector::iterator class_it = impl_->classes.begin();
  for (; class_it != impl_->classes.end(); ++class_it) {
    ASSERT((*class_it).second);
    JSClassRelease((*class_it).second);
  }

  delete impl_;
  impl_ = NULL;
}

ScriptContextInterface *JSScriptRuntime::CreateContext() {
  return new JSScriptContext(this, NULL);
}

JSClassRef JSScriptRuntime::GetClassRef(const JSClassDefinition *definition) {
  ASSERT(definition);

  // If associated JSClassRef object is already created, then just return it.
  Impl::ClassRefVector::iterator it = impl_->classes.begin();
  for (; it != impl_->classes.end(); ++it) {
    if ((*it).first == definition) {
      ASSERT((*it).second);
      return (*it).second;
    }
  }

  JSClassRef class_ref = JSClassCreate(definition);
  ASSERT(class_ref);
  impl_->classes.push_back(std::make_pair(definition, class_ref));

  return class_ref;
}

JSScriptContext *JSScriptRuntime::WrapExistingContext(JSContextRef js_context) {
  ASSERT(js_context);
  return new JSScriptContext(this, js_context);
}

} // namespace webkit
} // namespace ggadget
