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

// This file contains the implementation of class GdiplusGraphics

#include "gdiplus_graphics.h"
using std::min;
using std::max;
#include <gdiplus.h>
#include <string>
#include <ggadget/signals.h>
#include <ggadget/scoped_ptr.h>
#include <ggadget/logger.h>
#include "gdiplus_canvas.h"
#include "gdiplus_font.h"
#include "gdiplus_image.h"
#include "private_font_database.h"
#include "text_renderer.h"

namespace ggadget {
namespace win32 {

namespace {
static const int kPointsPerInch = 72;
inline double PointToPixel(double point, HDC hdc) {
  return point * ::GetDeviceCaps(hdc, LOGPIXELSY) / kPointsPerInch;
}
}  // namespace

class GdiplusGraphics::Impl {
 public:
  Impl(double zoom, const PrivateFontDatabase* private_font_database)
      : font_scale_(1.0),
        zoom_(zoom),
        private_font_database_(private_font_database) {
    ASSERT_M(zoom > 0, ("zoom = %lf must be positive\n", zoom));
  }
  double font_scale_;
  double zoom_;
  Signal1<void, double> on_zoom_signal_;
  const PrivateFontDatabase* private_font_database_;
};

GdiplusGraphics::GdiplusGraphics(
    double zoom, const PrivateFontDatabase* private_font_database) {
  impl_.reset(new Impl(zoom, private_font_database));
}

GdiplusGraphics::~GdiplusGraphics() {
}

Connection* GdiplusGraphics::ConnectOnZoom(Slot1<void, double>* slot) const {
  return impl_->on_zoom_signal_.Connect(slot);
}

CanvasInterface* GdiplusGraphics::NewCanvas(double w, double h) const {
  if (w <= 0 || h <= 0) return NULL;
  scoped_ptr<GdiplusCanvas> canvas(new GdiplusCanvas);
  if (canvas.get() == NULL) return NULL;
  if (!canvas->Init(this, w, h, true)) return NULL;
  if (!canvas->IsValid()) return NULL;
  return canvas.release();
}

ImageInterface* GdiplusGraphics::NewImage(const std::string& tag,
                                          const std::string& data,
                                          bool is_mask) const {
  if (data.empty()) return NULL;
  scoped_ptr<GdiplusImage> image(new GdiplusImage);
  if (image.get() == NULL) return NULL;
  if (!image->Init(tag, data, is_mask)) return NULL;
  if (!image->IsValid()) return NULL;
  return image.release();
}

FontInterface* GdiplusGraphics::NewFont(
    const std::string& family, double pt_size, FontInterface::Style style,
    FontInterface::Weight weight) const {
  GdiplusFont* font = new GdiplusFont;
  bool succeed = font->Init(family, pt_size, style, weight,
                            impl_->private_font_database_);
  if (!succeed) {
    font->Destroy();
    return NULL;
  }
  return font;
}

double GdiplusGraphics::GetZoom() const {
  return impl_->zoom_;
}

void GdiplusGraphics::SetZoom(double zoom) {
  ASSERT_M(zoom > 0, ("zoom = %lf must be positive\n", zoom));
  if (impl_->zoom_ != zoom) {
    impl_->zoom_ = zoom;
    impl_->on_zoom_signal_(impl_->zoom_);
  }
}

void GdiplusGraphics::SetFontScale(double scale) {
  ASSERT(scale > 0);
  impl_->font_scale_ = scale;
}

const PrivateFontDatabase* GdiplusGraphics::GetFontDatabase() const {
  return impl_->private_font_database_;
}

Gdiplus::Font* GdiplusGraphics::CreateFont(const TextFormat* format) const {
  UTF16String family_name_utf16;
  ConvertStringUTF8ToUTF16(format->font(), &family_name_utf16);
  scoped_ptr<Gdiplus::FontFamily> font_family;
  // Check private font database first.
  if (impl_->private_font_database_) {
    font_family.reset(impl_->private_font_database_->CreateFontFamilyByName(
        reinterpret_cast<const wchar_t*>(family_name_utf16.c_str())));
  }
  // If font not found, check the system fonts.
  if (font_family.get() == NULL || !font_family->IsAvailable()) {
    font_family.reset(new Gdiplus::FontFamily(
        reinterpret_cast<const wchar_t*>(family_name_utf16.c_str())));
  }
  // If font not found in system fonts either, return the default font.
  if (font_family.get() == NULL || !font_family->IsAvailable())
    font_family.reset(Gdiplus::FontFamily::GenericSansSerif()->Clone());
  if (font_family.get() == NULL || !font_family->IsAvailable())
    return NULL;
  INT font_style = (format->bold() ? Gdiplus::FontStyleBold : 0) |
                   (format->italic() ? Gdiplus::FontStyleItalic : 0);
  Gdiplus::REAL size = format->size() * impl_->font_scale_ * format->scale();
  return new Gdiplus::Font(font_family.get(),
                           static_cast<Gdiplus::REAL>(size),
                           font_style);
}

HFONT GdiplusGraphics::CreateHFont(const TextFormat* format) const {
  UTF16String family_name_utf16;
  ConvertStringUTF8ToUTF16(format->font(), &family_name_utf16);
  LOGFONT logfont = {0};
  logfont.lfCharSet = DEFAULT_CHARSET;
  HDC dc = ::CreateCompatibleDC(NULL);
  logfont.lfHeight = -static_cast<LONG>(PointToPixel(format->size(), dc) *
                                        impl_->font_scale_ * format->scale());
  ::DeleteDC(dc);
  logfont.lfUnderline = format->underline();
  logfont.lfStrikeOut = format->strikeout();
  logfont.lfItalic = format->italic();
  logfont.lfWeight = format->bold() ? FW_BOLD : FW_NORMAL;
  ::wcscpy_s(logfont.lfFaceName, LF_FACESIZE, family_name_utf16.c_str());
  return ::CreateFontIndirect(&logfont);
}

double GdiplusGraphics::GetFontScale() const {
  return impl_->font_scale_;
}

TextRendererInterface* GdiplusGraphics::NewTextRenderer() const {
  return new TextRenderer(this);
}

}  // namespace win32
}  // namespace ggadget
