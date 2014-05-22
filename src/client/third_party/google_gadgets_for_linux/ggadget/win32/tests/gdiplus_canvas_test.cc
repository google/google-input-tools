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

// The file contains the unit tests of the class GdiplusCanvas.
// modified from gtk/cairo_canvas_test.cc

#include <math.h>
#include <windows.h>
#ifdef GDIPVER
#undef GDIPVER
#endif
#define GDIPVER 0x0110
#include <gdiPlus.h>
#include "ggadget/common.h"
#include "ggadget/color.h"
#include "ggadget/canvas_interface.h"
#include "ggadget/scoped_ptr.h"
#include "ggadget/win32/gdiplus_canvas.h"
#include "ggadget/win32/gdiplus_graphics.h"

#include "unittest/gtest.h"

using ggadget::Color;
using ggadget::scoped_ptr;
using ggadget::down_cast;
using ggadget::win32::GdiplusCanvas;
using ggadget::win32::GdiplusGraphics;

bool g_savepng = false;

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

class GdiplusCanvasTest : public testing::Test {
 protected:
  GdiplusCanvasTest() : graphics_(1.0, NULL) {
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
    canvas_.reset(down_cast<GdiplusCanvas*>(graphics_.NewCanvas(300, 150)));
  }

  ~GdiplusCanvasTest() {
    if (g_savepng) {
      const testing::TestInfo* const test_info
        = testing::UnitTest::GetInstance()->current_test_info();
      wchar_t file[100] = {0};
      wsprintf(file, L"%S.png", test_info->name());
      CLSID pngClsid;
      GetEncoderClsid(L"image/png", &pngClsid);
      canvas_->GetImage()->Save(file, &pngClsid);
    }
    canvas_.reset(NULL);
    Gdiplus::GdiplusShutdown(gdiplusToken);
  }
  GdiplusGraphics graphics_;
  scoped_ptr<GdiplusCanvas> canvas_;
  ULONG_PTR gdiplusToken;
};

TEST_F(GdiplusCanvasTest, PushPopStateReturnValues) {
  EXPECT_FALSE(canvas_->PopState());

  // push 1x, pop 1x
  EXPECT_TRUE(canvas_->PushState());
  EXPECT_TRUE(canvas_->PopState());
  EXPECT_FALSE(canvas_->PopState());

  // push 3x, pop 3x
  EXPECT_TRUE(canvas_->PushState());
  EXPECT_TRUE(canvas_->PushState());
  EXPECT_TRUE(canvas_->PushState());
  EXPECT_TRUE(canvas_->PopState());
  EXPECT_TRUE(canvas_->PopState());
  EXPECT_TRUE(canvas_->PopState());
  EXPECT_FALSE(canvas_->PopState());

  EXPECT_FALSE(canvas_->PopState());
}

TEST_F(GdiplusCanvasTest, OpacityReturnValues) {
  EXPECT_FALSE(canvas_->MultiplyOpacity(1.7));
  EXPECT_TRUE(canvas_->MultiplyOpacity(.5));
  EXPECT_FALSE(canvas_->MultiplyOpacity(-.7));
  EXPECT_TRUE(canvas_->MultiplyOpacity(.7));
  EXPECT_FALSE(canvas_->MultiplyOpacity(1000.));
  EXPECT_TRUE(canvas_->MultiplyOpacity(.2));
}

TEST_F(GdiplusCanvasTest, DrawLines) {
  EXPECT_FALSE(canvas_->DrawLine(10., 10., 200., 20., -1., Color(1., 0., 0.)));
  EXPECT_TRUE(canvas_->DrawLine(10., 10., 200., 20., 1., Color(1., 0., 0.)));
  EXPECT_TRUE(canvas_->DrawLine(10., 30., 200., 30., 2., Color(0., 1., 0.)));
  EXPECT_TRUE(canvas_->DrawLine(10., 40., 200., 40., 1.5, Color(0., 0., 1.)));
  EXPECT_TRUE(canvas_->DrawLine(10., 50., 200., 50., 1., Color(0., 0., 0.)));
  EXPECT_TRUE(canvas_->DrawLine(10., 60., 200., 60., 4., Color(1., 1., 1.)));
}

TEST_F(GdiplusCanvasTest, DrawRectReturnValues) {
  EXPECT_FALSE(canvas_->DrawFilledRect(5., 6., -1., 5., Color(0., 0., 0.)));
  EXPECT_TRUE(canvas_->DrawFilledRect(5., 6., 1., 5., Color(0., 0., 0.)));
  EXPECT_TRUE(canvas_->DrawFilledRect(10., 10., 10., 5., Color(1., 0., 0.)));
  EXPECT_FALSE(canvas_->DrawFilledRect(5., 6., 1., -5., Color(0., 0., 0.)));
}

TEST_F(GdiplusCanvasTest, ClipRectReturnValues) {
  EXPECT_FALSE(canvas_->IntersectRectClipRegion(5., 6., -1., 5.));
  EXPECT_TRUE(canvas_->IntersectRectClipRegion(5., 6., 1., 5.));
  EXPECT_FALSE(canvas_->IntersectRectClipRegion(5., 6., 1., -5.));
}

// this test is meaningful only with -savepng
TEST_F(GdiplusCanvasTest, PushPopStateLines) {
  // should show up as 1.0
  EXPECT_TRUE(canvas_->DrawLine(10., 10., 200., 10., 10., Color(1., 0., 0.)));
  EXPECT_TRUE(canvas_->MultiplyOpacity(1.0));
  // should show up as 1.0
  EXPECT_TRUE(canvas_->DrawLine(10., 30., 200., 30., 10., Color(1., 0., 0.)));
  EXPECT_TRUE(canvas_->PushState());
  EXPECT_TRUE(canvas_->MultiplyOpacity(.5));
  // should show up as .5
  EXPECT_TRUE(canvas_->DrawLine(10., 50., 200., 50., 10., Color(1., 0., 0.)));
  EXPECT_TRUE(canvas_->PopState());
  // should show up as 1.0
  EXPECT_TRUE(canvas_->DrawLine(10., 70., 200., 70., 10., Color(1., 0., 0.)));
  EXPECT_TRUE(canvas_->MultiplyOpacity(.5));
  // should show up as .5
  EXPECT_TRUE(canvas_->DrawLine(10., 90., 200., 90., 10., Color(1., 0., 0.)));
  EXPECT_TRUE(canvas_->MultiplyOpacity(.5));
  // should show up as .25
  EXPECT_TRUE(canvas_->DrawLine(10., 110., 200., 110., 10., Color(1., 0., 0.)));
}

// this test is meaningful only with -savepng
TEST_F(GdiplusCanvasTest, Transformations) {
  // rotation
  EXPECT_TRUE(canvas_->DrawLine(10., 10., 200., 10., 10., Color(0., 1., 0.)));
  EXPECT_TRUE(canvas_->PushState());
  canvas_->RotateCoordinates(M_PI/6);
  EXPECT_TRUE(canvas_->DrawLine(10., 10., 200., 10., 10., Color(0., 1., 0.)));
  EXPECT_TRUE(canvas_->PopState());

  EXPECT_TRUE(canvas_->PushState());
  canvas_->ScaleCoordinates(1.0, -1.0);
  EXPECT_TRUE(canvas_->DrawLine(10., -10., 200., -140., 2., Color(0., 0., 0.)));
  EXPECT_TRUE(canvas_->PopState());

  EXPECT_TRUE(canvas_->MultiplyOpacity(.5));
  EXPECT_TRUE(canvas_->PushState());

  // scale
  EXPECT_TRUE(canvas_->DrawLine(10., 50., 200., 50., 10., Color(1., 0., 0.)));
  canvas_->ScaleCoordinates(1.3, 1.5);
  EXPECT_TRUE(canvas_->DrawLine(10., 50., 200., 50., 10., Color(1., 0., 0.)));
  EXPECT_TRUE(canvas_->PopState());

  // translation
  EXPECT_TRUE(canvas_->DrawLine(10., 110., 200., 110., 10., Color(0., 0., 1.)));
  canvas_->TranslateCoordinates(20., 25.);
  EXPECT_TRUE(canvas_->DrawLine(10., 110., 200., 110., 10., Color(0., 0., 1.)));
}

// this test is meaningful only with -savepng
TEST_F(GdiplusCanvasTest, FillRectAndClipping) {
  EXPECT_TRUE(canvas_->MultiplyOpacity(.5));
  canvas_->RotateCoordinates(.1);
  EXPECT_TRUE(canvas_->PushState());
  EXPECT_TRUE(canvas_->DrawFilledRect(10., 10., 280., 130., Color(1., 0., 0.)));
  EXPECT_TRUE(canvas_->IntersectRectClipRegion(30., 30., 100., 100.));
  EXPECT_TRUE(canvas_->IntersectRectClipRegion(70., 40., 100., 70.));
  EXPECT_TRUE(canvas_->DrawFilledRect(20., 20., 260., 110., Color(0., 1., 0.)));
  EXPECT_TRUE(canvas_->PopState());
  EXPECT_TRUE(canvas_->DrawFilledRect(110., 40., 90., 70., Color(0., 0., 1.)));
}

TEST_F(GdiplusCanvasTest, ClearRect) {
  EXPECT_TRUE(canvas_->DrawFilledRect(10., 10., 280., 130., Color(1., 0., 0.)));
  EXPECT_TRUE(canvas_->ClearCanvas());
  EXPECT_TRUE(canvas_->DrawFilledRect(10., 10., 280., 130., Color(0., 1., 0.)));
  EXPECT_TRUE(canvas_->ClearRect(20., 20., 10., 10.));
  EXPECT_FALSE(canvas_->ClearRect(20., 20., -1., 0));
  canvas_->ScaleCoordinates(2., 2.);
  EXPECT_TRUE(canvas_->ClearRect(20., 20., 10., 10.));
  canvas_->RotateCoordinates(M_PI/8);
  EXPECT_TRUE(canvas_->ClearRect(20., 20., 10., 10.));
}

int main(int argc, char **argv) {
  testing::ParseGTestFlags(&argc, argv);

  for (int i = 0; i < argc; i++) {
    if (0 == strcasecmp(argv[i], "-savepng")) {
      g_savepng = true;
      break;
    }
  }

  return RUN_ALL_TESTS();
}
