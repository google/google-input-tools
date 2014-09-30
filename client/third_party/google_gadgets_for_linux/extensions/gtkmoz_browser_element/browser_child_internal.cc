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

#include "browser_child_internal.h"

// MOZILLA_INTERNAL_API is defined here only to make nsIScriptContext.h happy.
// Because in fact it doesn't require any code related to MOZILLA_INTERNAL_API
// things, it's safe to link this file to browser_child.cc and xpcom glue.
#define MOZILLA_INTERNAL_API
#include <nsIScriptContext.h>

JSContext *GetJSContext(nsIScriptContext *script_context) {
  return reinterpret_cast<JSContext *>(script_context->GetNativeContext());
}
