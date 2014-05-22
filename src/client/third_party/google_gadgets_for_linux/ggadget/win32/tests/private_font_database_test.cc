/*
  Copyright 2011 Google Inc.

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

#include <windows.h>
#ifdef GDIPVER
#undef GDIPVER
#endif
#define GDIPVER 0x0110
#include <gdiplus.h>
#include <string>
#include "ggadget/common.h"
#include "ggadget/win32/private_font_database.h"
#include "unittest/gtest.h"

using ggadget::win32::PrivateFontDatabase;
const wchar_t* kFontFile = L"bs.ttf";
const wchar_t* kFontName = L"Baroque Script";

TEST(PrivateFontDatabaseTest, AddAndCreatePrivateFonts) {
  PrivateFontDatabase database;
  wchar_t* srcdir = _wgetenv(L"srcdir");
  std::wstring font_file = kFontFile;
  if (srcdir && *srcdir) {
    font_file = std::wstring(srcdir) + L"\\" + kFontFile;
  }
  EXPECT_TRUE(database.AddPrivateFont(font_file.c_str()));
  EXPECT_FALSE(database.AddPrivateFont(L"Not a file!"));
  Gdiplus::FontFamily* family = database.CreateFontFamilyByName(kFontName);
  EXPECT_NE(reinterpret_cast<Gdiplus::FontFamily*>(NULL), family);
  if (family != NULL)
    EXPECT_TRUE(family->IsAvailable());
  EXPECT_TRUE(database.CreateFontFamilyByName(L"not a font!") == NULL);
}

int main(int argc, char** argv) {
  testing::ParseGTestFlags(&argc, argv);
  ULONG_PTR gdiplusToken;
  Gdiplus::GdiplusStartupInput gdiplusStartupInput;
  Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
  int result = RUN_ALL_TESTS();
  Gdiplus::GdiplusShutdown(gdiplusToken);
  return result;
}
