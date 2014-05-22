/*
  Copyright 2013 Google Inc.

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

#include "ggadget/mac/quartz_image.h"

#import <AppKit/AppKit.h>
#include <algorithm>

#include "ggadget/mac/quartz_canvas.h"
#include "ggadget/mac/scoped_cftyperef.h"

namespace {

bool ImageHasAlpha(CGImageRef image) {
  CGImageAlphaInfo alpha_info = CGImageGetAlphaInfo(image);
  return alpha_info != kCGImageAlphaNone &&
      alpha_info != kCGImageAlphaNoneSkipLast &&
      alpha_info != kCGImageAlphaNoneSkipFirst;
}

const CGFloat* CreateMaskingComponents(CGColorSpaceRef color_space) {
  // We only support RGB color space for now.
  ASSERT(CGColorSpaceGetModel(color_space) == kCGColorSpaceModelRGB);
  const static CGFloat rgb_components[] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
  return rgb_components;
}

} // namespace

namespace ggadget{
namespace mac{

class QuartzImage::Impl {
 public:
  Impl() : is_mask_(false), fully_opaque_(false) {
  }

  bool Init(const std::string& tag, const std::string& data, bool is_mask) {
    is_mask_ = is_mask;
    tag_ = tag;
    canvas_.reset(new QuartzCanvas);

    // Load image data
    NSData* imageData =
        [NSData dataWithBytesNoCopy:const_cast<char*>(data.c_str())
                             length:data.size()
                       freeWhenDone:NO];
    if (imageData == nil) {
      return false;
    }
    NSBitmapImageRep* bitmapRep =
        [[NSBitmapImageRep alloc] initWithData:imageData];
    if (bitmapRep == nil) {
      return false;
    }

    ScopedCFTypeRef<CGImageRef> image([bitmapRep CGImage],
                                      ScopedCFTypeRef<CGImageRef>::RETAIN);
    if (!image.get()) {
      return false;
    }
    ScopedCFTypeRef<CGColorSpaceRef> color_space(
        CGImageGetColorSpace(image), ScopedCFTypeRef<CGColorSpaceRef>::RETAIN);
    size_t w = CGImageGetWidth(image.get());
    size_t h = CGImageGetHeight(image.get());

    // Create canvas
    width_ = w;
    height_ = h;
    canvas_->Init(NULL, width_, height_,
                  (is_mask || ImageHasAlpha(image.get()))
                  ? CanvasInterface::RAWIMAGE_FORMAT_ARGB32
                  : CanvasInterface::RAWIMAGE_FORMAT_RGB24);

    if (is_mask) {
      // create new mask image with original image data but black color be
      // fully transparent.
      const CGFloat* components = CreateMaskingComponents(color_space.get());
      image.reset(CGImageCreateWithMaskingColors(image.get(), components));
    }

    // Draw canvas with image. Keep in mind we moved canvas origin to upper-left
    // and flipped upside-down while Quartz draws images with origin to
    // lower-left with y axis grows upwards.
    canvas_->ScaleCoordinates(1.0, -1.0);
    CGContextDrawImage(canvas_->GetContext(),
                       CGRectMake(0.0, -height_, width_, height_),
                       image.get());
    canvas_->ScaleCoordinates(1.0, -1.0);
    if (is_mask) {
      fully_opaque_ = false;
    } else if (!ImageHasAlpha(image.get())) {
      fully_opaque_ = true;
    } else {
      fully_opaque_ = true;
      for (size_t y = 0; y < h && fully_opaque_; ++y) {
        for (size_t x = 0; x < w; ++x) {
          double opacity;
          canvas_->GetPointValue(x, y, /* color */ NULL, &opacity);
          if (opacity < 1.0) {
            fully_opaque_ = false;
            break;
          }
        } // for x
      } // for y
    }
    return true;
  }

  bool Init(double width, double height) {
    is_mask_ = false;
    fully_opaque_ = false;
    canvas_.reset(new QuartzCanvas);
    if (!canvas_.get()) {
      return false;
    }
    width_ = width;
    height_ = height;
    return canvas_->Init(NULL, width, height,
                         CanvasInterface::RAWIMAGE_FORMAT_ARGB32);
  }

  bool is_mask_;
  bool fully_opaque_;
  std::string tag_;
  double width_;
  double height_;
  scoped_ptr<QuartzCanvas> canvas_;
};

QuartzImage::QuartzImage() : impl_(new Impl) {
}

bool QuartzImage::Init(const std::string& tag,
                       const std::string& data,
                       bool is_mask) {
  return impl_->Init(tag, data, is_mask);
}

bool QuartzImage::Init(int width, int height) {
  return impl_->Init(width, height);
}

bool QuartzImage::IsValid() const {
  return impl_->canvas_.get() != NULL;
}

QuartzImage::~QuartzImage() {
}

void QuartzImage::Destroy() {
  delete this;
}

const CanvasInterface *QuartzImage::GetCanvas() const {
  return impl_->canvas_.get();
}

void QuartzImage::Draw(CanvasInterface *canvas, double x, double y) const {
  ASSERT(canvas);
  canvas->DrawCanvas(x, y, GetCanvas());
}

void QuartzImage::StretchDraw(CanvasInterface *canvas,
                              double x, double y,
                              double width, double height) const {
  ASSERT(canvas);
  const CanvasInterface* src_canvas = GetCanvas();
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

double QuartzImage::GetWidth() const {
  return impl_->width_;
}

double QuartzImage::GetHeight() const {
  return impl_->height_;
}

ImageInterface *QuartzImage::MultiplyColor(const Color &color) const {
  scoped_ptr<QuartzImage> multiplied(new QuartzImage);
  multiplied->Init(GetWidth(), GetHeight());
  QuartzCanvas* canvas = multiplied->impl_->canvas_.get();

  if (color == Color::kMiddleColor) {
    return multiplied.release();
  }

  // Draw original image as background.
  canvas->DrawCanvas(0.0, 0.0, GetCanvas());

  // Multiply all pixels with |color|.
  CGContextRef context = canvas->GetContext();
  unsigned char* data =
    static_cast<unsigned char*>(CGBitmapContextGetData(context));
  if (!data) {
    return false;
  }

  size_t bytes_per_pixel = CGBitmapContextGetBitsPerPixel(context) / 8;
  size_t bytes_per_row = CGBitmapContextGetBytesPerRow(context);
  size_t height = CGBitmapContextGetHeight(context);
  size_t width = CGBitmapContextGetWidth(context);
  unsigned int color_components[] = {
    0,    // alpha
    static_cast<unsigned int>(color.red * 512),
    static_cast<unsigned int>(color.green * 512),
    static_cast<unsigned int>(color.blue * 512),
  };
  for (size_t y = 0; y < height; ++y) {
    size_t index = y * bytes_per_row;
    for (size_t x = 0; x < width; ++x) {
      index += bytes_per_pixel;
      unsigned int alpha = data[index];
      for (size_t i = 1; i <= 3; ++i) {
        // Pixels are pre-multiplied, so max value is alpha.
        data[index + i] = std::min((data[index + i] * color_components[i]) >> 8,
                                   alpha);
      }
    }
  }

  return multiplied.release();
}

bool QuartzImage::GetPointValue(double x, double y,
                                Color *color, double *opacity) const {
  return impl_->canvas_->GetPointValue(x, y, color, opacity);
}

std::string QuartzImage::GetTag() const {
  return impl_->tag_;
}

bool QuartzImage::IsFullyOpaque() const {
  return impl_->fully_opaque_;
}

} // namespace mac
} // namespace ggadget
