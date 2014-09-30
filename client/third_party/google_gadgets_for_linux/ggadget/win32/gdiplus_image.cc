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

#include "gdiplus_image.h"

#include <windows.h>
#ifdef GDIPVER
#undef GDIPVER
#endif
#define GDIPVER 0x0110
using std::min;
using std::max;
#include <gdiplus.h>
#include <algorithm>
#include <cmath>
#include <string>
#include <ggadget/scoped_ptr.h>
#include <ggadget/logger.h>
#include <ggadget/color.h>
#include "gdiplus_graphics.h"
#include "gdiplus_canvas.h"

namespace ggadget {
namespace win32 {

namespace {

// Cast double to int with rounding.
inline int D2I(double d) {
  return static_cast<int>(d > 0 ? d + 0.5: d - 0.5);
}

}  // namespace

class GdiplusImage::Impl {
 public:
  Impl() : is_mask_(false), fully_opaque_(false) {
  }

  bool Init(const std::string& tag, const std::string& data, bool is_mask) {
    is_mask_ = is_mask;
    tag_ = tag;
    fully_opaque_ = false;
    canvas_.reset(new GdiplusCanvas);
    if (!canvas_->Init(data, false))
      return false;
    Gdiplus::Bitmap* image = canvas_->GetImage();
    if (is_mask) {
      scoped_ptr<GdiplusCanvas> mask_canvas(new GdiplusCanvas);
      mask_canvas->Init(NULL,  // We don't need graphic interface in image.
                        canvas_->GetWidth(), canvas_->GetHeight(),
                        false);  // No need to create Gdiplus::Graphics.
      // Create a color remap attribute that remap black to transparent.
      Gdiplus::ImageAttributes remap_attributes;
      Gdiplus::ColorMap remap;
      remap.oldColor = Gdiplus::Color(255, 0, 0, 0);
      remap.newColor = Gdiplus::Color(0, 0, 0, 0);
      remap_attributes.SetRemapTable(1, &remap);

      scoped_ptr<Gdiplus::Graphics> mask_graphics(
          Gdiplus::Graphics::FromImage(mask_canvas->GetImage()));
      Gdiplus::Rect image_rect(0, 0, image->GetWidth(),
                               image->GetHeight());
      mask_graphics->DrawImage(
          image, image_rect,
          0, 0, image->GetWidth(), image->GetHeight(),
          Gdiplus::UnitPixel, &remap_attributes);
      canvas_.reset(mask_canvas.release());
    } else if (!Gdiplus::IsAlphaPixelFormat(image->GetPixelFormat())) {
      fully_opaque_ = true;
    } else {
      fully_opaque_ = true;
      Gdiplus::Color color;
      for (size_t y = 0; y < image->GetHeight() && fully_opaque_; y++) {
        for (size_t x = 0; x < image->GetWidth(); ++x) {
          image->GetPixel(x, y, &color);
          if (color.GetAlpha() != 255) {
            fully_opaque_ = false;
            break;
          }
        }
      }
    }
    return true;
  }

  bool Init(double width, double height) {
    is_mask_ = false;
    canvas_.reset(new GdiplusCanvas);
    if (canvas_.get() == NULL) return false;
    return canvas_->Init(NULL, width, height, false);
  }

  ~Impl() {
  }

  bool is_mask_;
  scoped_ptr<GdiplusCanvas> canvas_;
  std::string tag_;
  bool fully_opaque_;
};

GdiplusImage::GdiplusImage() {
}

GdiplusImage::~GdiplusImage() {
}

bool GdiplusImage::Init(int width, int height) {
  impl_.reset(new Impl);
  if (impl_.get() == NULL) return false;
  return impl_->Init(width, height);
}

bool GdiplusImage::Init(const std::string& tag, const std::string& data,
                        bool is_mask) {
  impl_.reset(new Impl);
  if (impl_.get() == NULL) return false;
  return impl_->Init(tag, data, is_mask);
}

void GdiplusImage::Destroy() {
  delete this;
}

const CanvasInterface* GdiplusImage::GetCanvas() const {
  return impl_->canvas_.get();
}

void GdiplusImage::Draw(CanvasInterface* canvas, double x, double y) const {
  canvas->DrawCanvas(x, y, impl_->canvas_.get());
}

void GdiplusImage::StretchDraw(CanvasInterface* canvas,
                               double x, double y,
                               double width, double height) const {
  const GdiplusCanvas* src_canvas = impl_->canvas_.get();
  double cx = width / src_canvas->GetWidth();
  double cy = height / src_canvas->GetHeight();
  if (cx != 1.0 || cy != 1.0) {
    canvas->PushState();
    canvas->ScaleCoordinates(cx, cy);
    canvas->DrawCanvas(x / cx, y / cy, src_canvas);
    canvas->PopState();
  } else {
    Draw(canvas, x, y);
  }
}

double GdiplusImage::GetWidth() const {
  return impl_->canvas_->GetWidth();
}

double GdiplusImage::GetHeight() const {
  return impl_->canvas_->GetHeight();
}

ImageInterface* GdiplusImage::MultiplyColor(const Color &color) const {
  // Create a color matrix that multiply the rgb channel.
  Gdiplus::ColorMatrix color_transform = {
    static_cast<Gdiplus::REAL>(color.red * 2), 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, static_cast<Gdiplus::REAL>(color.green * 2), 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, static_cast<Gdiplus::REAL>(color.blue * 2), 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 1.0f};
  Gdiplus::ImageAttributes image_attribute;
  image_attribute.SetColorMatrix(&color_transform);
  scoped_ptr<GdiplusImage> multiplied_image(new GdiplusImage);
  multiplied_image->Init(D2I(GetWidth()), D2I(GetHeight()));
  scoped_ptr<Gdiplus::Graphics> multiplied_graphics(
      Gdiplus::Graphics::FromImage(
          multiplied_image->impl_->canvas_->GetImage()));
  Gdiplus::Rect image_rect(0, 0, D2I(GetWidth()), D2I(GetHeight()));
  multiplied_graphics->DrawImage(
      impl_->canvas_->GetImage(), image_rect,
      0, 0, D2I(GetWidth()), D2I(GetHeight()), Gdiplus::UnitPixel,
      &image_attribute);
  return multiplied_image.release();
}

bool GdiplusImage::GetPointValue(double x, double y, Color* color,
                                 double* opacity) const {
  return impl_->canvas_->GetPointValue(x, y, color, opacity);
}

std::string GdiplusImage::GetTag() const {
  return impl_->tag_;
}

bool GdiplusImage::IsFullyOpaque() const {
  return impl_->fully_opaque_;
}

bool GdiplusImage::IsValid() const {
  return impl_.get() != NULL && impl_->canvas_ != NULL;
}

}  // namespace win32
}  // namespace ggadget
