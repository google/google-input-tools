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

#include <cstdlib>
#include <cstdio>
#include <cairo.h>
#include <glib-object.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <strings.h>
#include <string>

#include "ggadget/common.h"
#include "ggadget/system_utils.h"
#include "ggadget/gtk/cairo_canvas.h"
#include "ggadget/gtk/cairo_graphics.h"
#include "unittest/gtest.h"

using namespace ggadget;
using namespace ggadget::gtk;

const double kPi = 3.141592653589793;
bool g_savepng = false;

static std::string kTestFile120day("120day.png");
static std::string kTestFileBase("base.png");
static std::string kTestFileKitty419("kitty419.jpg");
static std::string kTestFileTestMask("testmask.png");
static std::string kTestFileOpaque("opaque.png");

// fixture for creating the CairoCanvas object
class CairoGfxTest : public testing::Test {
 protected:
  CairoGraphics gfx_;
  CairoCanvas *target_;

  CairoGfxTest() : gfx_(2.0) {
    target_ = down_cast<CairoCanvas*>(gfx_.NewCanvas(300, 150));
  }

  ~CairoGfxTest() {
    if (g_savepng) {
      const testing::TestInfo *const test_info =
        testing::UnitTest::GetInstance()->current_test_info();
      char file[100];
      snprintf(file, arraysize(file), "%s.png", test_info->name());
      cairo_surface_write_to_png(target_->GetSurface(), file);
    }
    target_->Destroy();
  }
};

TEST_F(CairoGfxTest, Zoom) {
  EXPECT_DOUBLE_EQ(2.0, gfx_.GetZoom());
  EXPECT_EQ(600, cairo_image_surface_get_width(target_->GetSurface()));
  EXPECT_EQ(300, cairo_image_surface_get_height(target_->GetSurface()));
  gfx_.SetZoom(1.0);
  EXPECT_DOUBLE_EQ(1.0, gfx_.GetZoom());
  gfx_.SetZoom(0);
  EXPECT_DOUBLE_EQ(1.0, gfx_.GetZoom());

  EXPECT_DOUBLE_EQ(300.0, target_->GetWidth());
  EXPECT_DOUBLE_EQ(150.0, target_->GetHeight());
  EXPECT_EQ(300, cairo_image_surface_get_width(target_->GetSurface()));
  EXPECT_EQ(150, cairo_image_surface_get_height(target_->GetSurface()));
}

// this test is meaningful only with -savepng
TEST_F(CairoGfxTest, NewCanvas) {
  EXPECT_TRUE(target_->DrawFilledRect(150, 0, 150, 150, Color(1, 1, 1)));

  CanvasInterface *c = gfx_.NewCanvas(100, 100);
  ASSERT_TRUE(c != NULL);
  EXPECT_TRUE(c->DrawFilledRect(0, 0, 100, 100, Color(1, 0, 0)));

  EXPECT_TRUE(target_->DrawCanvas(50, 50, c));

  c->Destroy();
  c = NULL;
}

TEST_F(CairoGfxTest, LoadImage) {
  char *buffer = NULL;

  int fd = open(kTestFile120day.c_str(), O_RDONLY);
  ASSERT_NE(-1, fd);

  struct stat statvalue;
  ASSERT_EQ(0, fstat(fd, &statvalue));

  size_t filelen = statvalue.st_size;
  ASSERT_NE(0U, filelen);

  buffer = (char*)mmap(NULL, filelen, PROT_READ, MAP_PRIVATE, fd, 0);
  ASSERT_NE(MAP_FAILED, buffer);

  ImageInterface *img = gfx_.NewImage("", std::string(buffer, filelen), false);
  ASSERT_FALSE(NULL == img);
  ImageInterface *img1 = gfx_.NewImage("", std::string(buffer, filelen), false);
  ASSERT_FALSE(NULL == img1);
  ASSERT_TRUE(img != img1);
  img->Destroy();
  img1->Destroy();

  img = gfx_.NewImage(kTestFile120day, std::string(buffer, filelen), false);
  ASSERT_FALSE(NULL == img);
  img1 = gfx_.NewImage(kTestFile120day, std::string(buffer, filelen), false);
  ASSERT_FALSE(NULL == img1);
  ASSERT_TRUE(img != img1);
  img1->Destroy();
  img1 = gfx_.NewImage(kTestFile120day, std::string(buffer, filelen), true);
  ASSERT_TRUE(img != img1);
  img1->Destroy();

  EXPECT_TRUE(NULL == gfx_.NewImage("", std::string(), false));

  EXPECT_DOUBLE_EQ(450.0, img->GetWidth());
  EXPECT_DOUBLE_EQ(310.0, img->GetHeight());

  EXPECT_EQ(kTestFile120day, img->GetTag());

  img->Destroy();
  img = NULL;

  munmap(buffer, filelen);
}

// this test is meaningful only with -savepng
TEST_F(CairoGfxTest, DrawCanvas) {
  char *buffer = NULL;
  struct stat statvalue;
  size_t filelen;
  ImageInterface *img;
  double h, scale;

  // PNG
  int fd = open(kTestFileBase.c_str(), O_RDONLY);
  ASSERT_NE(-1, fd);

  ASSERT_EQ(0, fstat(fd, &statvalue));
  filelen = statvalue.st_size;
  ASSERT_NE(0U, filelen);

  buffer = (char*)mmap(NULL, filelen, PROT_READ, MAP_PRIVATE, fd, 0);
  ASSERT_NE(MAP_FAILED, buffer);

  img = gfx_.NewImage("", std::string(buffer, filelen), false);
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

  munmap(buffer, filelen);

  // JPG
  fd = open(kTestFileKitty419.c_str(), O_RDONLY);
  ASSERT_NE(-1, fd);

  ASSERT_EQ(0, fstat(fd, &statvalue));
  filelen = statvalue.st_size;
  ASSERT_NE(0U, filelen);

  buffer = (char*)mmap(NULL, filelen, PROT_READ, MAP_PRIVATE, fd, 0);
  ASSERT_NE(MAP_FAILED, buffer);

  img = gfx_.NewImage("", std::string(buffer, filelen), false);
  ASSERT_FALSE(NULL == img);

  h = img->GetHeight();
  scale = 150. / h;
  target_->ScaleCoordinates(scale, scale);
  EXPECT_TRUE(target_->DrawCanvas(0., 0., img->GetCanvas()));

  img->Destroy();
  img = NULL;

  munmap(buffer, filelen);
}

// this test is meaningful only with -savepng
TEST_F(CairoGfxTest, DrawImageMask) {
  char *buffer = NULL;
  struct stat statvalue;
  size_t filelen;
  ImageInterface *mask, *img;
  double h, scale;

  int fd = open(kTestFileTestMask.c_str(), O_RDONLY);
  ASSERT_NE(-1, fd);

  ASSERT_EQ(0, fstat(fd, &statvalue));
  filelen = statvalue.st_size;
  ASSERT_NE(0U, filelen);

  buffer = (char*)mmap(NULL, filelen, PROT_READ, MAP_PRIVATE, fd, 0);
  ASSERT_NE(MAP_FAILED, buffer);

  mask = gfx_.NewImage("", std::string(buffer, filelen), true);
  ASSERT_FALSE(NULL == mask);
  img = gfx_.NewImage("", std::string(buffer, filelen), false);
  ASSERT_FALSE(NULL == img);

  EXPECT_DOUBLE_EQ(450.0, mask->GetWidth());
  EXPECT_DOUBLE_EQ(310.0, mask->GetHeight());

  h = mask->GetHeight();
  scale = 150. / h;

  EXPECT_TRUE(target_->DrawFilledRect(0, 0, 300, 150, Color(0, 0, 1)));
  EXPECT_TRUE(target_->DrawCanvasWithMask(0, 0, img->GetCanvas(),
                                          0, 0, mask->GetCanvas()));

  CanvasInterface *c = gfx_.NewCanvas(100, 100);
  ASSERT_TRUE(c != NULL);
  EXPECT_TRUE(c->DrawFilledRect(0, 0, 100, 100, Color(0, 1, 0)));
  EXPECT_TRUE(target_->DrawCanvasWithMask(150, 0, c, 0, 0, mask->GetCanvas()));

  mask->Destroy();
  mask = NULL;

  img->Destroy();
  img = NULL;

  c->Destroy();
  c = NULL;

  munmap(buffer, filelen);
}

// this test is meaningful only with -savepng
TEST_F(CairoGfxTest, NewFontAndDrawText) {
  FontInterface *font1 = gfx_.NewFont("Serif", 14,
      FontInterface::STYLE_ITALIC, FontInterface::WEIGHT_BOLD);
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

  FontInterface *font2 = gfx_.NewFont("Serif", 14,
      FontInterface::STYLE_NORMAL, FontInterface::WEIGHT_NORMAL);
  ASSERT_TRUE(font2 != NULL);
  EXPECT_TRUE(target_->DrawText(0, 30, 100, 30, "hello world", font2,
              Color(0, 1, 0), CanvasInterface::ALIGN_LEFT,
              CanvasInterface::VALIGN_TOP,
              CanvasInterface::TRIMMING_NONE, 0));

  FontInterface *font3 = gfx_.NewFont("Serif", 14, FontInterface::STYLE_NORMAL,
      FontInterface::WEIGHT_BOLD);
  ASSERT_TRUE(font3 != NULL);
  EXPECT_TRUE(target_->DrawText(0, 60, 100, 30, "hello world", font3,
              Color(0, 0, 1), CanvasInterface::ALIGN_LEFT,
              CanvasInterface::VALIGN_TOP,
              CanvasInterface::TRIMMING_NONE, 0));

  FontInterface *font4 = gfx_.NewFont("Serif", 14,
      FontInterface::STYLE_ITALIC, FontInterface::WEIGHT_NORMAL);
  ASSERT_TRUE(font4 != NULL);
  EXPECT_TRUE(target_->DrawText(0, 90, 100, 30, "hello world", font4,
              Color(0, 1, 1), CanvasInterface::ALIGN_LEFT,
              CanvasInterface::VALIGN_TOP,
              CanvasInterface::TRIMMING_NONE, 0));

  FontInterface *font5 = gfx_.NewFont("Sans Serif", 16,
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

// this test is meaningful only with -savepng
TEST_F(CairoGfxTest, DrawTextWithTexture) {
  char *buffer = NULL;
  struct stat statvalue;
  size_t filelen;
  ImageInterface *img;

  int fd = open(kTestFileKitty419.c_str(), O_RDONLY);
  ASSERT_NE(-1, fd);

  ASSERT_EQ(0, fstat(fd, &statvalue));
  filelen = statvalue.st_size;
  ASSERT_NE(0U, filelen);

  buffer = (char*)mmap(NULL, filelen, PROT_READ, MAP_PRIVATE, fd, 0);
  ASSERT_NE(MAP_FAILED, buffer);

  img = gfx_.NewImage("", std::string(buffer, filelen), false);
  ASSERT_FALSE(NULL == img);

  FontInterface *font = gfx_.NewFont("Sans Serif", 20,
      FontInterface::STYLE_NORMAL, FontInterface::WEIGHT_BOLD);

  // test underline, strikeout and wrap
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

  munmap(buffer, filelen);
}

// this test is meaningful only with -savepng
TEST_F(CairoGfxTest, TextAttributeAndAlignment) {
  FontInterface *font5 = gfx_.NewFont("Sans Serif", 16,
      FontInterface::STYLE_NORMAL, FontInterface::WEIGHT_NORMAL);

  // test underline, strikeout and wrap
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

  // test alignment
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

// this test is meaningful only with -savepng
TEST_F(CairoGfxTest, SinglelineTrimming) {
  FontInterface *font5 = gfx_.NewFont("Sans Serif", 16,
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

// this test is meaningful only with -savepng
TEST_F(CairoGfxTest, MultilineTrimming) {
  FontInterface *font5 = gfx_.NewFont("Sans Serif", 16,
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

// this test is meaningful only with -savepng
TEST_F(CairoGfxTest, ChineseTrimming) {
  FontInterface *font5 = gfx_.NewFont("Sans Serif", 16,
      FontInterface::STYLE_NORMAL, FontInterface::WEIGHT_NORMAL);

  EXPECT_TRUE(target_->DrawFilledRect(0, 0, 105, 40, Color(.1, .1, 0)));
  EXPECT_TRUE(target_->DrawFilledRect(0, 50, 105, 40, Color(.1, .1, 0)));
  EXPECT_TRUE(target_->DrawFilledRect(0, 100, 105, 40, Color(.1, .1, 0)));
  EXPECT_TRUE(target_->DrawFilledRect(180, 0, 105, 40, Color(.1, .1, 0)));
  EXPECT_TRUE(target_->DrawFilledRect(180, 50, 105, 40, Color(.1, .1, 0)));
  EXPECT_TRUE(target_->DrawFilledRect(180, 100, 105, 40, Color(.1, .1, 0)));
  EXPECT_TRUE(target_->DrawText(0, 0, 105, 40, "你好，谷歌",
              font5, Color(1, 1, 1), CanvasInterface::ALIGN_CENTER,
              CanvasInterface::VALIGN_MIDDLE,
              CanvasInterface::TRIMMING_NONE, 0));

  EXPECT_TRUE(target_->DrawText(0, 50, 105, 40, "你好，谷歌",
              font5, Color(1, 1, 1), CanvasInterface::ALIGN_CENTER,
              CanvasInterface::VALIGN_MIDDLE,
              CanvasInterface::TRIMMING_CHARACTER,0));

  EXPECT_TRUE(target_->DrawText(0, 100, 105, 40, "你好，谷歌",
              font5, Color(1, 1, 1), CanvasInterface::ALIGN_CENTER,
              CanvasInterface::VALIGN_MIDDLE,
              CanvasInterface::TRIMMING_CHARACTER_ELLIPSIS, 0));

  EXPECT_TRUE(target_->DrawText(180, 0, 105, 40, "你好，谷歌",
              font5, Color(1, 1, 1), CanvasInterface::ALIGN_CENTER,
              CanvasInterface::VALIGN_MIDDLE,
              CanvasInterface::TRIMMING_WORD, 0));

  EXPECT_TRUE(target_->DrawText(180, 50, 105, 40, "你好，谷歌",
              font5, Color(1, 1, 1), CanvasInterface::ALIGN_CENTER,
              CanvasInterface::VALIGN_MIDDLE,
              CanvasInterface::TRIMMING_WORD_ELLIPSIS, 0));

  EXPECT_TRUE(target_->DrawText(180, 100, 105, 40, "你好，谷歌",
              font5, Color(1, 1, 1), CanvasInterface::ALIGN_CENTER,
              CanvasInterface::VALIGN_MIDDLE,
              CanvasInterface::TRIMMING_PATH_ELLIPSIS, 0));

  font5->Destroy();
}

// this test is meaningful only with -savepng
TEST_F(CairoGfxTest, RTLTrimming) {
  FontInterface *font5 = gfx_.NewFont("Sans Serif", 16,
      FontInterface::STYLE_NORMAL, FontInterface::WEIGHT_NORMAL);

  EXPECT_TRUE(target_->DrawFilledRect(0, 0, 100, 40, Color(.1, .1, 0)));
  EXPECT_TRUE(target_->DrawFilledRect(0, 50, 100, 40, Color(.1, .1, 0)));
  EXPECT_TRUE(target_->DrawFilledRect(0, 100, 100, 40, Color(.1, .1, 0)));
  EXPECT_TRUE(target_->DrawFilledRect(200, 0, 100, 40, Color(.1, .1, 0)));
  EXPECT_TRUE(target_->DrawFilledRect(200, 50, 100, 40, Color(.1, .1, 0)));
  EXPECT_TRUE(target_->DrawFilledRect(200, 100, 100, 40, Color(.1, .1, 0)));
  EXPECT_TRUE(target_->DrawText(0, 0, 100, 40,
              "سَدفهلكجشِلكَفهسدفلكجسدف",
              font5, Color(1, 1, 1), CanvasInterface::ALIGN_CENTER,
              CanvasInterface::VALIGN_MIDDLE,
              CanvasInterface::TRIMMING_NONE, 0));

  EXPECT_TRUE(target_->DrawText(0, 50, 100, 40,
              "سَدفهلكجشِلكَفهسدفلكجسدف",
              font5, Color(1, 1, 1), CanvasInterface::ALIGN_CENTER,
              CanvasInterface::VALIGN_MIDDLE,
              CanvasInterface::TRIMMING_CHARACTER,0));

  EXPECT_TRUE(target_->DrawText(0, 100, 100, 40,
              "سَدفهلكجشِلكَفهسدفلكجسدف",
              font5, Color(1, 1, 1), CanvasInterface::ALIGN_CENTER,
              CanvasInterface::VALIGN_MIDDLE,
              CanvasInterface::TRIMMING_CHARACTER_ELLIPSIS, 0));

  EXPECT_TRUE(target_->DrawText(200, 0, 100, 40,
              "سَدفهلكجشِلكَفهسدفلكجسدف",
              font5, Color(1, 1, 1), CanvasInterface::ALIGN_CENTER,
              CanvasInterface::VALIGN_MIDDLE,
              CanvasInterface::TRIMMING_WORD, 0));

  EXPECT_TRUE(target_->DrawText(200, 50, 100, 40,
              "سَدفهلكجشِلكَفهسدفلكجسدف",
              font5, Color(1, 1, 1), CanvasInterface::ALIGN_CENTER,
              CanvasInterface::VALIGN_MIDDLE,
              CanvasInterface::TRIMMING_WORD_ELLIPSIS, 0));

  EXPECT_TRUE(target_->DrawText(200, 100, 100, 40,
              "سَدفهلكجشِلكَفهسدفلكجسدف",
              font5, Color(1, 1, 1), CanvasInterface::ALIGN_CENTER,
              CanvasInterface::VALIGN_MIDDLE,
              CanvasInterface::TRIMMING_PATH_ELLIPSIS, 0));

  font5->Destroy();
}

// this test is meaningful only with -savepng
TEST_F(CairoGfxTest, ColorMultiply) {
  char *buffer = NULL;
  struct stat statvalue;
  size_t filelen;
  ImageInterface *img;
  double h, scale;

  // PNG
  int fd = open(kTestFileBase.c_str(), O_RDONLY);
  ASSERT_NE(-1, fd);

  ASSERT_EQ(0, fstat(fd, &statvalue));
  filelen = statvalue.st_size;
  ASSERT_NE(0U, filelen);

  buffer = (char*)mmap(NULL, filelen, PROT_READ, MAP_PRIVATE, fd, 0);
  ASSERT_NE(MAP_FAILED, buffer);

  img = gfx_.NewImage("", std::string(buffer, filelen), false);
  ASSERT_FALSE(NULL == img);

  h = img->GetHeight();
  scale = 150. / h;

  ImageInterface *img1 = img->MultiplyColor(Color(0, 0.5, 1));
  EXPECT_TRUE(target_->PushState());
  target_->ScaleCoordinates(scale, scale);
  EXPECT_TRUE(target_->MultiplyOpacity(.5));
  EXPECT_TRUE(target_->DrawCanvas(150., 0., img1->GetCanvas()));
  EXPECT_TRUE(target_->PopState());

  img->Destroy();
  img = NULL;
  img1->Destroy();
  img1 = NULL;

  munmap(buffer, filelen);

  // JPG
  fd = open(kTestFileKitty419.c_str(), O_RDONLY);
  ASSERT_NE(-1, fd);

  ASSERT_EQ(0, fstat(fd, &statvalue));
  filelen = statvalue.st_size;
  ASSERT_NE(0U, filelen);

  buffer = (char*)mmap(NULL, filelen, PROT_READ, MAP_PRIVATE, fd, 0);
  ASSERT_NE(MAP_FAILED, buffer);

  img = gfx_.NewImage("", std::string(buffer, filelen), false);
  ASSERT_FALSE(NULL == img);

  h = img->GetHeight();
  scale = 150. / h;
  img1 = img->MultiplyColor(Color(0.5, 0, 0.8));
  target_->ScaleCoordinates(scale, scale);
  EXPECT_TRUE(target_->DrawCanvas(0., 0., img1->GetCanvas()));

  img1->Destroy();
  img1 = NULL;
  img->Destroy();
  img = NULL;

  munmap(buffer, filelen);
}

TEST_F(CairoGfxTest, ImageOpaque) {
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
    ImageInterface *img = gfx_.NewImage("", content, false);
    ASSERT_TRUE(img);
    EXPECT_EQ(images[i].opaque, img->IsFullyOpaque());
    img->Destroy();
    img = NULL;
  }
}


int main(int argc, char **argv) {
  testing::ParseGTestFlags(&argc, argv);

  g_type_init();
  // Hack for autoconf out-of-tree build.
  char *srcdir = getenv("srcdir");
  if (srcdir && *srcdir) {
    kTestFile120day = std::string(srcdir) + std::string("/") + kTestFile120day;
    kTestFileBase = std::string(srcdir) + std::string("/") + kTestFileBase;
    kTestFileKitty419 = std::string(srcdir) + std::string("/") + kTestFileKitty419;
    kTestFileTestMask = std::string(srcdir) + std::string("/") + kTestFileTestMask;
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
