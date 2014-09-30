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

#ifndef EXTENSIONS_GTKMOZ_BROWSER_CHILD_INTERNAL_H__
#define EXTENSIONS_GTKMOZ_BROWSER_CHILD_INTERNAL_H__

#include <jsapi.h>

class nsIScriptContext;
// nsIScriptContext.h requires internal string API, but we don't use it.
// Separate related code to another file to resolve the conflicts.
JSContext *GetJSContext(nsIScriptContext *script_context);

#endif // EXTENSIONS_GTKMOZ_BROWSER_CHILD_INTERNAL_H__
