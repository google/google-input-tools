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

#include <cstdio>
#include <cairo.h>
#include <strings.h>

#include "ggadget/common.h"
#include "ggadget/color.h"
#include "ggadget/canvas_interface.h"
#include "ggadget/gtk/cairo_canvas.h"
#include "ggadget/gtk/cairo_graphics.h"
#include "unittest/gtest.h"

using namespace ggadget;
using namespace ggadget::gtk;

const double kPi = 3.141592653589793;
bool g_savepng = false;

// fixture for creating the CairoCanvas object
class CairoCanvasTest : public testing::Test {
 protected:
  CairoGraphics gfx_;
  CairoCanvas *canvas_;

  CairoCanvasTest() : gfx_(1.0) {
    canvas_ = down_cast<CairoCanvas*>(gfx_.NewCanvas(300, 150));
  }

  ~CairoCanvasTest() {
    if (g_savepng) {
      const testing::TestInfo *const test_info =
        testing::UnitTest::GetInstance()->current_test_info();
      char file[100];
      snprintf(file, arraysize(file), "%s.png", test_info->name());
      cairo_surface_write_to_png(canvas_->GetSurface(), file);
    }

    canvas_->Destroy();
  }
};

TEST_F(CairoCanvasTest, PushPopStateReturnValues) {
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

TEST_F(CairoCanvasTest, OpacityReturnValues) {
  EXPECT_FALSE(canvas_->MultiplyOpacity(1.7));
  EXPECT_TRUE(canvas_->MultiplyOpacity(.5));
  EXPECT_FALSE(canvas_->MultiplyOpacity(-.7));
  EXPECT_TRUE(canvas_->MultiplyOpacity(.7));
  EXPECT_FALSE(canvas_->MultiplyOpacity(1000.));
  EXPECT_TRUE(canvas_->MultiplyOpacity(.2));
}

TEST_F(CairoCanvasTest, DrawLines) {
  EXPECT_FALSE(canvas_->DrawLine(10., 10., 200., 20., -1., Color(1., 0., 0.)));
  EXPECT_TRUE(canvas_->DrawLine(10., 10., 200., 20., 1., Color(1., 0., 0.)));
  EXPECT_TRUE(canvas_->DrawLine(10., 30., 200., 30., 2., Color(0., 1., 0.)));
  EXPECT_TRUE(canvas_->DrawLine(10., 40., 200., 40., 1.5, Color(0., 0., 1.)));
  EXPECT_TRUE(canvas_->DrawLine(10., 50., 200., 50., 1., Color(0., 0., 0.)));
  EXPECT_TRUE(canvas_->DrawLine(10., 60., 200., 60., 4., Color(1., 1., 1.)));
}

TEST_F(CairoCanvasTest, DrawRectReturnValues) {
  EXPECT_FALSE(canvas_->DrawFilledRect(5., 6., -1., 5., Color(0., 0., 0.)));
  EXPECT_TRUE(canvas_->DrawFilledRect(5., 6., 1., 5., Color(0., 0., 0.)));
  EXPECT_FALSE(canvas_->DrawFilledRect(5., 6., 1., -5., Color(0., 0., 0.)));
}

TEST_F(CairoCanvasTest, ClipRectReturnValues) {
  EXPECT_FALSE(canvas_->IntersectRectClipRegion(5., 6., -1., 5.));
  EXPECT_TRUE(canvas_->IntersectRectClipRegion(5., 6., 1., 5.));
  EXPECT_FALSE(canvas_->IntersectRectClipRegion(5., 6., 1., -5.));
}

// this test is meaningful only with -savepng
TEST_F(CairoCanvasTest, PushPopStateLines) {
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
TEST_F(CairoCanvasTest, Transformations) {
  // rotation
  EXPECT_TRUE(canvas_->DrawLine(10., 10., 200., 10., 10., Color(0., 1., 0.)));
  EXPECT_TRUE(canvas_->PushState());
  canvas_->RotateCoordinates(kPi/6);
  EXPECT_TRUE(canvas_->DrawLine(10., 10., 200., 10., 10., Color(0., 1., 0.)));
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
TEST_F(CairoCanvasTest, FillRectAndClipping) {
  EXPECT_TRUE(canvas_->MultiplyOpacity(.5));
  EXPECT_TRUE(canvas_->PushState());
  EXPECT_TRUE(canvas_->DrawFilledRect(10., 10., 280., 130., Color(1., 0., 0.)));
  EXPECT_TRUE(canvas_->IntersectRectClipRegion(30., 30., 100., 100.));
  EXPECT_TRUE(canvas_->IntersectRectClipRegion(70., 40., 100., 70.));
  EXPECT_TRUE(canvas_->DrawFilledRect(20., 20., 260., 110., Color(0., 1., 0.)));
  EXPECT_TRUE(canvas_->PopState());
  EXPECT_TRUE(canvas_->DrawFilledRect(110., 40., 90., 70., Color(0., 0., 1.)));
}

#if CAIRO_VERSION >= CAIRO_VERSION_ENCODE(1,2,0)
TEST_F(CairoCanvasTest, GetPointValue) {
  Color color;
  double opacity;

  EXPECT_TRUE(canvas_->MultiplyOpacity(0.5));
  EXPECT_TRUE(canvas_->DrawFilledRect(10., 50., 280., 100., Color(0.8, 0., 0.)));

  EXPECT_TRUE(canvas_->GetPointValue(10, 70, &color, &opacity));
  EXPECT_NEAR(0.5, opacity, (1/256.0));
  EXPECT_NEAR(0.8, color.red, (1/256.0));
  EXPECT_DOUBLE_EQ(0, color.green);
  EXPECT_DOUBLE_EQ(0, color.blue);
  EXPECT_TRUE(canvas_->GetPointValue(70, 10, &color, &opacity));
  EXPECT_DOUBLE_EQ(0, opacity);
  EXPECT_DOUBLE_EQ(0, color.red);
  EXPECT_DOUBLE_EQ(0, color.green);
  EXPECT_DOUBLE_EQ(0, color.blue);
  EXPECT_FALSE(canvas_->GetPointValue(310, 20, &color, &opacity));
  EXPECT_FALSE(canvas_->GetPointValue(20, -2, &color, &opacity));
}
#endif

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
