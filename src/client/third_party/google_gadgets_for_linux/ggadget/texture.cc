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

#include "texture.h"

#include <cmath>
#include "canvas_interface.h"
#include "graphics_interface.h"
#include "image_interface.h"
#include "logger.h"
#include "file_manager_interface.h"
#include "string_utils.h"
#include "view.h"
#include "small_object.h"

namespace ggadget {

class Texture::Impl : public SmallObject<> {
 public:
  Impl(ImageInterface *image)
    : color_(.0, .0, .0),
      opacity_(1.0),
      image_(image) {
    name_ = image_ ? image_->GetTag() : "";
  }

  Impl(const Color &color, double opacity)
    : color_(color),
      opacity_(opacity),
      image_(NULL),
      name_(opacity == 1.0 ?
            StringPrintf("#%02X%02X%02X",
                         static_cast<int>(round(color.red * 255)),
                         static_cast<int>(round(color.green * 255)),
                         static_cast<int>(round(color.blue * 255))) :
            StringPrintf("#%02X%02X%02X%02X",
                         static_cast<int>(round(opacity * 255)),
                         static_cast<int>(round(color.red * 255)),
                         static_cast<int>(round(color.green * 255)),
                         static_cast<int>(round(color.blue * 255)))) {
  }

  ~Impl() {
    if (image_) image_->Destroy();
    image_ = NULL;
  }

  void Draw(CanvasInterface *canvas, double x, double y,
            double width, double height) const {
    if (image_) {
      const CanvasInterface *image_canvas = image_->GetCanvas();
      if (image_canvas) {
        // Don't apply opacity_ here because it is only applicable with color_.
        canvas->DrawFilledRectWithCanvas(x, y, width, height, image_canvas);
      }
    } else if (opacity_ > 0) {
      if (opacity_ != 1.0) {
        canvas->PushState();
        canvas->MultiplyOpacity(opacity_);
      }
      canvas->DrawFilledRect(x, y, width, height, color_);
      if (opacity_ != 1.0)
        canvas->PopState();
    }
  }

  void DrawText(CanvasInterface *canvas, double x, double y,
                double width, double height, const char *text,
                const FontInterface *f,
                CanvasInterface::Alignment align,
                CanvasInterface::VAlignment valign,
                CanvasInterface::Trimming trimming,
                int text_flags) const {
    if (image_) {
      const CanvasInterface *image_canvas = image_->GetCanvas();
      if (image_canvas) {
        // Don't apply opacity_ here because it is only applicable with color_.
        canvas->DrawTextWithTexture(x, y, width, height, text, f, image_canvas,
                                    align, valign, trimming, text_flags);
      }
    } else if (opacity_ > 0) {
      if (opacity_ != 1.0) {
        canvas->PushState();
        canvas->MultiplyOpacity(opacity_);
      }
      canvas->DrawText(x, y, width, height, text, f, color_,
                       align, valign, trimming, text_flags);
      if (opacity_ != 1.0)
        canvas->PopState();
    }
  }

  void DrawFormattedText(CanvasInterface *canvas,
                         TextRendererInterface *renderer) {
    if (image_) {
      const CanvasInterface *image_canvas = image_->GetCanvas();
      if (image_canvas) {
        // Don't apply opacity_ here because it is only applicable with color_.
        renderer->DrawTextWithTexture(image_canvas, canvas);
        return;
      }
    }
    renderer->DrawText(canvas);
  }

  bool IsFullyOpaque() {
    if (image_)
      return image_->IsFullyOpaque();
    return opacity_ == 1.0;
  }

  Color color_;
  double opacity_;
  ImageInterface *image_;
  std::string name_;
};

Texture::Texture(ImageInterface *image)
    : impl_(new Impl(image)) {
}

Texture::Texture(const Color &color, double opacity)
    : impl_(new Impl(color, opacity)) {
}

Texture::~Texture() {
  delete impl_;
  impl_ = NULL;
}

void Texture::Draw(CanvasInterface *canvas, double x, double y,
                   double width, double height) const {
  ASSERT(canvas);
  impl_->Draw(canvas, x, y, width, height);
}

void Texture::DrawText(CanvasInterface *canvas, double x, double y,
                       double width, double height, const char *text,
                       const FontInterface *f, CanvasInterface::Alignment align,
                       CanvasInterface::VAlignment valign,
                       CanvasInterface::Trimming trimming,
                       int text_flags) const {
  ASSERT(canvas);
  impl_->DrawText(canvas, x, y, width, height, text, f,
                  align, valign, trimming, text_flags);
}

std::string Texture::GetSrc() const {
  return impl_->name_;
}

const ImageInterface *Texture::GetImage() const {
  return impl_->image_;
}

bool Texture::IsFullyOpaque() const {
  return impl_->IsFullyOpaque();
}

void Texture::DrawText(CanvasInterface *canvas,
                       TextRendererInterface *renderer) const {
  impl_->DrawFormattedText(canvas, renderer);
}

} // namespace ggadget
