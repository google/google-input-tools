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
#include <clocale>
#include <unittest/gtest.h>
#include "../js_script_context.h"
#include "../js_script_runtime.h"

static JSClassDefinition kClassDefinition1 = kJSClassDefinitionEmpty;
static JSClassDefinition kClassDefinition2 = kJSClassDefinitionEmpty;

using namespace ggadget;
using namespace ggadget::webkit;

TEST(JSScriptRuntime, JSScriptRuntime) {
  JSScriptRuntime *runtime = new JSScriptRuntime();
  ASSERT_TRUE(runtime != NULL);
  ScriptContextInterface *context = runtime->CreateContext();
  ASSERT_TRUE(context != NULL);

  JSClassRef classref1 = runtime->GetClassRef(&kClassDefinition1);
  ASSERT_TRUE(classref1 != NULL);
  JSClassRef classref2 = runtime->GetClassRef(&kClassDefinition2);
  ASSERT_TRUE(classref2 != NULL);
  ASSERT_NE(classref1, classref2);
  ASSERT_EQ(classref1, runtime->GetClassRef(&kClassDefinition1));
  ASSERT_EQ(classref2, runtime->GetClassRef(&kClassDefinition2));
  context->Destroy();
  delete runtime;
}

int main(int argc, char **argv) {
  setlocale(LC_ALL, "");
  testing::ParseGTestFlags(&argc, argv);
  return RUN_ALL_TESTS();
}
