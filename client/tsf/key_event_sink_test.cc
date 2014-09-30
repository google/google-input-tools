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
#include "tsf/key_event_sink.h"
#include <gtest/gunit.h>

namespace ime_goopy {
namespace tsf {
static const wchar_t kTestChar = L'A';
TEST(KeyEventSink, TestKey) {
  CComObjectStackEx<KeyEventSink> key_event_sink;
  MockEngine engine;
  key_event_sink.Initialize(NULL, &engine);
  BOOL eaten = false;
  EXPECT_TRUE(SUCCEEDED(key_event_sink.OnTestKeyDown(kTestChar, 0, &eaten)));
  EXPECT_EQ(TRUE, eaten);
  EXPECT_TRUE(key_event_sink.eat_key_up_[kTestChar]);
  EXPECT_EQ(1, engine.commands().size());
  EXPECT_TRUE(engine.commands()[0]->is<ShouldProcessKeyCommand>());

  engine.Reset();
  EXPECT_TRUE(SUCCEEDED(key_event_sink.OnTestKeyUp(kTestChar, 0, &eaten)));
  EXPECT_EQ(TRUE, eaten);
  EXPECT_FALSE(key_event_sink.eat_key_up_[kTestChar]);
  EXPECT_EQ(1, engine.commands().size());
  EXPECT_TRUE(engine.commands()[0]->is<ShouldProcessKeyCommand>());

  key_event_sink.Uninitialize();
}

TEST(KeyEventSink, Key) {
  CComObjectStackEx<KeyEventSink> key_event_sink;
  MockEngine engine;
  key_event_sink.Initialize(NULL, &engine);
  BOOL eaten = false;
  EXPECT_TRUE(SUCCEEDED(key_event_sink.OnKeyDown(kTestChar, 0, &eaten)));
  EXPECT_EQ(TRUE, eaten);
  EXPECT_EQ(2, engine.commands().size());
  EXPECT_TRUE(engine.commands()[0]->is<ShouldProcessKeyCommand>());
  EXPECT_TRUE(engine.commands()[1]->is<ProcessKeyCommand>());
  EXPECT_TRUE(key_event_sink.eat_key_up_[kTestChar]);

  engine.Reset();
  EXPECT_TRUE(SUCCEEDED(key_event_sink.OnKeyUp(kTestChar, 0, &eaten)));
  EXPECT_EQ(TRUE, eaten);
  EXPECT_EQ(2, engine.commands().size());
  EXPECT_TRUE(engine.commands()[0]->is<ShouldProcessKeyCommand>());
  EXPECT_TRUE(engine.commands()[1]->is<ProcessKeyCommand>());
  EXPECT_FALSE(key_event_sink.eat_key_up_[kTestChar]);

  key_event_sink.Uninitialize();
}
}  // namespace tsf
}  // namespace ime_goopy
