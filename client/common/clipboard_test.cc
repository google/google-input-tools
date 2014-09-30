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

#include "common/clipboard.h"
#include "common/string_utils.h"
#include <gtest/gunit.h>

using ime_goopy::Utf8ToWide;

TEST(ClipboardTest, TestClipboardText) {
  Clipboard clipboard;
  Clipboard::ObjectMap objects;
  Clipboard::ObjectMapParam param;

  string test_string("HelloWorld");
  for (int i = 0; i < test_string.size(); i++)
    param.push_back(test_string[i]);

  objects[Clipboard::CBF_TEXT].push_back(param);
  clipboard.WriteObjects(objects);

  // Check format
  EXPECT_TRUE(clipboard.IsFormatAvailable(CF_UNICODETEXT));
  EXPECT_FALSE(clipboard.IsFormatAvailable(CF_BITMAP));

  // Read and verify
  wstring utf16_text;
  string utf8_text;
  clipboard.ReadText(&utf16_text);
  clipboard.ReadAsciiText(&utf8_text);
  EXPECT_EQ(Utf8ToWide(test_string), utf16_text);
  EXPECT_EQ(test_string, utf8_text);

  // Test Utf16 string.
  wstring utf16_string(L"\u9686\u9686\u9686\u9686\u9686\u9686\u9686\u9686");
  clipboard.WriteText(utf16_string);
  EXPECT_TRUE(clipboard.IsFormatAvailable(CF_UNICODETEXT));
  utf16_text.clear();
  clipboard.ReadText(&utf16_text);
  EXPECT_EQ(utf16_string, utf16_text);

  clipboard.Destroy();
}

TEST(ClipboardTest, TestClipboardBitmap) {
  const char* pixel_data = "Fake bitmap data";

  Clipboard clipboard;
  Clipboard::ObjectMap objects;
  Clipboard::ObjectMapParam param(pixel_data, pixel_data + 16);

  objects[Clipboard::CBF_BITMAP].push_back(param);

  const Clipboard::BitmapSize size = { param.size() / 4, 1 };
  param.clear();
  const char* size_data = reinterpret_cast<const char*>(&size);
  param.assign(size_data, size_data + sizeof(size));
  objects[Clipboard::CBF_BITMAP].push_back(param);
  clipboard.WriteObjects(objects);

  // Check format
  EXPECT_FALSE(clipboard.IsFormatAvailable(CF_UNICODETEXT));
  EXPECT_TRUE(clipboard.IsFormatAvailable(CF_BITMAP));

  clipboard.Destroy();
}
