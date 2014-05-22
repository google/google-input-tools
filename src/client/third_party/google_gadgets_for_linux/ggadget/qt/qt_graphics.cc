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

#include <ggadget/color.h>
#include <ggadget/common.h>
#include <ggadget/logger.h>
#include "qt_graphics.h"
#include "qt_canvas.h"
#include "qt_image.h"
#include "qt_font.h"

namespace ggadget {
namespace qt {

class QtGraphics::Impl {
 public:
  Impl(double zoom) : zoom_(zoom) {
    if (zoom_ <= 0) zoom_ = 1;
  }

  double zoom_;
  Signal1<void, double> on_zoom_signal_;
};

QtGraphics::QtGraphics(double zoom) : impl_(new Impl(zoom)) {
}

QtGraphics::~QtGraphics() {
  delete impl_;
  impl_ = NULL;
}

double QtGraphics::GetZoom() const {
  return impl_->zoom_;
}

void QtGraphics::SetZoom(double zoom) {
  if (impl_->zoom_ != zoom) {
    impl_->zoom_ = (zoom > 0 ? zoom : 1);
    impl_->on_zoom_signal_(impl_->zoom_);
  }
}

Connection *QtGraphics::ConnectOnZoom(Slot1<void, double> *slot) const {
  return impl_->on_zoom_signal_.Connect(slot);
}

CanvasInterface *QtGraphics::NewCanvas(double w, double h) const {
  if (!w || !h) return NULL;

  QtCanvas *canvas = new QtCanvas(this, w, h, true);
  if (!canvas) return NULL;
  if (!canvas->IsValid()) {
    delete canvas;
    canvas = NULL;
  }
  return canvas;
}

ImageInterface *QtGraphics::NewImage(const std::string &tag,
                                     const std::string &data,
                                     bool is_mask) const {
  if (data.empty()) return NULL;

  QtImage *img = new QtImage(NULL, tag, data, is_mask);
  if (!img) return NULL;
  if (!img->IsValid()) {
    img->Destroy();
    img = NULL;
  }
  return img;
}

FontInterface *QtGraphics::NewFont(const std::string &family,
                                   double pt_size,
                                   FontInterface::Style style,
                                   FontInterface::Weight weight) const {
  return new QtFont(family, pt_size, style, weight);
}

TextRendererInterface *QtGraphics::NewTextRenderer() const {
  return NULL;
}

} // namespace qt
} // namespace ggadget
