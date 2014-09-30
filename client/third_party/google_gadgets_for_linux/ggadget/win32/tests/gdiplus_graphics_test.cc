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

// This file contains the tests for GdiplusGraphics ,GdiplusCanvas, GdiplusFont
// and GdiplusImage. Modified from gtk/cairo_graphics_test.cc

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <io.h>
#include <string.h>
#include <windows.h>
#ifdef GDIPVER
#undef GDIPVER
#endif
#define GDIPVER 0x0110
#include <gdiplus.h>

#include <string>
#include <cstdlib>
#include <cstdio>

#include "ggadget/common.h"
#include "ggadget/scoped_ptr.h"
#include "ggadget/system_utils.h"
#include "ggadget/win32/gdiplus_canvas.h"
#include "ggadget/win32/gdiplus_graphics.h"
#include "ggadget/win32/gdiplus_image.h"
#include "ggadget/win32/gdiplus_font.h"
#include "unittest/gtest.h"

using ggadget::CanvasInterface;
using ggadget::Color;
using ggadget::ImageInterface;
using ggadget::FontInterface;
using ggadget::scoped_ptr;
using ggadget::UTF16Char;
using ggadget::UTF16String;
using ggadget::ConvertStringUTF16ToUTF8;
using ggadget::down_cast;
using ggadget::ReadFileContents;

using ggadget::win32::GdiplusCanvas;
using ggadget::win32::GdiplusGraphics;
using ggadget::win32::GdiplusFont;
using ggadget::win32::GdiplusImage;

const double kPi = 3.141592653589793;
bool g_savepng = false;

static std::string kTestFile120day("120day.png");
static std::string kTestFileBase("base.png");
static std::string kTestFileKitty419("kitty419.jpg");
static std::string kTestFileTestMask("testmask.png");
static std::string kTestFileOpaque("opaque.png");

// Get CLSID of specified encode, copied from msdn.
// link: http://msdn.microsoft.com/en-us/library/ms533843(v=vs.85).aspx
// @param format the format of image codec, such as "image/png"
// @param clsid[OUT]  the output clsid of given encoder
// Returns 0 if success, otherwise return -1.
int GetEncoderClsid(const wchar_t* format, CLSID* clsid) {
  UINT num = 0;          // number of image encoders
  UINT size = 0;         // size of the image encoder array in bytes

  Gdiplus::ImageCodecInfo* image_codec_info = NULL;

  Gdiplus::GetImageEncodersSize(&num, &size);
  if (size == 0)
    return -1;  // Failure

  image_codec_info = reinterpret_cast<Gdiplus::ImageCodecInfo*>(malloc(size));
  if (image_codec_info == NULL)
    return -1;  // Failure

  GetImageEncoders(num, size, image_codec_info);

  for (UINT j = 0; j < num; ++j) {
    if (wcscmp(image_codec_info[j].MimeType, format) == 0) {
      *clsid = image_codec_info[j].Clsid;
      free(image_codec_info);
      return 0;  // Success
    }
  }

  free(image_codec_info);
  return -1;  // Failure
}

// fixture for creating the GdiplusCanvas object
class GdiplusGfxTest : public testing::Test {
 protected:
  GdiplusGfxTest() : gfx_(2.0) {
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
    target_.reset(down_cast<GdiplusCanvas*>(gfx_.NewCanvas(300, 150)));
  }

  ~GdiplusGfxTest() {
    if (g_savepng) {
      const testing::TestInfo* const test_info =
          testing::UnitTest::GetInstance()->current_test_info();
      wchar_t file[100] = {0};
      wsprintf(file, L"%S.png", test_info->name());
      CLSID pngClsid;
      GetEncoderClsid(L"image/png", &pngClsid);
      target_->GetImage()->Save(file, &pngClsid);
    }
    target_.reset(NULL);
    Gdiplus::GdiplusShutdown(gdiplusToken);
  }
  GdiplusGraphics gfx_;
  scoped_ptr<GdiplusCanvas> target_;
  ULONG_PTR gdiplusToken;
};

TEST_F(GdiplusGfxTest, Zoom) {
  EXPECT_DOUBLE_EQ(2.0, gfx_.GetZoom());
  EXPECT_EQ(600, target_->GetImage()->GetWidth());
  EXPECT_EQ(300, target_->GetImage()->GetHeight());

  gfx_.SetZoom(1.0);
  EXPECT_DOUBLE_EQ(1.0, gfx_.GetZoom());

  EXPECT_DOUBLE_EQ(300.0, target_->GetWidth());
  EXPECT_DOUBLE_EQ(150.0, target_->GetHeight());
  EXPECT_EQ(300, target_->GetImage()->GetWidth());
  EXPECT_EQ(150, target_->GetImage()->GetHeight());
}

// This test is meaningful only with -savepng
TEST_F(GdiplusGfxTest, NewCanvas) {
  EXPECT_TRUE(target_->DrawFilledRect(150, 0, 150, 150, Color(1, 1, 1)));

  CanvasInterface* c = gfx_.NewCanvas(100, 100);
  ASSERT_TRUE(c != NULL);
  EXPECT_TRUE(c->DrawFilledRect(0, 0, 100, 100, Color(1, 0, 0)));

  c->Destroy();
  c = NULL;
}

TEST_F(GdiplusGfxTest, LoadImage) {
  std::string buffer;
  ReadFileContents(kTestFile120day.c_str(), &buffer);
  ImageInterface* img = gfx_.NewImage("", buffer, false);
  ASSERT_NE(static_cast<ImageInterface*>(NULL), img);
  ImageInterface* img1 = gfx_.NewImage("", buffer, false);
  ASSERT_NE(static_cast<ImageInterface*>(NULL), img1);
  ASSERT_TRUE(img != img1);
  img->Destroy();
  img1->Destroy();

  img = gfx_.NewImage(kTestFile120day, buffer, false);
  ASSERT_FALSE(NULL == img);
  img1 = gfx_.NewImage(kTestFile120day, buffer, false);
  ASSERT_FALSE(NULL == img1);
  ASSERT_TRUE(img != img1);
  img1->Destroy();
  img1 = gfx_.NewImage(kTestFile120day, buffer, true);
  ASSERT_TRUE(img != img1);
  img1->Destroy();

  EXPECT_TRUE(NULL == gfx_.NewImage("", std::string(), false));

  EXPECT_DOUBLE_EQ(450.0, img->GetWidth());
  EXPECT_DOUBLE_EQ(310.0, img->GetHeight());

  EXPECT_EQ(kTestFile120day, img->GetTag());

  img->Destroy();
  img = NULL;
}

// This test is meaningful only with -savepng
TEST_F(GdiplusGfxTest, DrawCanvas) {
  ImageInterface* img;
  double h, scale;

  // PNG
  std::string buffer;
  ReadFileContents(kTestFileBase.c_str(), &buffer);
  img = gfx_.NewImage("", buffer, false);
  ASSERT_FALSE(NULL == img);

  h = img->GetHeight();
  scale = 150. / h;

  EXPECT_FALSE(target_->DrawCanvas(50., 0., NULL));

  EXPECT_TRUE(target_->PushState());
  target_->ScaleCoordinates(scale, scale);
  EXPECT_TRUE(target_->MultiplyOpacity(.5));
  EXPECT_TRUE(target_->DrawCanvas(150., 0., img->GetCanvas()));
  EXPECT_TRUE(target_->PopState());

  img->Destroy();
  img = NULL;

  // JPG
  ReadFileContents(kTestFileKitty419.c_str(), &buffer);
  img = gfx_.NewImage("", buffer, false);
  ASSERT_FALSE(NULL == img);

  h = img->GetHeight();
  scale = 150. / h;
  EXPECT_TRUE(target_->PushState());
  target_->ScaleCoordinates(scale, scale);
  EXPECT_TRUE(target_->DrawCanvas(0., 0., img->GetCanvas()));
  EXPECT_TRUE(target_->PopState());
  img->Destroy();
  img = NULL;
}

// This test is meaningful only with -savepng
TEST_F(GdiplusGfxTest, DrawImageMask) {
  ImageInterface* mask, *img;
  double h, scale;

  std::string buffer;
  ReadFileContents(kTestFileTestMask.c_str(), &buffer);
  mask = gfx_.NewImage("", buffer, true);
  ASSERT_FALSE(NULL == mask);

  ReadFileContents(kTestFile120day.c_str(), &buffer);
  img = gfx_.NewImage("", buffer, false);
  ASSERT_FALSE(NULL == img);

  EXPECT_DOUBLE_EQ(450.0, mask->GetWidth());
  EXPECT_DOUBLE_EQ(310.0, mask->GetHeight());

  h = mask->GetHeight();
  scale = 150. / h;
  EXPECT_TRUE(target_->PushState());
  EXPECT_TRUE(target_->DrawFilledRect(0, 0, 300, 150, Color(0, 0, 1)));
  EXPECT_TRUE(target_->MultiplyOpacity(0.7));
  EXPECT_TRUE(target_->DrawCanvasWithMask(0, 0, img->GetCanvas(),
                                          0, 0, mask->GetCanvas()));
  EXPECT_TRUE(target_->PopState());

  CanvasInterface* c = gfx_.NewCanvas(100, 100);
  ASSERT_TRUE(c != NULL);
  EXPECT_TRUE(c->DrawFilledRect(0, 0, 100, 100, Color(0, 1, 0)));
  EXPECT_TRUE(target_->DrawCanvasWithMask(150, 0, c, 0, 0, mask->GetCanvas()));

  mask->Destroy();
  mask = NULL;

  img->Destroy();
  img = NULL;

  c->Destroy();
  c = NULL;
}

// This test is meaningful only with -savepng
TEST_F(GdiplusGfxTest, NewFontAndDrawText) {
  FontInterface* font1 = gfx_.NewFont(
      "Calibri", 14, FontInterface::STYLE_ITALIC, FontInterface::WEIGHT_BOLD);
  EXPECT_EQ(FontInterface::STYLE_ITALIC, font1->GetStyle());
  EXPECT_EQ(FontInterface::WEIGHT_BOLD, font1->GetWeight());
  EXPECT_DOUBLE_EQ(14.0, font1->GetPointSize());

  EXPECT_FALSE(target_->DrawText(0, 0, 100, 30, NULL, font1, Color(1, 0, 0),
               CanvasInterface::ALIGN_LEFT, CanvasInterface::VALIGN_TOP,
               CanvasInterface::TRIMMING_NONE, 0));
  EXPECT_FALSE(target_->DrawText(0, 0, 100, 30, "abc", NULL, Color(1, 0, 0),
               CanvasInterface::ALIGN_LEFT, CanvasInterface::VALIGN_TOP,
               CanvasInterface::TRIMMING_NONE, 0));

  ASSERT_TRUE(font1 != NULL);
  EXPECT_TRUE(target_->DrawText(0, 0, 100, 30, "hello world", font1,
              Color(1, 0, 0), CanvasInterface::ALIGN_LEFT,
              CanvasInterface::VALIGN_TOP,
              CanvasInterface::TRIMMING_NONE, 0));

  FontInterface* font2 = gfx_.NewFont("Times New Roman", 14,
      FontInterface::STYLE_NORMAL, FontInterface::WEIGHT_NORMAL);
  ASSERT_TRUE(font2 != NULL);
  EXPECT_TRUE(target_->DrawText(0, 30, 100, 30, "hello world", font2,
              Color(0, 1, 0), CanvasInterface::ALIGN_LEFT,
              CanvasInterface::VALIGN_TOP,
              CanvasInterface::TRIMMING_NONE, 0));

  FontInterface* font3 = gfx_.NewFont("Times New Roman", 14,
                                      FontInterface::STYLE_NORMAL,
                                      FontInterface::WEIGHT_BOLD);
  ASSERT_TRUE(font3 != NULL);
  EXPECT_TRUE(target_->DrawText(0, 60, 100, 30, "hello world", font3,
              Color(0, 0, 1), CanvasInterface::ALIGN_LEFT,
              CanvasInterface::VALIGN_TOP,
              CanvasInterface::TRIMMING_NONE, 0));

  FontInterface* font4 = gfx_.NewFont("Times New Roman", 14,
      FontInterface::STYLE_ITALIC, FontInterface::WEIGHT_NORMAL);
  ASSERT_TRUE(font4 != NULL);
  EXPECT_TRUE(target_->DrawText(0, 90, 100, 30, "hello world", font4,
              Color(0, 1, 1), CanvasInterface::ALIGN_LEFT,
              CanvasInterface::VALIGN_TOP,
              CanvasInterface::TRIMMING_NONE, 0));

  FontInterface* font5 = gfx_.NewFont("Times New Roman", 16,
      FontInterface::STYLE_NORMAL, FontInterface::WEIGHT_NORMAL);
  ASSERT_TRUE(font5 != NULL);
  EXPECT_TRUE(target_->DrawText(0, 120, 100, 30, "hello world", font5,
              Color(1, 1, 0), CanvasInterface::ALIGN_LEFT,
              CanvasInterface::VALIGN_TOP,
              CanvasInterface::TRIMMING_NONE, 0));

  font1->Destroy();
  font2->Destroy();
  font3->Destroy();
  font4->Destroy();
  font5->Destroy();
}

// This test is meaningful only with -savepng.
TEST_F(GdiplusGfxTest, DrawTextWithTexture) {
  ImageInterface* img;
  std::string buffer;
  ReadFileContents(kTestFileKitty419.c_str(), &buffer);
  img = gfx_.NewImage("", buffer, false);
  ASSERT_FALSE(NULL == img);

  FontInterface* font = gfx_.NewFont("Times New Roman", 20,
      FontInterface::STYLE_NORMAL, FontInterface::WEIGHT_BOLD);

  // Test underline, strikeout and wrap.
  EXPECT_TRUE(target_->DrawFilledRect(0, 0, 150, 90, Color(.7, 0, 0)));
  EXPECT_TRUE(target_->DrawTextWithTexture(0, 0, 150, 90,
              "hello world, gooooooogle",
              font, img->GetCanvas(), CanvasInterface::ALIGN_LEFT,
              CanvasInterface::VALIGN_TOP, CanvasInterface::TRIMMING_NONE,
              CanvasInterface::TEXT_FLAGS_UNDERLINE |
              CanvasInterface::TEXT_FLAGS_WORDWRAP));
  EXPECT_TRUE(target_->DrawFilledRect(0, 100, 150, 50, Color(.7, 0, 0)));
  EXPECT_TRUE(target_->DrawTextWithTexture(0, 100, 150, 50, "hello world",
              font, img->GetCanvas(), CanvasInterface::ALIGN_LEFT,
              CanvasInterface::VALIGN_TOP, CanvasInterface::TRIMMING_NONE,
              CanvasInterface::TEXT_FLAGS_UNDERLINE |
              CanvasInterface::TEXT_FLAGS_STRIKEOUT));

  // test alignment
  EXPECT_TRUE(target_->DrawFilledRect(180, 0, 120, 60, Color(.7, 0, 0)));
  EXPECT_TRUE(target_->DrawTextWithTexture(180, 0, 120, 60, "hello", font,
              img->GetCanvas(), CanvasInterface::ALIGN_CENTER,
              CanvasInterface::VALIGN_MIDDLE,
              CanvasInterface::TRIMMING_NONE, 0));
  EXPECT_TRUE(target_->DrawFilledRect(180, 80, 120, 60, Color(.7, 0, 0)));
  EXPECT_TRUE(target_->DrawTextWithTexture(180, 80, 120, 60, "hello", font,
              img->GetCanvas(), CanvasInterface::ALIGN_RIGHT,
              CanvasInterface::VALIGN_BOTTOM,
              CanvasInterface::TRIMMING_NONE, 0));

  img->Destroy();
  img = NULL;

  font->Destroy();
  font = NULL;
}

// This test is meaningful only with -savepng.
TEST_F(GdiplusGfxTest, TextAttributeAndAlignment) {
  FontInterface* font5 = gfx_.NewFont("Times New Roman", 16,
      FontInterface::STYLE_NORMAL, FontInterface::WEIGHT_NORMAL);

  // Test underline, strikeout and wrap.
  EXPECT_TRUE(target_->DrawFilledRect(0, 0, 100, 110, Color(.1, .1, 0)));
  EXPECT_TRUE(target_->DrawFilledRect(0, 120, 100, 30, Color(.3, .3, .1)));
  EXPECT_TRUE(target_->DrawText(0, 0, 100, 120, "hello world, gooooooogle",
              font5, Color(1, 1, 0), CanvasInterface::ALIGN_LEFT,
              CanvasInterface::VALIGN_TOP, CanvasInterface::TRIMMING_NONE,
              CanvasInterface::TEXT_FLAGS_UNDERLINE |
              CanvasInterface::TEXT_FLAGS_WORDWRAP));

  EXPECT_TRUE(target_->DrawText(0, 120, 100, 30, "hello world", font5,
              Color(1, 1, 0), CanvasInterface::ALIGN_LEFT,
              CanvasInterface::VALIGN_TOP, CanvasInterface::TRIMMING_NONE,
              CanvasInterface::TEXT_FLAGS_UNDERLINE |
              CanvasInterface::TEXT_FLAGS_STRIKEOUT));

  // Test alignment.
  EXPECT_TRUE(target_->DrawFilledRect(200, 0, 100, 60, Color(.1, .1, 0)));
  EXPECT_TRUE(target_->DrawFilledRect(200, 80, 100, 60, Color(.3, .3, .1)));
  EXPECT_TRUE(target_->DrawText(200, 0, 100, 60, "hello", font5,
              Color(1, 1, 1), CanvasInterface::ALIGN_CENTER,
              CanvasInterface::VALIGN_MIDDLE,
              CanvasInterface::TRIMMING_NONE, 0));
  EXPECT_TRUE(target_->DrawText(200, 80, 100, 60, "hello", font5,
              Color(1, 1, 1), CanvasInterface::ALIGN_RIGHT,
              CanvasInterface::VALIGN_BOTTOM,
              CanvasInterface::TRIMMING_NONE, 0));

  font5->Destroy();
}

// This test is meaningful only with -savepng.
TEST_F(GdiplusGfxTest, JustifyAlignmentTest) {
  FontInterface* font5 = gfx_.NewFont("Times New Roman", 14,
      FontInterface::STYLE_NORMAL, FontInterface::WEIGHT_NORMAL);
  EXPECT_TRUE(target_->DrawFilledRect(0, 0, 100, 80, Color(.3, .3, .1)));
  EXPECT_TRUE(target_->DrawText(
      0, 0, 100, 80, "This is a loooooooooogword !\n it is a new line.",
      font5, Color(1, 1, 0), CanvasInterface::ALIGN_JUSTIFY,
      CanvasInterface::VALIGN_TOP, CanvasInterface::TRIMMING_PATH_ELLIPSIS,
      CanvasInterface::TEXT_FLAGS_UNDERLINE |
      CanvasInterface::TEXT_FLAGS_WORDWRAP));

  EXPECT_TRUE(target_->DrawFilledRect(150, 0, 100, 80, Color(.3, .3, .1)));
  EXPECT_TRUE(target_->DrawText(
      150, 0, 100, 80, "This is a loooooooooogword !\nit is a new line.",
      font5, Color(1, 1, 0), CanvasInterface::ALIGN_LEFT,
      CanvasInterface::VALIGN_TOP, CanvasInterface::TRIMMING_PATH_ELLIPSIS,
      CanvasInterface::TEXT_FLAGS_UNDERLINE |
      CanvasInterface::TEXT_FLAGS_WORDWRAP));
}

// This test is meaningful only with -savepng.
TEST_F(GdiplusGfxTest, SinglelineTrimming) {
  FontInterface* font5 = gfx_.NewFont("Times New Roman", 16,
      FontInterface::STYLE_NORMAL, FontInterface::WEIGHT_NORMAL);

  EXPECT_TRUE(target_->DrawFilledRect(0, 0, 100, 30, Color(.1, .1, 0)));
  EXPECT_TRUE(target_->DrawFilledRect(200, 0, 100, 30, Color(.1, .1, 0)));
  EXPECT_TRUE(target_->DrawFilledRect(0, 40, 100, 30, Color(.1, .1, 0)));
  EXPECT_TRUE(target_->DrawFilledRect(200, 40, 100, 30, Color(.1, .1, 0)));
  EXPECT_TRUE(target_->DrawFilledRect(0, 80, 100, 30, Color(.1, .1, 0)));
  EXPECT_TRUE(target_->DrawFilledRect(200, 80, 100, 30, Color(.1, .1, 0)));

  EXPECT_TRUE(target_->DrawText(0, 0, 100, 30, "hello world", font5,
              Color(1, 1, 1), CanvasInterface::ALIGN_CENTER,
              CanvasInterface::VALIGN_BOTTOM,
              CanvasInterface::TRIMMING_NONE, 0));
  EXPECT_TRUE(target_->DrawText(0, 40, 100, 30, "hello world", font5,
              Color(1, 1, 1), CanvasInterface::ALIGN_CENTER,
              CanvasInterface::VALIGN_BOTTOM,
              CanvasInterface::TRIMMING_CHARACTER, 0));
  EXPECT_TRUE(target_->DrawText(0, 80, 100, 30, "hello world", font5,
              Color(1, 1, 1), CanvasInterface::ALIGN_CENTER,
              CanvasInterface::VALIGN_BOTTOM,
              CanvasInterface::TRIMMING_CHARACTER_ELLIPSIS, 0));
  EXPECT_TRUE(target_->DrawText(200, 0, 100, 30, "hello world", font5,
              Color(1, 1, 1), CanvasInterface::ALIGN_CENTER,
              CanvasInterface::VALIGN_BOTTOM,
              CanvasInterface::TRIMMING_WORD, 0));
  EXPECT_TRUE(target_->DrawText(200, 40, 100, 30, "hello world", font5,
              Color(1, 1, 1), CanvasInterface::ALIGN_CENTER,
              CanvasInterface::VALIGN_BOTTOM,
              CanvasInterface::TRIMMING_WORD_ELLIPSIS, 0));
  EXPECT_TRUE(target_->DrawText(200, 80, 100, 30, "hello world", font5,
              Color(1, 1, 1), CanvasInterface::ALIGN_CENTER,
              CanvasInterface::VALIGN_BOTTOM,
              CanvasInterface::TRIMMING_PATH_ELLIPSIS, 0));
  font5->Destroy();
}

// This test is meaningful only with -savepng.
TEST_F(GdiplusGfxTest, MultilineTrimming) {
  FontInterface* font5 = gfx_.NewFont("Times New Roman", 16,
      FontInterface::STYLE_NORMAL, FontInterface::WEIGHT_NORMAL);

  EXPECT_TRUE(target_->DrawFilledRect(0, 0, 100, 40, Color(.1, .1, 0)));
  EXPECT_TRUE(target_->DrawFilledRect(0, 50, 100, 40, Color(.1, .1, 0)));
  EXPECT_TRUE(target_->DrawFilledRect(0, 100, 100, 40, Color(.1, .1, 0)));
  EXPECT_TRUE(target_->DrawFilledRect(200, 0, 100, 40, Color(.1, .1, 0)));
  EXPECT_TRUE(target_->DrawFilledRect(200, 50, 100, 40, Color(.1, .1, 0)));
  EXPECT_TRUE(target_->DrawFilledRect(200, 100, 100, 40, Color(.1, .1, 0)));
  EXPECT_TRUE(target_->DrawText(0, 0, 100, 40, "Hello world, gooooogle",
              font5, Color(1, 1, 1), CanvasInterface::ALIGN_CENTER,
              CanvasInterface::VALIGN_MIDDLE,
              CanvasInterface::TRIMMING_NONE,
              CanvasInterface::TEXT_FLAGS_WORDWRAP));

  EXPECT_TRUE(target_->DrawText(0, 50, 100, 40, "Hello world, gooooogle",
              font5, Color(1, 1, 1), CanvasInterface::ALIGN_CENTER,
              CanvasInterface::VALIGN_MIDDLE,
              CanvasInterface::TRIMMING_CHARACTER,
              CanvasInterface::TEXT_FLAGS_WORDWRAP));

  EXPECT_TRUE(target_->DrawText(0, 100, 100, 40, "Hello world, gooooogle",
              font5, Color(1, 1, 1), CanvasInterface::ALIGN_CENTER,
              CanvasInterface::VALIGN_MIDDLE,
              CanvasInterface::TRIMMING_CHARACTER_ELLIPSIS,
              CanvasInterface::TEXT_FLAGS_WORDWRAP));

  EXPECT_TRUE(target_->DrawText(200, 0, 100, 40, "Hello world, gooooogle",
              font5, Color(1, 1, 1), CanvasInterface::ALIGN_CENTER,
              CanvasInterface::VALIGN_MIDDLE, CanvasInterface::TRIMMING_WORD,
              CanvasInterface::TEXT_FLAGS_WORDWRAP));

  EXPECT_TRUE(target_->DrawText(200, 50, 100, 40, "Hello world, gooooogle",
              font5, Color(1, 1, 1), CanvasInterface::ALIGN_CENTER,
              CanvasInterface::VALIGN_MIDDLE,
              CanvasInterface::TRIMMING_WORD_ELLIPSIS,
              CanvasInterface::TEXT_FLAGS_WORDWRAP));

  EXPECT_TRUE(target_->DrawText(200, 100, 100, 40, "Hello world, gooooogle",
              font5, Color(1, 1, 1), CanvasInterface::ALIGN_CENTER,
              CanvasInterface::VALIGN_MIDDLE,
              CanvasInterface::TRIMMING_PATH_ELLIPSIS,
              CanvasInterface::TEXT_FLAGS_WORDWRAP));

  font5->Destroy();
}

// This test is meaningful only with -savepng.
TEST_F(GdiplusGfxTest, ChineseTrimming) {
  UTF16String text(reinterpret_cast<const UTF16Char*>(L"你好，谷歌"));
  std::string utf8_string;
  ConvertStringUTF16ToUTF8(text, &utf8_string);
  FontInterface* font5 = gfx_.NewFont("Times New Roman", 16,
      FontInterface::STYLE_NORMAL, FontInterface::WEIGHT_NORMAL);
  EXPECT_TRUE(target_->DrawFilledRect(0, 0, 105, 40, Color(.1, .1, 0)));
  EXPECT_TRUE(target_->DrawFilledRect(0, 50, 105, 40, Color(.1, .1, 0)));
  EXPECT_TRUE(target_->DrawFilledRect(0, 100, 105, 40, Color(.1, .1, 0)));
  EXPECT_TRUE(target_->DrawFilledRect(180, 0, 105, 40, Color(.1, .1, 0)));
  EXPECT_TRUE(target_->DrawFilledRect(180, 50, 105, 40, Color(.1, .1, 0)));
  EXPECT_TRUE(target_->DrawFilledRect(180, 100, 105, 40, Color(.1, .1, 0)));
  EXPECT_TRUE(target_->DrawText(0, 0, 105, 40, utf8_string.c_str(),
              font5, Color(1, 1, 1), CanvasInterface::ALIGN_CENTER,
              CanvasInterface::VALIGN_MIDDLE,
              CanvasInterface::TRIMMING_NONE, 0));

  EXPECT_TRUE(target_->DrawText(0, 50, 105, 40, utf8_string.c_str(),
              font5, Color(1, 1, 1), CanvasInterface::ALIGN_CENTER,
              CanvasInterface::VALIGN_MIDDLE,
              CanvasInterface::TRIMMING_CHARACTER, 0));

  EXPECT_TRUE(target_->DrawText(0, 100, 105, 40, utf8_string.c_str(),
              font5, Color(1, 1, 1), CanvasInterface::ALIGN_CENTER,
              CanvasInterface::VALIGN_MIDDLE,
              CanvasInterface::TRIMMING_CHARACTER_ELLIPSIS, 0));

  EXPECT_TRUE(target_->DrawText(180, 0, 105, 40, utf8_string.c_str(),
              font5, Color(1, 1, 1), CanvasInterface::ALIGN_CENTER,
              CanvasInterface::VALIGN_MIDDLE,
              CanvasInterface::TRIMMING_WORD, 0));

  EXPECT_TRUE(target_->DrawText(180, 50, 105, 40, utf8_string.c_str(),
              font5, Color(1, 1, 1), CanvasInterface::ALIGN_CENTER,
              CanvasInterface::VALIGN_MIDDLE,
              CanvasInterface::TRIMMING_WORD_ELLIPSIS, 0));

  EXPECT_TRUE(target_->DrawText(180, 100, 105, 40, utf8_string.c_str(),
              font5, Color(1, 1, 1), CanvasInterface::ALIGN_CENTER,
              CanvasInterface::VALIGN_MIDDLE,
              CanvasInterface::TRIMMING_PATH_ELLIPSIS, 0));

  font5->Destroy();
}

// This test is meaningful only with -savepng.
TEST_F(GdiplusGfxTest, RTLTrimming) {
  FontInterface* font5 = gfx_.NewFont("Times New Roman", 16,
      FontInterface::STYLE_NORMAL, FontInterface::WEIGHT_NORMAL);
  UTF16String text(
      reinterpret_cast<const UTF16Char*>(L"سَدفهلكجشِلكَفهسدفلكجسدف"));
  std::string utf8_string;
  ConvertStringUTF16ToUTF8(text, &utf8_string);
  EXPECT_TRUE(target_->DrawFilledRect(0, 0, 100, 40, Color(.1, .1, 0)));
  EXPECT_TRUE(target_->DrawFilledRect(0, 50, 100, 40, Color(.1, .1, 0)));
  EXPECT_TRUE(target_->DrawFilledRect(0, 100, 100, 40, Color(.1, .1, 0)));
  EXPECT_TRUE(target_->DrawFilledRect(200, 0, 100, 40, Color(.1, .1, 0)));
  EXPECT_TRUE(target_->DrawFilledRect(200, 50, 100, 40, Color(.1, .1, 0)));
  EXPECT_TRUE(target_->DrawFilledRect(200, 100, 100, 40, Color(.1, .1, 0)));
  EXPECT_TRUE(target_->DrawText(0, 0, 100, 40,
              utf8_string.c_str(),
              font5, Color(1, 1, 1), CanvasInterface::ALIGN_CENTER,
              CanvasInterface::VALIGN_MIDDLE,
              CanvasInterface::TRIMMING_NONE, 0));

  EXPECT_TRUE(target_->DrawText(0, 50, 100, 40,
              utf8_string.c_str(),
              font5, Color(1, 1, 1), CanvasInterface::ALIGN_CENTER,
              CanvasInterface::VALIGN_MIDDLE,
              CanvasInterface::TRIMMING_CHARACTER, 0));

  EXPECT_TRUE(target_->DrawText(0, 100, 100, 40,
              utf8_string.c_str(),
              font5, Color(1, 1, 1), CanvasInterface::ALIGN_CENTER,
              CanvasInterface::VALIGN_MIDDLE,
              CanvasInterface::TRIMMING_CHARACTER_ELLIPSIS, 0));

  EXPECT_TRUE(target_->DrawText(200, 0, 100, 40,
              utf8_string.c_str(),
              font5, Color(1, 1, 1), CanvasInterface::ALIGN_CENTER,
              CanvasInterface::VALIGN_MIDDLE,
              CanvasInterface::TRIMMING_WORD, 0));

  EXPECT_TRUE(target_->DrawText(200, 50, 100, 40,
              utf8_string.c_str(),
              font5, Color(1, 1, 1), CanvasInterface::ALIGN_CENTER,
              CanvasInterface::VALIGN_MIDDLE,
              CanvasInterface::TRIMMING_WORD_ELLIPSIS, 0));

  EXPECT_TRUE(target_->DrawText(200, 100, 100, 40,
              utf8_string.c_str(),
              font5, Color(1, 1, 1), CanvasInterface::ALIGN_CENTER,
              CanvasInterface::VALIGN_MIDDLE,
              CanvasInterface::TRIMMING_PATH_ELLIPSIS, 0));

  font5->Destroy();
}

// This test is meaningful only with -savepng.
TEST_F(GdiplusGfxTest, ColorMultiply) {
  ImageInterface* img;
  double h, scale;

  // PNG
  std::string buffer;
  ReadFileContents(kTestFileBase.c_str(), &buffer);
  img = gfx_.NewImage("", buffer, false);
  ASSERT_FALSE(NULL == img);

  h = img->GetHeight();
  scale = 150. / h;

  ImageInterface* img1 = img->MultiplyColor(Color(0, 0.5, 1));
  EXPECT_TRUE(target_->PushState());
  target_->ScaleCoordinates(scale, scale);
  EXPECT_TRUE(target_->MultiplyOpacity(.5));
  EXPECT_TRUE(target_->DrawCanvas(150., 0., img1->GetCanvas()));
  EXPECT_TRUE(target_->PopState());

  // JPG
  ReadFileContents(kTestFileBase.c_str(), &buffer);
  img = gfx_.NewImage("", buffer, false);
  ASSERT_FALSE(NULL == img);

  h = img->GetHeight();
  scale = 150. / h;
  img1 = img->MultiplyColor(Color(0.5, 0, 0.8));
  EXPECT_TRUE(target_->PushState());
  target_->ScaleCoordinates(scale, scale);
  EXPECT_TRUE(target_->DrawCanvas(0., 0., img1->GetCanvas()));
  EXPECT_TRUE(target_->PopState());
  img1->Destroy();
  img1 = NULL;
  img->Destroy();
  img = NULL;
}

TEST_F(GdiplusGfxTest, ImageOpaque) {
  struct Images {
    std::string filename;
    bool opaque;
  } images[] = {
    { kTestFile120day, true },
    { kTestFileBase, false },
    { kTestFileOpaque, true },
    { "", false }
  };

  for (size_t i = 0; images[i].filename.length(); ++i) {
    std::string content;
    ASSERT_TRUE(ReadFileContents(images[i].filename.c_str(), &content));
    ImageInterface* img = gfx_.NewImage("", content, false);
    ASSERT_TRUE(img);
    EXPECT_EQ(images[i].opaque, img->IsFullyOpaque());
    img->Destroy();
    img = NULL;
  }
}


int main(int argc, char** argv) {
  testing::ParseGTestFlags(&argc, argv);

  // g_type_init();
  // Hack for autoconf out-of-tree build.
  char* srcdir = getenv("srcdir");
  if (srcdir && *srcdir) {
    kTestFile120day = std::string(srcdir) + std::string("/") + kTestFile120day;
    kTestFileBase = std::string(srcdir) + std::string("/") + kTestFileBase;
    kTestFileKitty419 = std::string(srcdir) + std::string("/")
                      + kTestFileKitty419;
    kTestFileTestMask = std::string(srcdir) + std::string("/")
                      + kTestFileTestMask;
    kTestFileOpaque = std::string(srcdir) + std::string("/") + kTestFileOpaque;
  }

  for (int i = 0; i < argc; i++) {
    if (0 == strcasecmp(argv[i], "-savepng")) {
      g_savepng = true;
      break;
    }
  }

  return RUN_ALL_TESTS();
}
