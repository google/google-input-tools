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

#include "ggadget/mac/quartz_canvas.h"

#include <stack>

#include "ggadget/clip_region.h"
#include "ggadget/mac/ct_font.h"
#include "ggadget/mac/scoped_cftyperef.h"
#include "ggadget/signals.h"

namespace {
const size_t kBitsPerComponent = 8;
const size_t kBitsPerPixel = 32;

struct QuartzPixelFormat {
  ggadget::mac::ScopedCFTypeRef<CGColorSpaceRef> colorSpace;
  CGImageAlphaInfo contextAlphaInfo;
  CGBitmapInfo bitmapInfo;

  QuartzPixelFormat()
      : bitmapInfo(0),
        colorSpace(CGColorSpaceCreateWithName(kCGColorSpaceGenericRGB)) {
  }

  bool Init(ggadget::CanvasInterface::RawImageFormat rawImageFormat) {
    bitmapInfo = kCGBitmapByteOrder32Big;
    switch (rawImageFormat) {
    case ggadget::CanvasInterface::RAWIMAGE_FORMAT_ARGB32:
      // We need to use premultiplied alpha for context
      contextAlphaInfo = kCGImageAlphaPremultipliedFirst;
      // We use premultiplied alpha for image data
      bitmapInfo |= kCGImageAlphaPremultipliedFirst;
      break;
    case ggadget::CanvasInterface::RAWIMAGE_FORMAT_RGB24:
      contextAlphaInfo = kCGImageAlphaNoneSkipFirst;
      bitmapInfo |= kCGImageAlphaNoneSkipFirst;
      break;
    default:
      return false;
    }
    return true;
  }
};

struct PatternInfo {
  double width;
  double height;
  ggadget::mac::ScopedCFTypeRef<CGImageRef> image;

  PatternInfo(double w, double h,
              const ggadget::mac::ScopedCFTypeRef<CGImageRef> &img)
      : width(w),
        height(h),
        image(img) {
  }
};

} // namespace

namespace ggadget {
namespace mac {

class QuartzCanvas::Impl {
 public:
  Impl() : context_(NULL),
           width_(0.0f),
           height_(0.0f),
           zoom_(1.0f),
           opacity_(1.0f) {
  }

  ~Impl() {
    on_zoom_connection_->Disconnect();
  }

  bool Init(const QuartzGraphics *graphics, double w, double h,
            RawImageFormat rawImageFormat) {
    width_ = w;
    height_ = h;
    if (graphics != NULL) {
      zoom_ = graphics->GetZoom();
      on_zoom_connection_ =
          graphics->ConnectOnZoom(NewSlot(this, &Impl::OnZoom));
    }
    rawImageFormat_ = rawImageFormat;
    context_.reset(CreateContext(zoom_, w, h, rawImageFormat));
    return context_.get() != NULL;
  }

  bool Init(const QuartzGraphics *graphics, double w, double h,
            CGContextRef context) {
    if (!context || !graphics) {
      return false;
    }
    context_.reset(context, ScopedCFTypeRef<CGContextRef>::RETAIN);
    width_ = w;
    height_ = h;
    if (graphics != NULL) {
      zoom_ = graphics->GetZoom();
      on_zoom_connection_ =
          graphics->ConnectOnZoom(NewSlot(this, &Impl::OnZoom));
    }
    InitContext(zoom_, h * zoom_, context);
    return true;
  }

  static CGContextRef CreateContext(double zoom, double w, double h,
                                    RawImageFormat rawImageFormat) {
    ASSERT(w > 0);
    ASSERT(h > 0);
    ASSERT(zoom > 0);

    size_t width = static_cast<size_t>(ceil(w * zoom));
    size_t height = static_cast<size_t>(ceil(h * zoom));

    QuartzPixelFormat pixelFormat;
    if (!pixelFormat.Init(rawImageFormat)) {
      return NULL;
    }

    ScopedCFTypeRef<CGContextRef> context(CGBitmapContextCreate(
        NULL /* data */, width, height, kBitsPerComponent,
        width * 4 /* bytesPerRow */, pixelFormat.colorSpace.get(),
        pixelFormat.contextAlphaInfo));

    if (context.get()) {
      InitContext(zoom, height, context.get());
    }

    return context.release();
  }

  static void InitContext(double zoom, double height, CGContextRef context) {
    // The Quartz 2D coordinate system has its origin at lower-left corner
    // and the y-axis increases from bottom to top. We need to transform the
    // context so that it matches our coordinate system, whose origin locates
    // at upper-left corner and y-axis increases from top to bottom
    CGContextTranslateCTM(context, 0.0, height);
    CGContextScaleCTM(context, 1.0, -1.0);

    if (zoom != 1.0) {
      CGContextScaleCTM(context, zoom, zoom);
    }

    // Save this state as initial state for ClearCanvas
    CGContextSaveGState(context);
  }

  CGImageRef CreateImage() {
    if (!context_.get()) {
      return NULL;
    }
    return CGBitmapContextCreateImage(context_.get());
  }

  CGPatternRef CreatePattern(CGAffineTransform matrix) {
    if (!context_.get()) {
      return NULL;
    }
    ScopedCFTypeRef<CGImageRef> image(CreateImage());
    if (!image.get()) {
      return NULL;
    }
    PatternInfo* pattern_info = new PatternInfo(width_, height_, image);
    if (!pattern_info) {
      return NULL;
    }
    ScopedCFTypeRef<CGPatternRef> pattern(
        CGPatternCreate(pattern_info,
                        CGRectMake(0, 0,
                                   CGImageGetWidth(image),
                                   CGImageGetHeight(image)),
                        matrix,
                        CGImageGetWidth(image),
                        CGImageGetHeight(image),
                        kCGPatternTilingConstantSpacing,
                        true,
                        &kPatternCallbacks));
    if (!pattern.get()) {
      delete pattern_info;
      return NULL;
    }
    return pattern.release();
  }

  static void DrawPatternCallback(void* info, CGContextRef context) {
    PatternInfo* pattern_info = static_cast<PatternInfo*>(info);
    CGContextDrawImage(context,
                       CGRectMake(0.0, 0.0, pattern_info->width,
                                  pattern_info->height),
                       pattern_info->image.get());
  }

  static void ReleasePatternInfoCallback(void* info) {
    PatternInfo* pattern_info = static_cast<PatternInfo*>(info);
    delete pattern_info;
  }

  void OnZoom(double zoom) {
    if (zoom_ == zoom) {
      return;
    }
    zoom_ = zoom;
    context_.reset(CreateContext(zoom_, width_, height_, rawImageFormat_));
  }

  bool PushState() {
    if (context_.get() == NULL) {
      return false;
    }
    CGContextSaveGState(context_.get());
    opacity_stack_.push(opacity_);
    return true;
  }

  bool PopState() {
    if (context_.get() == NULL) {
      return false;
    }
    if (opacity_stack_.empty()) {
      return false;
    }
    CGContextRestoreGState(context_.get());
    opacity_ = opacity_stack_.top();
    opacity_stack_.pop();
    return true;
  }

  bool MultiplyOpacity(double opacity) {
    if (opacity < 0.0 || opacity > 1.0) {
      return false;
    }
    if (context_.get() == NULL) {
      return false;
    }
    opacity_ *= opacity;
    CGContextSetAlpha(context_.get(), opacity_);
    return true;
  }

  void RotateCoordinates(double radians) {
    if (context_.get() == NULL) {
      return;
    }
    CGContextRotateCTM(context_.get(), radians);
  }

  void TranslateCoordinates(double dx, double dy) {
    if (context_.get() == NULL) {
      return;
    }
    CGContextTranslateCTM(context_.get(), dx, dy);
  }

  void ScaleCoordinates(double sx, double sy) {
    if (context_.get() == NULL) {
      return;
    }
    CGContextScaleCTM(context_.get(), sx, sy);
  }

  bool DrawImageInRect(double x, double y,
                       double w, double h,
                       CGImageRef img) {
    ASSERT(context_.get());
    if (!img) {
      return false;
    }
    if (w < 0.0 || h < 0.0) {
      return false;
    }

    // We flipped the context to match our coordinate system when context was
    // created. We need to flip it back so that the image won't be painted
    // up side down
    ScaleCoordinates(1, -1);
    // Now we need to paint the image at (x, -y - h, x + w, -y)
    CGRect boundingRect = CGRectMake(x, -y - h, w, h);
    CGContextDrawImage(context_.get(), boundingRect, img);
    // Revert the flip
    ScaleCoordinates(1, -1);
    return true;
  }

  bool DrawPatternInRect(double x, double y,
                         double w, double h,
                         CGPatternRef pattern) {
    ASSERT(context_.get());
    if (!pattern) {
      return false;
    }

    this->PushState();

    ScopedCFTypeRef<CGColorSpaceRef> colorSpace(
        CGColorSpaceCreatePattern(NULL));
    CGContextSetFillColorSpace(context_.get(), colorSpace.get());

    ScaleCoordinates(1, -1);
    CGRect boundingRect = CGRectMake(x, -y - h, w, h);
    CGFloat alpha = 1.0;
    CGContextSetFillPattern(context_.get(), pattern, &alpha);
    CGContextFillRect(context_.get(), boundingRect);
    this->PopState();
    return true;
  }

  ScopedCFTypeRef<CGContextRef> context_;
  double width_;
  double height_;
  double zoom_;
  double opacity_;
  RawImageFormat rawImageFormat_;
  Connection *on_zoom_connection_;
  std::stack<double> opacity_stack_;
  static const CGPatternCallbacks kPatternCallbacks;
}; // class Impl

const CGPatternCallbacks QuartzCanvas::Impl::kPatternCallbacks = {
  .version = 0,
  .drawPattern = DrawPatternCallback,
  .releaseInfo = ReleasePatternInfoCallback,
};

QuartzCanvas::QuartzCanvas() : impl_(NULL) {
}

QuartzCanvas::~QuartzCanvas() {
}

bool QuartzCanvas::Init(const QuartzGraphics *graphics, double w, double h,
                        RawImageFormat rawImageFormat) {
  impl_.reset(new Impl);
  if (impl_.get() == NULL) {
    return false;
  }
  return impl_->Init(graphics, w, h, rawImageFormat);
}

bool QuartzCanvas::Init(const QuartzGraphics *graphics, double w, double h,
                        CGContextRef context) {
  impl_.reset(new Impl);
  if (impl_.get() == NULL) {
    return false;
  }
  return impl_->Init(graphics, w, h, context);
}

void QuartzCanvas::Destroy() {
  delete this;
}

double QuartzCanvas::GetWidth() const {
  return impl_->width_;
}

double QuartzCanvas::GetHeight() const {
  return impl_->height_;
}

bool QuartzCanvas::PushState() {
  return impl_->PushState();
}

bool QuartzCanvas::PopState() {
  return impl_->PopState();
}

bool QuartzCanvas::MultiplyOpacity(double opacity) {
  return impl_->MultiplyOpacity(opacity);
}

void QuartzCanvas::RotateCoordinates(double radians) {
  impl_->RotateCoordinates(radians);
}

void QuartzCanvas::TranslateCoordinates(double dx, double dy) {
  impl_->TranslateCoordinates(dx, dy);
}

void QuartzCanvas::ScaleCoordinates(double cx, double cy) {
  impl_->ScaleCoordinates(cx, cy);
}

bool QuartzCanvas::ClearCanvas() {
  if (!IsValid()) {
    return false;
  }

  CGContextRef context = GetContext();

  // restore to initial states
  while (impl_->PopState()) {}
  CGContextRestoreGState(context);
  CGContextSaveGState(context);
  impl_->opacity_ = 1.0;

  // reset transformation to identity matrix
  CGAffineTransform currentCtm = CGContextGetCTM(context);
  CGAffineTransform revertCtm = CGAffineTransformInvert(currentCtm);
  CGContextConcatCTM(context, revertCtm);

  // clear canvas
  CGFloat canvasWidth = ceil(impl_->width_ * impl_->zoom_);
  CGFloat canvasHeight = ceil(impl_->height_ * impl_->zoom_);
  CGContextClearRect(context, CGRectMake(0.0, 0.0, canvasWidth, canvasHeight));

  // restore transformation
  CGContextConcatCTM(context, currentCtm);
  return true;
}

bool QuartzCanvas::ClearRect(double x, double y, double w, double h) {
  if (!IsValid()) {
    return false;
  }
  CGContextClearRect(GetContext(), CGRectMake(x, y, w, h));
  return true;
}

bool QuartzCanvas::DrawLine(double x0, double y0, double x1, double y1,
                            double width, const Color &c) {
  if (!IsValid()) {
    return false;
  }
  if (width < 0.0) {
    return false;
  }

  CGContextRef context = GetContext();
  this->PushState();
  CGContextBeginPath(context);
  CGContextMoveToPoint(context, x0, y0);
  CGContextAddLineToPoint(context, x1, y1);
  CGContextSetLineWidth(context, width);
  CGContextSetRGBStrokeColor(context, c.red, c.green, c.blue, 1.0);
  CGContextStrokePath(context);
  this->PopState();
  return true;
}

bool QuartzCanvas::DrawFilledRect(double x, double y,
                                  double w, double h, const Color &c) {
  if (!IsValid()) {
    return false;
  }
  if (w < 0.0 || h < 0.0) {
    return false;
  }

  CGContextRef context = GetContext();
  this->PushState();
  CGContextSetRGBFillColor(context, c.red, c.green, c.blue, 1.0);
  CGContextFillRect(context, CGRectMake(x, y, w, h));
  this->PopState();
  return true;
}

CGImageRef QuartzCanvas::CreateImage() const {
  return impl_->CreateImage();
}

bool QuartzCanvas::DrawCanvas(double x, double y, const CanvasInterface *img) {
  if (img == NULL) {
    return false;
  }
  if (!IsValid()) {
    return false;
  }
  const QuartzCanvas* source_canvas = down_cast<const QuartzCanvas*>(img);
  ScopedCFTypeRef<CGImageRef> image(source_canvas->CreateImage());
  if (!image.get())
    return false;
  impl_->DrawImageInRect(x, y,
                         source_canvas->GetWidth(),
                         source_canvas->GetHeight(),
                         image.get());
  return true;
}

bool QuartzCanvas::DrawRawImage(double x, double y,
                                const char *data, RawImageFormat format,
                                int width, int height, int stride) {
  if (!IsValid()) {
    return false;
  }
  if (!data) {
    return false;
  }
  QuartzPixelFormat pixelFormat;
  pixelFormat.Init(format);

  if (stride < kBitsPerPixel / 8 * width) {
    return false;
  }
  ScopedCFTypeRef<CGDataProviderRef> provider(CGDataProviderCreateWithData(
      NULL /* info */, data, stride * height, NULL /* releaseDataCallback */));
  if (!provider.get()) {
    return false;
  }
  ScopedCFTypeRef<CGImageRef> image(CGImageCreate(
      width, height, kBitsPerComponent, kBitsPerPixel,
      stride, pixelFormat.colorSpace.get(), pixelFormat.bitmapInfo,
      provider.get(), NULL /* decode */, true /* shouldInterpolate */,
      kCGRenderingIntentDefault));
  if (!image.get()) {
    return false;
  }
  impl_->DrawImageInRect(x, y, width, height, image.get());
  return true;
}

bool QuartzCanvas::DrawFilledRectWithCanvas(double x, double y,
                                            double w, double h,
                                            const CanvasInterface *img) {
  if (img == NULL) {
    return false;
  }
  CGContextRef context = GetContext();
  if (!context) {
    return false;
  }

  const QuartzCanvas* source_canvas = down_cast<const QuartzCanvas*>(img);
  CGFloat inv_zoom = 1.0 / source_canvas->GetZoom();

  // Quartz uses pattern space as the coordinate system for drawing patterns,
  // which is seperate from user space. We have to transform pattern space to
  // match user space.
  CGAffineTransform matrix = CGContextGetCTM(context);
  matrix = CGAffineTransformTranslate(matrix, x, y);
  // User space is vertically flipped. We need to revert the flip for pattern
  // space so it will not be upside down
  matrix = CGAffineTransformScale(matrix, inv_zoom, -inv_zoom);

  ScopedCFTypeRef<CGPatternRef> pattern(
      source_canvas->impl_->CreatePattern(matrix));
  if (!pattern.get()) {
    return false;
  }

  impl_->DrawPatternInRect(x, y, w, h, pattern.get());
  return true;
}

bool QuartzCanvas::DrawCanvasWithMask(double x, double y,
                                      const CanvasInterface *img,
                                      double mx, double my,
                                      const CanvasInterface *mask) {
  if (!IsValid()) {
    return false;
  }
  CGContextRef context = GetContext();
  bool ret = true;
  this->PushState();
  this->IntersectRectClipRegion(mx, my, mask->GetWidth(), mask->GetHeight());
  CGContextBeginTransparencyLayer(context, NULL);
  ret &= this->DrawCanvas(x, y, img);
  if (ret) {
    CGContextSetBlendMode(context, kCGBlendModeDestinationOut);
    ret &= this->DrawCanvas(mx, my, mask);
  }
  CGContextEndTransparencyLayer(context);
  this->PopState();
  return ret;
}

bool QuartzCanvas::DrawText(double x, double y, double width, double height,
                            const char *text, const FontInterface *f,
                            const Color &c, Alignment align, VAlignment valign,
                            Trimming trimming, int text_flags) {
  // This function is no longer used.
  ASSERT_M(0, ("Please use TextRenderer::DrawText"));
  return false;
}

bool QuartzCanvas::DrawTextWithTexture(double x, double y, double width,
                                       double height, const char *text,
                                       const FontInterface *f,
                                       const CanvasInterface *texture,
                                       Alignment align, VAlignment valign,
                                       Trimming trimming, int text_flags) {
  // This function is no longer used.
  ASSERT_M(0, ("Please use TextRenderer::DrawTextWithTexture"));
  return false;
}

bool QuartzCanvas::IntersectRectClipRegion(double x, double y,
                                           double w, double h) {
  if (!IsValid()) {
    return false;
  }
  if (w <= 0.0 || h <= 0.0) {
    return false;
  }
  CGContextClipToRect(GetContext(), CGRectMake(x, y, w, h));
  return true;
}

bool QuartzCanvas::IntersectGeneralClipRegion(const ClipRegion &region) {
  if (!IsValid()) {
    return false;
  }
  if (region.IsEmpty()) {
    return true;
  }
  size_t count = 0;
  CGRect rects[region.GetRectangleCount()];
  for (size_t i = 0; i < region.GetRectangleCount(); ++i) {
    const Rectangle& regionRect = region.GetRectangle(i);
    if (!regionRect.IsEmpty()) {
      CGRect &rect = rects[count++];
      rect = CGRectMake(regionRect.x, regionRect.y,
                        regionRect.w, regionRect.h);
    }
  }
  CGContextClipToRects(GetContext(), rects, count);
  return true;
}

bool QuartzCanvas::GetTextExtents(const char *text, const FontInterface *f,
                                  int text_flags, double in_width,
                                  double *width, double *height) {
  return false;
}

bool QuartzCanvas::GetPointValue(double x, double y,
                                 Color *color, double *opacity) const {
  if (!IsValid()) {
    return false;
  }

  CGContextRef context = GetContext();
  unsigned char* data =
    static_cast<unsigned char*>(CGBitmapContextGetData(context));
  if (!data) {
    return false;
  }
  CGPoint device_pos = CGContextConvertPointToDeviceSpace(context,
                                                          CGPointMake(x, y));
  int xi = static_cast<int>(round(device_pos.x));
  int yi = static_cast<int>(round(device_pos.y));
  if (xi < 0 || xi >= CGBitmapContextGetWidth(context) ||
      yi < 0 || yi >= CGBitmapContextGetHeight(context)) {
    return false;
  }
  size_t index = yi * CGBitmapContextGetBytesPerRow(context) +
      xi * CGBitmapContextGetBitsPerPixel(context) / 8;
  if (color) {
    color->red = data[index + 1] / 255.0;
    color->green = data[index + 2] / 255.0;
    color->blue = data[index + 3] / 255.0;
  }
  if (opacity &&
      impl_->rawImageFormat_ == CanvasInterface::RAWIMAGE_FORMAT_ARGB32) {
    *opacity = data[index] / 255.0;
    // color components are pre-multiplied.
    if (color && *opacity > 0) {
      color->red /= *opacity;
      color->green /= *opacity;
      color->blue /= *opacity;
    }
  }
  return true;
}

bool QuartzCanvas::IsValid() const {
  return impl_->context_.get() != NULL;
}

double QuartzCanvas::GetZoom() const {
  return impl_->zoom_;
}

CGContextRef QuartzCanvas::GetContext() const {
  return impl_->context_.get();
}

void QuartzCanvas::SetBlendMode(CGBlendMode mode) {
  CGContextSetBlendMode(GetContext(), mode);
}

} // namespace mac
} // namespace ggadget
