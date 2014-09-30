/*
  Copyright 2014 Google Inc.

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

#include <atlbase.h>
#include <atlcom.h>
#include <atlcomcli.h>
#include <msctf.h>

#include "common/mock_engine.h"
#include "tsf/keyboard.h"
#include <gtest/gunit.h>

namespace ime_goopy {
namespace tsf {
static const wchar_t kTestChar = L'A';
TEST(Keyboard, TestKey) {
  CComObjectStackEx<Keyboard> keyboard;
  MockEngine engine;
  keyboard.Initialize(NULL, &engine);
  BOOL eaten = false;
  EXPECT_TRUE(SUCCEEDED(keyboard.OnTestKeyDown(kTestChar, 0, &eaten)));
  EXPECT_EQ(TRUE, eaten);
  EXPECT_TRUE(keyboard.eat_key_up_[kTestChar]);
  EXPECT_EQ(1, engine.commands().size());
  EXPECT_TRUE(engine.commands()[0]->is<ShouldProcessKeyCommand>());

  engine.Reset();
  EXPECT_TRUE(SUCCEEDED(keyboard.OnTestKeyUp(kTestChar, 0, &eaten)));
  EXPECT_EQ(TRUE, eaten);
  EXPECT_FALSE(keyboard.eat_key_up_[kTestChar]);
  EXPECT_EQ(1, engine.commands().size());
  EXPECT_TRUE(engine.commands()[0]->is<ShouldProcessKeyCommand>());

  keyboard.Uninitialize();
}

TEST(Keyboard, Key) {
  CComObjectStackEx<Keyboard> keyboard;
  MockEngine engine;
  keyboard.Initialize(NULL, &engine);
  BOOL eaten = false;
  EXPECT_TRUE(SUCCEEDED(keyboard.OnKeyDown(kTestChar, 0, &eaten)));
  EXPECT_EQ(TRUE, eaten);
  EXPECT_EQ(2, engine.commands().size());
  EXPECT_TRUE(engine.commands()[0]->is<ShouldProcessKeyCommand>());
  EXPECT_TRUE(engine.commands()[1]->is<ProcessKeyCommand>());
  EXPECT_TRUE(keyboard.eat_key_up_[kTestChar]);

  engine.Reset();
  EXPECT_TRUE(SUCCEEDED(keyboard.OnKeyUp(kTestChar, 0, &eaten)));
  EXPECT_EQ(TRUE, eaten);
  EXPECT_EQ(2, engine.commands().size());
  EXPECT_TRUE(engine.commands()[0]->is<ShouldProcessKeyCommand>());
  EXPECT_TRUE(engine.commands()[1]->is<ProcessKeyCommand>());
  EXPECT_FALSE(keyboard.eat_key_up_[kTestChar]);

  keyboard.Uninitialize();
}
}  // namespace tsf
}  // namespace ime_goopy
