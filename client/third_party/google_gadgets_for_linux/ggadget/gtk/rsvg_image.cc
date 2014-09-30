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

#include <cmath>
#include <string>
#include <librsvg/rsvg.h>
#include <librsvg/rsvg-cairo.h>
#include <ggadget/color.h>
#include <ggadget/logger.h>
#include <ggadget/signals.h>
#include <ggadget/small_object.h>
#include "cairo_graphics.h"
#include "cairo_canvas.h"
#include "rsvg_image.h"

namespace ggadget {
namespace gtk {

class RsvgImage::Impl : public SmallObject<> {
 public:
  Impl(const CairoGraphics *graphics, const std::string &data)
      : width_(0), height_(0), rsvg_(NULL), canvas_(NULL),
        zoom_(graphics->GetZoom()), on_zoom_connection_(NULL) {
    GError *error = NULL;
    const guint8 *ptr = reinterpret_cast<const guint8*>(data.c_str());
    rsvg_ = rsvg_handle_new_from_data(ptr, data.size(), &error);
    if (error)
      g_error_free(error);
    if (rsvg_) {
      RsvgDimensionData dim;
      rsvg_handle_get_dimensions(rsvg_, &dim);
      width_ = dim.width;
      height_ = dim.height;
      on_zoom_connection_ =
        graphics->ConnectOnZoom(NewSlot(this, &Impl::OnZoom));
    }
  }

  ~Impl() {
    if (rsvg_)
      g_object_unref(rsvg_);
    if (on_zoom_connection_)
      on_zoom_connection_->Disconnect();
    DestroyCanvas(canvas_);
  }

  void OnZoom(double zoom) {
    if (zoom_ != zoom && zoom > 0) {
      zoom_ = zoom;
      // Destroy the canvas so that it'll be recreated again with new zoom
      // factor when calling GetCanvas().
      DestroyCanvas(canvas_);
      canvas_ = NULL;
    } else if (zoom < 0) {
      // if zoom < 0 then means the graphics has been destroyed, then change
      // the zoom level back to 1 and remove the connection to graphics.
      if (zoom_ != 1) {
        DestroyCanvas(canvas_);
        canvas_ = NULL;
      }
      zoom_ = 1;
      on_zoom_connection_ = NULL;
    }
  }

  double width_;
  double height_;
  RsvgHandle *rsvg_;
  CairoCanvas *canvas_;
  double zoom_;
  Connection *on_zoom_connection_;
};

RsvgImage::RsvgImage(const CairoGraphics *graphics, const std::string &tag,
                     const std::string &data, bool is_mask)
    : CairoImageBase(tag, is_mask),
      impl_(new Impl(graphics, data)) {
  // RsvgImage doesn't support mask for now.
  ASSERT(!is_mask);
}

RsvgImage::~RsvgImage() {
  delete impl_;
  impl_ = NULL;
}

bool RsvgImage::IsValid() const {
  return impl_->rsvg_ != NULL;
}

CanvasInterface *RsvgImage::GetCanvas() const {
  if (!impl_->canvas_ && impl_->rsvg_) {
    impl_->canvas_ = new CairoCanvas(impl_->zoom_,
                                     impl_->width_, impl_->height_,
                                     CAIRO_FORMAT_ARGB32);
    // Draw the image onto the canvas.
    cairo_t *cr = impl_->canvas_->GetContext();
    rsvg_handle_render_cairo(impl_->rsvg_, cr);
  }
  return impl_->canvas_;
}

void RsvgImage::StretchDraw(CanvasInterface *canvas,
                            double x, double y,
                            double width, double height) const {
  ASSERT(canvas);
  if (canvas && impl_->rsvg_) {
    // If no stretch, use cached canvas to improve performance.
    // Otherwise draw rsvg directly onto the canvas to get better effect.
    if (width == impl_->width_ && height == impl_->height_) {
      const CanvasInterface *image = GetCanvas();
      ASSERT(image);
      if (image)
        canvas->DrawCanvas(x, y, image);
    } else {
      double cx = width / impl_->width_;
      double cy = height / impl_->height_;
      canvas->PushState();
      canvas->IntersectRectClipRegion(x, y, width, height);
      canvas->TranslateCoordinates(x, y);
      canvas->ScaleCoordinates(cx, cy);
      CairoCanvas *cc = down_cast<CairoCanvas*>(canvas);
      rsvg_handle_render_cairo(impl_->rsvg_, cc->GetContext());
      canvas->PopState();
    }
  }
}

double RsvgImage::GetWidth() const {
  return impl_->width_;
}

double RsvgImage::GetHeight() const {
  return impl_->height_;
}

bool RsvgImage::IsFullyOpaque() const {
  // Always treats that a rsvg image is not fully opaque.
  return false;
}

} // namespace gtk
} // namespace ggadget
