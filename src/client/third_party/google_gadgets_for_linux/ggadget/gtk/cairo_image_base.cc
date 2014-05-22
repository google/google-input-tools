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

#include <string>
#include <gdk/gdkcairo.h>
#include <ggadget/logger.h>
#include <ggadget/color.h>
#include <ggadget/signals.h>
#include <ggadget/small_object.h>
#include "cairo_image_base.h"
#include "cairo_graphics.h"
#include "cairo_canvas.h"

namespace ggadget {
namespace gtk {

class CairoImageBase::Impl : public SmallObject<> {
 public:
  Impl(const std::string &tag, bool is_mask)
    : tag_(tag), is_mask_(is_mask) {
  }

  std::string tag_;
  bool is_mask_;
};

CairoImageBase::CairoImageBase(const std::string &tag, bool is_mask)
  : impl_(new Impl(tag, is_mask)) {
}

CairoImageBase::~CairoImageBase() {
  delete impl_;
  impl_ = NULL;
}

void CairoImageBase::Destroy() {
  delete this;
}

void CairoImageBase::Draw(CanvasInterface *canvas, double x, double y) const {
  const CanvasInterface *image = GetCanvas();
  ASSERT(canvas && image);
  if (image && canvas)
    canvas->DrawCanvas(x, y, image);
}

void CairoImageBase::StretchDraw(CanvasInterface *canvas,
                                 double x, double y,
                                 double width, double height) const {
  const CanvasInterface *image = GetCanvas();
  ASSERT(canvas && image);
  if (!canvas || !image)
    return;
  double image_width = image->GetWidth();
  double image_height = image->GetHeight();
  if (image && canvas && image_width > 0 && image_height > 0) {
    double cx = width / image_width;
    double cy = height / image_height;
    if (cx != 1.0 || cy != 1.0) {
      canvas->PushState();
      canvas->ScaleCoordinates(cx, cy);
      canvas->DrawCanvas(x / cx, y / cy, image);
      canvas->PopState();
    } else {
      canvas->DrawCanvas(x, y, image);
    }
  }
}

class ColorMultipliedImage : public CairoImageBase {
 public:
  ColorMultipliedImage(const CairoImageBase *image,
                       const Color &color_multiply)
      : CairoImageBase("", false),
        width_(0), height_(0), fully_opaque_(false),
        color_multiply_(color_multiply), canvas_(NULL) {
    if (image) {
      width_ = image->GetWidth();
      height_ = image->GetHeight();
      fully_opaque_ = image->IsFullyOpaque();
      canvas_ = new CairoCanvas(1, width_, height_, CAIRO_FORMAT_ARGB32);
      image->Draw(canvas_, 0, 0);
      canvas_->MultiplyColor(color_multiply_);
    }
  }

  virtual ~ColorMultipliedImage() {
    DestroyCanvas(canvas_);
  }

  virtual double GetWidth() const { return width_; }
  virtual double GetHeight() const { return height_; }
  virtual bool IsFullyOpaque() const { return fully_opaque_; }
  virtual bool IsValid() const { return canvas_ != NULL; }
  virtual CanvasInterface *GetCanvas() const { return canvas_; }

  double width_;
  double height_;
  bool fully_opaque_;
  Color color_multiply_;
  CairoCanvas *canvas_;
};

ImageInterface *CairoImageBase::MultiplyColor(const Color &color) const {
  // Because this method is const, we should always return a new image even
  // if the color is pure white.
  return new ColorMultipliedImage(this, color);
}

bool CairoImageBase::GetPointValue(double x, double y,
                                   Color *color, double *opacity) const {
  const CanvasInterface *canvas = GetCanvas();
  return canvas && canvas->GetPointValue(x, y, color, opacity);
}

std::string CairoImageBase::GetTag() const {
  return impl_->tag_;
}

} // namespace gtk
} // namespace ggadget
