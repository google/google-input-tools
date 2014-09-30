/*
  Copyright 2012 Google Inc.

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

// This file contains the implementation of class QuartzGraphics

#include "ggadget/mac/quartz_graphics.h"

#include "ggadget/mac/ct_font.h"
#include "ggadget/mac/ct_text_renderer.h"
#include "ggadget/mac/quartz_canvas.h"
#include "ggadget/mac/quartz_image.h"
#include "ggadget/signals.h"
#include "ggadget/text_formats.h"

namespace ggadget {
namespace mac {

class QuartzGraphics::Impl {
 public:
  explicit Impl(double zoom)
      : zoom_(zoom) {
    ASSERT_M(zoom > 0, ("zoom = %lf must be positive\n", zoom));
  }

  double zoom_;
  Signal1<void, double> on_zoom_signal_;
};

QuartzGraphics::QuartzGraphics(double zoom) {
  impl_.reset(new Impl(zoom));
  DLOG("New QuartzGraphics: %p", this);
}

QuartzGraphics::~QuartzGraphics() {
}

CanvasInterface* QuartzGraphics::NewCanvas(double w, double h) const {
  if (w <= 0 || h <= 0) return NULL;
  scoped_ptr<QuartzCanvas> canvas(new QuartzCanvas);
  if (canvas.get() == NULL) return NULL;
  if (!canvas->Init(this, w, h, CanvasInterface::RAWIMAGE_FORMAT_ARGB32))
    return NULL;
  if (!canvas->IsValid()) return NULL;
  return canvas.release();
}

ImageInterface* QuartzGraphics::NewImage(const std::string& tag,
                                         const std::string& data,
                                         bool is_mask) const {
  scoped_ptr<QuartzImage> image(new QuartzImage);
  if (!image->Init(tag, data, is_mask))
    return NULL;
  return image.release();
}

FontInterface* QuartzGraphics::NewFont(const std::string &family,
                                       double pt_size,
                                       FontInterface::Style style,
                                       FontInterface::Weight weight) const {
  scoped_ptr<CtFont> font(new CtFont);
  if (!font->Init(family, pt_size, style, weight))
    return NULL;
  return font.release();
}

FontInterface* QuartzGraphics::NewFont(const TextFormat &format) const {
  return NewFont(format.font(), format.size(),
                 format.italic()
                   ? FontInterface::STYLE_ITALIC
                   : FontInterface::STYLE_NORMAL,
                 format.bold()
                   ? FontInterface::WEIGHT_BOLD
                   : FontInterface::WEIGHT_NORMAL);
}

TextRendererInterface* QuartzGraphics::NewTextRenderer() const {
  return new CtTextRenderer(this);
}

double QuartzGraphics::GetZoom() const {
  return impl_->zoom_;
}

void QuartzGraphics::SetZoom(double zoom) {
  ASSERT_M(zoom > 0, ("zoom = %lf must be positive\n", zoom));
  if (impl_->zoom_ != zoom) {
    impl_->zoom_ = zoom;
    impl_->on_zoom_signal_(impl_->zoom_);
  }
}

Connection* QuartzGraphics::ConnectOnZoom(Slot1<void, double>* slot) const {
  return impl_->on_zoom_signal_.Connect(slot);
}

} // namespace mac
} // namespace ggadget
