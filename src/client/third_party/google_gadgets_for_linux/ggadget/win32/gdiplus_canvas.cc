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

#include "gdiplus_canvas.h"

using std::min;
using std::max;
#include <windows.h>
#ifdef GDIPVER
#undef GDIPVER
#endif
#define GDIPVER 0x0110
#include <gdiplus.h>
#include <usp10.h>
#include <algorithm>
#include <string>
#include <stack>
#include <vector>

#include <ggadget/clip_region.h>
#include <ggadget/color.h>
#include <ggadget/logger.h>
#include <ggadget/scoped_ptr.h>
#include <ggadget/signals.h>
#include "gdiplus_font.h"
#include "gdiplus_graphics.h"
#include "gdiplus_image.h"

#ifdef DrawText
#undef DrawText
#endif

namespace ggadget {
namespace win32 {

namespace {

static const double kByteMax = 255.0;
static const int kPointsPerInch = 72;

// Casts different number types to Gdiplus::REAL
inline Gdiplus::REAL ToReal(double x) {
  return static_cast<Gdiplus::REAL>(x);
}

// Casts double to int with rounding.
inline int D2I(double d) {
  return static_cast<int>(d > 0 ? d + 0.5: d - 0.5);
}

// Loads a bitmap from string data, the data size is in data.size().
Gdiplus::Bitmap* LoadBitmapFromString(const std::string& data) {
  HGLOBAL global_buffer = ::GlobalAlloc(GMEM_MOVEABLE, data.size());
  if (global_buffer == NULL) {
    return NULL;
  }
  void* buffer = ::GlobalLock(global_buffer);
  if (buffer == NULL) {
    ::GlobalFree(global_buffer);
    return NULL;
  }
  CopyMemory(buffer, data.data(), data.size());
  ::GlobalUnlock(global_buffer);
  IStream* stream = NULL;
  if (::CreateStreamOnHGlobal(global_buffer, FALSE, &stream) != S_OK) {
    ::GlobalFree(global_buffer);
    return NULL;
  }
  Gdiplus::Bitmap* image = Gdiplus::Bitmap::FromStream(stream);
  stream->Release();
  if (image == NULL) {
    ::GlobalFree(global_buffer);
    return NULL;
  }
  if (image->GetLastStatus() != Gdiplus::Ok) {
    ::GlobalFree(global_buffer);
    return NULL;
  }
  return image;
}

// Converts ggadget::Color struct to Gdiplus::Color with respect to alpha.
inline Gdiplus::Color GdiplusColor(const Color color, double opacity) {
  return Gdiplus::Color(static_cast<BYTE>(opacity * kByteMax),
                        static_cast<BYTE>(color.RedInt()),
                        static_cast<BYTE>(color.GreenInt()),
                        static_cast<BYTE>(color.BlueInt()));
}

// Creates a new Bitmap that contains the masked version of source_image with
// respect to the mask. mx and my specify the coordinates of the mask's top left
// corner in the source_image(they can be negative).
// The returned image should be deleted by caller.
Gdiplus::Bitmap* CreateMaskedImage(
    const Gdiplus::Bitmap& source_image, const Gdiplus::Bitmap& mask,
    double mx, double my) {
  // Drops const because GDI+ methods don't have const modifier.
  Gdiplus::Bitmap* source_pointer = const_cast<Gdiplus::Bitmap*>(&source_image);
  Gdiplus::Bitmap* mask_pointer = const_cast<Gdiplus::Bitmap*>(&mask);
  scoped_ptr<Gdiplus::Bitmap> masked_image(new Gdiplus::Bitmap(
      source_pointer->GetWidth(), source_pointer->GetHeight(),
      PixelFormat32bppARGB));
  RECT source_bounding = {
      0, 0, source_pointer->GetWidth(), source_pointer->GetHeight()};
  RECT mask_bounding = {static_cast<LONG>(mx), static_cast<LONG>(my),
                        static_cast<LONG>(mx + mask_pointer->GetWidth()),
                        static_cast<LONG>(my + mask_pointer->GetHeight())};
  RECT masked_bounding;
  IntersectRect(&masked_bounding, &source_bounding, &mask_bounding);
  Gdiplus::Rect masked_rect(masked_bounding.left, masked_bounding.top,
                            masked_bounding.right - masked_bounding.left,
                            masked_bounding.bottom - masked_bounding.top);
  // Copy source image to masked image first.
  Gdiplus::Graphics masked_graphics(masked_image.get());
  masked_graphics.Clear(Gdiplus::Color::Transparent);
  masked_graphics.DrawImage(source_pointer,
                            masked_rect,  // Destination rect.
                            masked_rect.X, masked_rect.Y,  // Source top left.
                            masked_rect.Width, masked_rect.Height,
                            Gdiplus::UnitPixel);
  // Manipulate the alpha channel of the masked image pixel by pixel.
  Gdiplus::BitmapData mask_data;
  if (mask_pointer->LockBits(&masked_rect, Gdiplus::ImageLockModeRead,
      mask_pointer->GetPixelFormat(), &mask_data) != Gdiplus::Ok) {
    return NULL;
  }
  Gdiplus::BitmapData masked_data;
  if (masked_image->LockBits(
      &masked_rect, Gdiplus::ImageLockModeWrite, masked_image->GetPixelFormat(),
      &masked_data) != Gdiplus::Ok) {
    mask_pointer->UnlockBits(&mask_data);
    return NULL;
  }
  int offset_x = Gdiplus::GetPixelFormatSize(masked_image->GetPixelFormat())/8;
  int alpha_offset = Gdiplus::Color::AlphaShift / 8;
  for (int y = masked_rect.Y; y < masked_rect.GetBottom(); ++y) {
    BYTE* masked_byte = reinterpret_cast<BYTE*>(masked_data.Scan0) +
                        y * masked_data.Stride;
    BYTE* mask_byte = reinterpret_cast<BYTE*>(mask_data.Scan0) +
                      (y - D2I(my)) * mask_data.Stride;
    for (int x = masked_rect.X; x < masked_rect.GetRight(); ++x) {
      masked_byte[x * offset_x + alpha_offset] = static_cast<BYTE>(
          mask_byte[(x - D2I(mx)) * offset_x + alpha_offset] / kByteMax *
          masked_byte[x * offset_x + alpha_offset]);
    }
  }
  mask_pointer->UnlockBits(&mask_data);
  masked_image->UnlockBits(&masked_data);
  return masked_image.release();
}

HFONT FontInterfaceToHFONT(const FontInterface* f, int text_flags) {
  HDC dc = ::GetDC(NULL);
  Gdiplus::Graphics temp_graphics(dc);
  const GdiplusFont* font = down_cast<const GdiplusFont*>(f);
  bool underline = text_flags & CanvasInterface::TEXT_FLAGS_UNDERLINE;
  bool strike_out = text_flags & CanvasInterface::TEXT_FLAGS_STRIKEOUT;
  scoped_ptr<Gdiplus::Font> gdiplus_font(font->CreateGdiplusFont(underline,
                                                                 strike_out));
  LOGFONTW logfont = {0};
  gdiplus_font->GetLogFontW(&temp_graphics, &logfont);
  HFONT hfont = ::CreateFontIndirectW(&logfont);
  ::ReleaseDC(NULL, dc);
  return hfont;
}

}  // namespace

// The implementation of GdiplusCamvas.
class GdiplusCanvas::Impl {
 public:
  Impl()
      : width_(0),
        height_(0),
        opacity_(1.0f),
        zoom_(1.0f),
        on_zoom_connection_(NULL) {
  }

  ~Impl() {
    if (on_zoom_connection_)
      on_zoom_connection_->Disconnect();
  }

  bool Init(const std::string& data, bool create_graphics) {
    image_.reset(LoadBitmapFromString(data));
    if (image_.get() == NULL)
      return false;
    width_ = image_->GetWidth();
    height_ = image_->GetHeight();
    if (create_graphics) {
      gdiplus_graphics_.reset(Gdiplus::Graphics::FromImage(image_.get()));
      if (gdiplus_graphics_.get() == NULL)
        return false;
    }
    return true;
  }

  bool Init(const GdiplusGraphics* graphics, double width, double height,
            bool create_graphics) {
    width_ = width;
    height_ = height;
    if (graphics) {
      zoom_ = graphics->GetZoom();
      on_zoom_connection_ =
        graphics->ConnectOnZoom(NewSlot(this, &Impl::OnZoom));
    }
    image_.reset(new Gdiplus::Bitmap(D2I(width*zoom_), D2I(height*zoom_),
                                     PixelFormat32bppARGB));
    if (image_.get() == NULL) {
      return false;
    }
    if (create_graphics) {
      gdiplus_graphics_.reset(Gdiplus::Graphics::FromImage(image_.get()));
      if (gdiplus_graphics_.get() == NULL)
        return false;
      if (gdiplus_graphics_->ScaleTransform(ToReal(zoom_), ToReal(zoom_)) !=
          Gdiplus::Ok)
        return false;
      gdiplus_graphics_->SetTextRenderingHint(
          Gdiplus::TextRenderingHintSystemDefault);
      gdiplus_graphics_->SetSmoothingMode(Gdiplus::SmoothingModeHighSpeed);
      gdiplus_graphics_->SetCompositingQuality(
          Gdiplus::CompositingQualityHighSpeed);
    }
    return true;
  }

  bool PopState() {
    if (gdiplus_graphics_.get() == NULL)
      return false;
    if (opacity_stack_.empty() || graphics_state_stack_.empty())
      return false;
    opacity_ = opacity_stack_.top();
    Gdiplus::Status status =
        gdiplus_graphics_->Restore(graphics_state_stack_.top());
    opacity_stack_.pop();
    graphics_state_stack_.pop();
    return status == Gdiplus::Ok;
  }

  bool PushState() {
    if (gdiplus_graphics_.get() == NULL)
      return false;
    opacity_stack_.push(opacity_);
    graphics_state_stack_.push(gdiplus_graphics_->Save());
    return true;
  }

  void ClearStacks() {
    graphics_state_stack_ = std::stack<Gdiplus::GraphicsState>();
    opacity_stack_ = std::stack<double>();
  }

  void OnZoom(double zoom) {
    if (zoom == zoom_) return;
    if (!image_.get())  // Not support zoom for such canvas
      return;
    Gdiplus::Bitmap* new_image = new Gdiplus::Bitmap(
        D2I(width_ * zoom), D2I(height_ * zoom), PixelFormat32bppARGB);
    if (!new_image) return;
    image_.reset(new_image);
    gdiplus_graphics_.reset(Gdiplus::Graphics::FromImage(new_image));
    gdiplus_graphics_->Clear(Gdiplus::Color(0, 0, 0, 0));
    gdiplus_graphics_->ScaleTransform(ToReal(zoom), ToReal(zoom));
    zoom_ = zoom;
  }

  scoped_ptr<Gdiplus::Bitmap> image_;
  scoped_ptr<Gdiplus::Graphics> gdiplus_graphics_;
  double width_;
  double height_;
  double opacity_;
  double zoom_;
  std::stack<Gdiplus::GraphicsState> graphics_state_stack_;
  std::stack<double> opacity_stack_;
  Connection *on_zoom_connection_;
};

Gdiplus::Bitmap* GdiplusCanvas::GetImage() const {
  return impl_->image_.get();
}

GdiplusCanvas::GdiplusCanvas() {
}

GdiplusCanvas::~GdiplusCanvas() {
}

bool GdiplusCanvas::Init(const GdiplusGraphics *graphics, double w, double h,
                         bool create_graphics) {
  impl_.reset(new Impl);
  if (impl_.get() == NULL)
    return false;
  return impl_->Init(graphics, w, h, create_graphics);
}

bool GdiplusCanvas::Init(const std::string &data, bool create_graphics) {
  impl_.reset(new Impl);
  if (impl_.get() == NULL)
    return false;
  return impl_->Init(data, create_graphics);
}

void GdiplusCanvas::Destroy() {
  delete this;
}

double GdiplusCanvas::GetWidth() const {
  return impl_->width_;
}

double GdiplusCanvas::GetHeight() const {
  return impl_->height_;
}

bool GdiplusCanvas::PopState() {
  return impl_->PopState();
}

bool GdiplusCanvas::PushState() {
  return impl_->PushState();
}

bool GdiplusCanvas::MultiplyOpacity(double opacity) {
  if (opacity >= 0.0 && opacity <= 1.0) {
    impl_->opacity_ *= opacity;
    return true;
  }
  return false;
}

void GdiplusCanvas::RotateCoordinates(double radians) {
  Gdiplus::Graphics* gdiplus_graphics = impl_->gdiplus_graphics_.get();
  if (gdiplus_graphics == NULL)
    return;
  gdiplus_graphics->RotateTransform(ToReal(radians / M_PI * 180));
}

void GdiplusCanvas::TranslateCoordinates(double dx, double dy) {
  Gdiplus::Graphics* gdiplus_graphics = impl_->gdiplus_graphics_.get();
  if (gdiplus_graphics == NULL)
    return;
  gdiplus_graphics->TranslateTransform(ToReal(dx), ToReal(dy));
}

void GdiplusCanvas::ScaleCoordinates(double cx, double cy) {
  Gdiplus::Graphics* gdiplus_graphics = impl_->gdiplus_graphics_.get();
  if (gdiplus_graphics == NULL)
    return;
  gdiplus_graphics->ScaleTransform(ToReal(cx), ToReal(cy));
}

bool GdiplusCanvas::ClearCanvas() {
  Gdiplus::Graphics* gdiplus_graphics = impl_->gdiplus_graphics_.get();
  if (gdiplus_graphics == NULL) return false;
  gdiplus_graphics->ResetClip();
  gdiplus_graphics->ResetTransform();
  gdiplus_graphics->Clear(Gdiplus::Color::Transparent);
  gdiplus_graphics->ScaleTransform(ToReal(impl_->zoom_),
                                   ToReal(impl_->zoom_));
  impl_->opacity_ = 1;
  impl_->ClearStacks();
  return true;
}

bool GdiplusCanvas::ClearRect(double x, double y, double w, double h) {
  if (w < 0 || h < 0)
    return false;
  Gdiplus::Graphics* gdiplus_graphics = impl_->gdiplus_graphics_.get();
  if (gdiplus_graphics == NULL) return false;
  Gdiplus::Color background_color = Gdiplus::Color(0, 0, 0, 0);
  PushState();
  Gdiplus::RectF clip_rect(ToReal(x), ToReal(y), ToReal(w), ToReal(h));
  gdiplus_graphics->IntersectClip(clip_rect);
  Gdiplus::Status status = gdiplus_graphics->Clear(background_color);
  PopState();
  return status == Gdiplus::Ok;
}

bool GdiplusCanvas::DrawLine(
    double x0, double y0, double x1, double y1, double width, const Color &c) {
  if (width <= 0)
    return false;
  Gdiplus::Graphics* gdiplus_graphics = impl_->gdiplus_graphics_.get();
  if (gdiplus_graphics == NULL) return false;
  Gdiplus::Color color = GdiplusColor(c, impl_->opacity_);
  Gdiplus::Pen pen(color,
                   ToReal(width));
  if (pen.GetLastStatus() != Gdiplus::Ok || pen.GetWidth() != width)
    return false;
  return gdiplus_graphics->DrawLine(&pen, ToReal(x0), ToReal(y0),
                                    ToReal(x1), ToReal(y1)) == Gdiplus::Ok;
}

bool GdiplusCanvas::DrawFilledRect(double x, double y, double w, double h,
                                   const Color& c) {
  if (w < 0 || h < 0)
    return false;
  Gdiplus::Graphics* gdiplus_graphics = impl_->gdiplus_graphics_.get();
  if (gdiplus_graphics == NULL) return false;
  Gdiplus::SolidBrush brush(GdiplusColor(c, impl_->opacity_));
  return gdiplus_graphics->FillRectangle(
      &brush, ToReal(x), ToReal(y), ToReal(w), ToReal(h)) == Gdiplus::Ok;
}

bool GdiplusCanvas::DrawCanvas(double x, double y,
                               const CanvasInterface* img) {
  if (img == NULL)
    return false;
  Gdiplus::Graphics* gdiplus_graphics = impl_->gdiplus_graphics_.get();
  if (gdiplus_graphics == NULL) return false;
  const GdiplusCanvas* source_canvas = down_cast<const GdiplusCanvas*>(img);
  Gdiplus::Bitmap* source_image = source_canvas->impl_->image_.get();
  if (source_image == NULL) return false;
  Gdiplus::ImageAttributes image_attribute;
  Gdiplus::ColorMatrix color_matrix = {
    1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, ToReal(impl_->opacity_), 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
  };
  image_attribute.SetColorMatrix(&color_matrix);
  double source_zoom = down_cast<const GdiplusCanvas*>(img)->GetZoom();
  Gdiplus::REAL invert_zoom = ToReal(1/source_zoom);
  PushState();
  gdiplus_graphics->ScaleTransform(invert_zoom, invert_zoom);
  Gdiplus::Status status = gdiplus_graphics->DrawImage(
      source_image,
      Gdiplus::RectF(ToReal(x * source_zoom), ToReal(y * source_zoom),
                     ToReal(source_image->GetWidth()),
                     ToReal(source_image->GetHeight())),
      Gdiplus::RectF(0, 0, ToReal(source_image->GetWidth()),
                     ToReal(source_image->GetHeight())),
      Gdiplus::UnitPixel, &image_attribute);
  PopState();
  return status == Gdiplus::Ok;
}

bool GdiplusCanvas::DrawRawImage(double x, double y, const char* data,
                                 RawImageFormat format, int width, int height,
                                 int stride) {
  if (data == NULL) return false;
  Gdiplus::Graphics* gdiplus_graphics = impl_->gdiplus_graphics_.get();
  if (gdiplus_graphics == NULL) return false;
  Gdiplus::PixelFormat pixel_format = PixelFormat32bppARGB;
  if (format == RAWIMAGE_FORMAT_RGB24)
    pixel_format = PixelFormat32bppRGB;
  Gdiplus::Bitmap source_image(
      width, height, stride, pixel_format,
      reinterpret_cast <BYTE*>(const_cast<char*>(data)));
  if (source_image.GetLastStatus() != Gdiplus::Ok)
    return false;
  Gdiplus::ImageAttributes image_attribute;
  Gdiplus::ColorMatrix color_matrix = {
    1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, ToReal(impl_->opacity_), 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
  };
  image_attribute.SetColorMatrix(&color_matrix);
  Gdiplus::Status status = gdiplus_graphics->DrawImage(
      &source_image,
      Gdiplus::RectF(ToReal(x), ToReal(y), ToReal(source_image.GetWidth()),
                     ToReal(source_image.GetHeight())),
      Gdiplus::RectF(0, 0, ToReal(source_image.GetWidth()),
                     ToReal(source_image.GetHeight())),
      Gdiplus::UnitPixel, &image_attribute);
  return status == Gdiplus::Ok;
}

bool GdiplusCanvas::DrawFilledRectWithCanvas(double x, double y,
                                             double w, double h,
                                             const CanvasInterface* img) {
  if (img == NULL) return false;
  Gdiplus::Graphics* gdiplus_graphics = impl_->gdiplus_graphics_.get();
  if (gdiplus_graphics == NULL) return false;
  const GdiplusCanvas* source_canvas = down_cast<const GdiplusCanvas*>(img);
  Gdiplus::Bitmap* source_image = source_canvas->impl_->image_.get();
  if (source_image == NULL) return false;
  Gdiplus::ImageAttributes image_attribute;
  Gdiplus::ColorMatrix color_matrix = {
    1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, ToReal(impl_->opacity_), 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
  };
  image_attribute.SetColorMatrix(&color_matrix);
  Gdiplus::Rect image_bound(0, 0,
                source_image->GetWidth(), source_image->GetHeight());
  Gdiplus::TextureBrush brush(source_image, image_bound, &image_attribute);
  if (brush.GetLastStatus() != Gdiplus::Ok)
    return false;
  if (brush.SetWrapMode(Gdiplus::WrapModeTile) != Gdiplus::Ok)
    return false;
  Gdiplus::RectF rect(0, 0, ToReal(w), ToReal(h));
  PushState();
  // Translate coordinates because texture brush always oriented at (0, 0).
  TranslateCoordinates(x, y);
  bool success = (gdiplus_graphics->FillRectangle(&brush, rect) == Gdiplus::Ok);
  PopState();
  return success;
}

bool GdiplusCanvas::DrawCanvasWithMask(double x, double y,
                                       const CanvasInterface* img,
                                       double mx, double my,
                                       const CanvasInterface* mask) {
  if (img == NULL || mask == NULL) return false;
  Gdiplus::Graphics* gdiplus_graphics = impl_->gdiplus_graphics_.get();
  const GdiplusCanvas* source_canvas = down_cast<const GdiplusCanvas*>(img);
  Gdiplus::Bitmap* source_image = source_canvas->impl_->image_.get();
  if (source_image == NULL) return false;
  double source_zoom = down_cast<const GdiplusCanvas*>(img)->GetZoom();
  // scale source image to the unscaled size first
  if (source_zoom != 1.0) {
    Gdiplus::Bitmap* new_source_image = new Gdiplus::Bitmap(
        D2I(img->GetWidth()), D2I(img->GetHeight()), PixelFormat32bppARGB);
    Gdiplus::Graphics new_source_graphics(new_source_image);
    Gdiplus::REAL invert_zoom = ToReal(1/source_zoom);
    new_source_graphics.ScaleTransform(invert_zoom, invert_zoom);
    new_source_graphics.DrawImage(source_image, 0, 0);
    source_image = new_source_image;
  }
  const GdiplusCanvas* mask_canvas = down_cast<const GdiplusCanvas*>(mask);
  Gdiplus::Bitmap* mask_image = mask_canvas->impl_->image_.get();
  if (mask_image == NULL) return false;
  double mask_zoom = down_cast<const GdiplusCanvas*>(mask)->GetZoom();
  // scale mask image to the unscaled size first
  if (mask_zoom != 1.0) {
    Gdiplus::Bitmap* new_mask_image = new Gdiplus::Bitmap(
      D2I(mask->GetWidth()), D2I(mask->GetHeight()), PixelFormat32bppARGB);
    Gdiplus::Graphics new_mask_graphics(new_mask_image);
    Gdiplus::REAL invert_zoom = ToReal(1/mask_zoom);
    new_mask_graphics.ScaleTransform(invert_zoom, invert_zoom);
    new_mask_graphics.DrawImage(mask_image, 0, 0);
    mask_image = new_mask_image;
  }
  scoped_ptr<Gdiplus::Bitmap> masked_image(
    CreateMaskedImage(*source_image, *mask_image, mx - x, my - y));
  if (mask_zoom != 1.0) delete mask_image;
  if (source_zoom != 1.0) delete source_image;
  // set image_attribute to apply opacity value
  Gdiplus::ImageAttributes image_attribute;
  Gdiplus::ColorMatrix color_matrix = {
    1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, ToReal(impl_->opacity_), 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
  };
  image_attribute.SetColorMatrix(&color_matrix);
  Gdiplus::Status status = gdiplus_graphics->DrawImage(masked_image.get(),
    Gdiplus::RectF(ToReal(x), ToReal(y), ToReal(masked_image->GetWidth()),
                   ToReal(masked_image->GetHeight())),
    Gdiplus::RectF(0, 0, ToReal(masked_image->GetWidth()),
                   ToReal(masked_image->GetHeight())),
    Gdiplus::UnitPixel, &image_attribute);
  return status == Gdiplus::Ok;
}

bool GdiplusCanvas::DrawText(double x, double y, double width, double height,
                             const char* text, const FontInterface* f,
                             const Color &c, Alignment align,
                             VAlignment valign, Trimming trimming,
                             int text_flags) {
  // This function is no longer used.
  ASSERT_M(0, ("Please use TextRenderer::DrawText"));
  return false;
}

bool GdiplusCanvas::DrawTextWithTexture(
    double x, double y, double width, double height, const char* text,
    const FontInterface* f, const CanvasInterface* texture, Alignment align,
    VAlignment valign, Trimming trimming, int text_flags) {
  // This function is no longer used.
  ASSERT_M(0, ("Please use TextRenderer::DrawTextWithTexture"));
  return false;
}

bool GdiplusCanvas::IntersectRectClipRegion(double x, double y,
                                            double w, double h) {
  if (w < 0 || h < 0)
    return false;
  Gdiplus::Graphics* gdiplus_graphics = impl_->gdiplus_graphics_.get();
  if (gdiplus_graphics == NULL) return false;
  Gdiplus::RectF clip_rect(ToReal(x), ToReal(y), ToReal(w), ToReal(h));
  return gdiplus_graphics->IntersectClip(clip_rect) == Gdiplus::Ok;
}

bool GdiplusCanvas::IntersectGeneralClipRegion(const ClipRegion &region) {
  Gdiplus::Region gdiplus_region;
  gdiplus_region.MakeEmpty();
  for (size_t i = 0; i < region.GetRectangleCount(); ++i) {
    const Rectangle& rect = region.GetRectangle(i);
    if (!rect.IsEmpty()) {
      Gdiplus::RectF rectf(ToReal(rect.x), ToReal(rect.y),
                           ToReal(rect.w), ToReal(rect.h));
      if (gdiplus_region.Union(rectf) != Gdiplus::Ok)
        return false;
    }
  }
  Gdiplus::Graphics* gdiplus_graphics = impl_->gdiplus_graphics_.get();
  if (gdiplus_graphics == NULL) return false;
  return gdiplus_graphics->IntersectClip(&gdiplus_region) == Gdiplus::Ok;
}

bool GdiplusCanvas::GetTextExtents(const char* text, const FontInterface* f,
                                   int text_flags, double in_width,
                                   double* width, double* height) {
  // This function is no longer used.
  ASSERT_M(0, ("Please use TextRenderer::GetTextExtents"));
  return false;
}

bool GdiplusCanvas::GetPointValue(double x, double y,
                                  Color* color, double* opacity) const {
  Gdiplus::Bitmap* image = GetImage();
  Gdiplus::Color gdiplus_color;
  if (image->GetPixel(D2I(x), D2I(y), &gdiplus_color) != Gdiplus::Ok)
    return false;
  if (color != NULL) {
    color->red = gdiplus_color.GetR() / kByteMax;
    color->green = gdiplus_color.GetG() / kByteMax;
    color->blue = gdiplus_color.GetB() / kByteMax;
  }
  if (opacity != NULL)
    *opacity = gdiplus_color.GetA() / kByteMax;
  return true;
}

Gdiplus::Graphics* GdiplusCanvas::GetGdiplusGraphics() const {
  return impl_->gdiplus_graphics_.get();
}

bool GdiplusCanvas::IsValid() const {
  return impl_ != NULL && impl_->gdiplus_graphics_ != NULL;
}

double GdiplusCanvas::GetZoom() const {
  return impl_->zoom_;
}

double GdiplusCanvas::GetOpacity() const {
  return impl_->opacity_;
}

}  // namespace win32
}  // namespace ggadget
