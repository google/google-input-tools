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

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include "ggadget/logger.h"
#include "unittest/gtest.h"
#include "ggadget/gtk/hotkey.cc"

using namespace ggadget;
using namespace ggadget::gtk;

TEST(HotKeyTest, KeyEvent) {
  KeyEvent key;

  ASSERT_FALSE(key.IsValid());
  key = KeyEvent("Ctrl");
  ASSERT_EQ(0u, key.GetKeyValue());
  ASSERT_EQ(static_cast<unsigned int>(KEY_ControlMask), key.GetKeyMask());
  ASSERT_STREQ("Ctrl", key.GetKeyString().c_str());
  ASSERT_FALSE(key.IsNormalKey());
  key = KeyEvent("Ctrl-t");
  ASSERT_EQ(static_cast<unsigned int>(GDK_t), key.GetKeyValue());
  ASSERT_EQ(static_cast<unsigned int>(KEY_ControlMask), key.GetKeyMask());
  ASSERT_STREQ("Ctrl-t", key.GetKeyString().c_str());
  ASSERT_TRUE(key.IsNormalKey());
  key = KeyEvent("t-Ctrl");
  ASSERT_EQ(static_cast<unsigned int>(GDK_t), key.GetKeyValue());
  ASSERT_EQ(static_cast<unsigned int>(KEY_ControlMask), key.GetKeyMask());
  ASSERT_STREQ("Ctrl-t", key.GetKeyString().c_str());
  ASSERT_TRUE(key.IsNormalKey());
  key = KeyEvent("Alt-Ctrl-t");
  ASSERT_EQ(static_cast<unsigned int>(GDK_t), key.GetKeyValue());
  ASSERT_EQ(static_cast<unsigned int>(KEY_ControlMask|KEY_AltMask),
            key.GetKeyMask());
  ASSERT_STREQ("Ctrl-Alt-t", key.GetKeyString().c_str());
  ASSERT_TRUE(key.IsNormalKey());
  key.Reset();
  key.AppendKeyEvent(KeyEvent(GDK_Control_L, 0), true);
  ASSERT_EQ(static_cast<unsigned int>(GDK_Control_L), key.GetKeyValue());
  ASSERT_EQ(0u, key.GetKeyMask());
  key.AppendKeyEvent(KeyEvent(GDK_t, KEY_ControlMask), false);
  ASSERT_EQ(static_cast<unsigned int>(GDK_t), key.GetKeyValue());
  ASSERT_EQ(static_cast<unsigned int>(KEY_ControlMask), key.GetKeyMask());
}

TEST(HotKeyTest, KeyEventRecorder) {
  KeyEventRecorder rec;
  KeyEvent key;
  ASSERT_FALSE(rec.PushKeyEvent(KeyEvent(GDK_Control_L, 0), true, &key));
  ASSERT_FALSE(rec.PushKeyEvent(KeyEvent(GDK_Alt_L, KEY_ControlMask),
                                true, &key));
  ASSERT_FALSE(rec.PushKeyEvent(KeyEvent(GDK_t, KEY_ControlMask|KEY_AltMask),
                                true, &key));
  ASSERT_FALSE(rec.PushKeyEvent(KeyEvent(GDK_t, KEY_ControlMask|KEY_AltMask),
                                false, &key));
  ASSERT_FALSE(rec.PushKeyEvent(KeyEvent(GDK_Control_L,
                                         KEY_ControlMask|KEY_AltMask),
                                false, &key));
  ASSERT_TRUE(rec.PushKeyEvent(KeyEvent(GDK_Alt_L, KEY_AltMask), false, &key));
  ASSERT_STREQ("Ctrl-Alt-t", key.GetKeyString().c_str());
}

int main(int argc, char **argv) {
  testing::ParseGTestFlags(&argc, argv);
  gtk_init(&argc, &argv);
  return RUN_ALL_TESTS();
}
