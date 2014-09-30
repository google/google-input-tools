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
#include <vector>
#include <stack>
#include <algorithm>
#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtGui/QTextDocument>
#include <QtGui/QAbstractTextDocumentLayout>

#include <ggadget/scoped_ptr.h>
#include <ggadget/signals.h>
#include <ggadget/slot.h>
#include <ggadget/clip_region.h>
#include <ggadget/math_utils.h>
#include "qt_graphics.h"
#include "qt_canvas.h"
#include "qt_font.h"

#if 1
#undef DLOG
#define DLOG  true ? (void) 0 : LOG
#endif

namespace ggadget {
namespace qt {

const char *const kEllipsisText = "...";

static void MakeImageTransparent(QImage *img) {
  QPainter p(img);
  p.setCompositionMode(QPainter::CompositionMode_Source);
  p.fillRect(img->rect(), Qt::transparent);
}

static void SetupPainter(QPainter *p) {
  p->setCompositionMode(QPainter::CompositionMode_SourceOver);
  p->setRenderHint(QPainter::SmoothPixmapTransform, true);
  p->setRenderHint(QPainter::Antialiasing, false);
  p->setBackground(Qt::transparent);
}

class QtCanvas::Impl {
 public:
  Impl(QtCanvas *owner, const QtGraphics *g,
       double w, double h, bool create_painter)
      : owner_(owner),
        width_(w), height_(h), opacity_(1.), zoom_(1.),
        on_zoom_connection_(NULL),
        image_(NULL), painter_(NULL), region_(NULL) {
    if (g){
      zoom_ = g->GetZoom();
      on_zoom_connection_ =
        g->ConnectOnZoom(NewSlot(this, &Impl::OnZoom));
    }
    image_ = new QImage(D2I(w*zoom_), D2I(h*zoom_),
                        QImage::Format_ARGB32_Premultiplied);
    if (image_ == NULL) return;
    MakeImageTransparent(image_);
    if (create_painter) {
      painter_ = new QPainter(image_);
      SetupPainter(painter_);
      painter_->scale(zoom_, zoom_);
    }
  }

  Impl(QtCanvas *owner, const std::string &data, bool create_painter)
      : owner_(owner),
        width_(0), height_(0),
        opacity_(1.), zoom_(1.),
        on_zoom_connection_(NULL),
        image_(NULL), painter_(NULL), region_(NULL) {
    image_ = new QImage();
    if (!image_) return;

    bool ret = image_->loadFromData(
        reinterpret_cast<const uchar *>(data.c_str()),
        static_cast<int>(data.length()));

    if (ret) {
      width_ = image_->width();
      height_ = image_->height();
      if (create_painter) {
        painter_ = new QPainter(image_);
        SetupPainter(painter_);
      }
    } else {
      delete image_;
      image_ = NULL;
    }
  }

  Impl(QtCanvas *owner, double w, double h, QPainter *painter)
      : owner_(owner),
        width_(w), height_(h), opacity_(1.), zoom_(1.),
        on_zoom_connection_(NULL), image_(NULL),
        painter_(painter), region_(NULL) {
    SetupPainter(painter_);
  }

  ~Impl() {
    if (painter_ && image_) delete painter_;
    if (image_) delete image_;
    if (on_zoom_connection_)
      on_zoom_connection_->Disconnect();
  }

  bool DrawLine(double x0, double y0, double x1, double y1,
                double width, const Color &c) {
    QPainter *p = painter_;
    QColor color(c.RedInt(), c.GreenInt(), c.BlueInt());
    QPen pen(color);
    pen.setWidthF(width);
    p->setPen(pen);
    p->drawLine(QPointF(x0, y0), QPointF(x1, y1));
    return true;
  }

  bool DrawFilledRect(double x, double y, double w, double h, const Color &c) {
    DLOG("DrawFilledRect:%p", owner_);
    QPainter *p = painter_;
    QColor color(c.RedInt(), c.GreenInt(), c.BlueInt());
    p->fillRect(QRectF(x, y, w, h), color);
    return true;
  }

  bool DrawCanvas(double x, double y, const CanvasInterface *img) {
    DLOG("DrawCanvas:%p on %p", img, owner_);
    QPainter *p = painter_;
    const QtCanvas *canvas = reinterpret_cast<const QtCanvas*>(img);
    Impl *impl = canvas->impl_;
    double sx, sy;
    if (impl->GetScale(&sx, &sy)) {
      p->save();
      p->scale(sx, sy);
      p->drawImage(QPointF(x / sx, y / sy), *canvas->GetImage());
      p->restore();
    } else {
      p->drawImage(QPointF(x, y), *canvas->GetImage());
    }
    return true;
  }

  bool DrawFilledRectWithCanvas(double x, double y, double w, double h,
                                const CanvasInterface *img) {
    DLOG("DrawFilledRectWithCanvas: %p on %p", img, owner_);
    QPainter *p = painter_;
    const QtCanvas *canvas = reinterpret_cast<const QtCanvas*>(img);
    p->fillRect(QRectF(x, y, w, h), *canvas->GetImage());
    return true;
  }

  bool DrawCanvasWithMask(double x, double y,
                          const CanvasInterface *img,
                          double mx, double my,
                          const CanvasInterface *mask) {
    GGL_UNUSED(mx);
    GGL_UNUSED(my);
    DLOG("DrawCanvasWithMask: (%p, %p) on %p", img, mask, owner_);
    QPainter *p = painter_;
    const QtCanvas *s = reinterpret_cast<const QtCanvas*>(img);
    const QtCanvas *m = reinterpret_cast<const QtCanvas*>(mask);
    QImage *simg = s->GetImage();
    // FIXME: the content of img will be changed. but hopefully it's ok
    // in current drawing model, where DrawCanvasWithMask() is only used in
    // BasicElement's Draw() method and img is a temporary buffer.
    QPainter *spainter = s->GetQPainter();
    spainter->setCompositionMode(QPainter::CompositionMode_DestinationIn);
    spainter->drawImage(0, 0, *m->GetImage());
    spainter->setCompositionMode(QPainter::CompositionMode_SourceOver);
    Impl *simpl = s->impl_;
    double sx, sy;
    if (simpl->GetScale(&sx, &sy) && image_) {
      p->save();
      p->scale(sx, sy);
      p->drawImage(QPointF(x / sx, y / sy), *simg);
      p->restore();
    } else {
      p->drawImage(QPointF(x, y), *simg);
    }
    return true;
  }

  static void SetupQTextDocument(QTextDocument *doc,
                                 const FontInterface *f,
                                 int text_flags,
                                 Alignment align,
                                 double in_width) {
    // Setup font
    const QtFont *qtfont = down_cast<const QtFont*>(f);
    QFont font = *qtfont->GetQFont();

    if (text_flags & TEXT_FLAGS_UNDERLINE)
      font.setUnderline(true);
    else
      font.setUnderline(false);

    if (text_flags & TEXT_FLAGS_STRIKEOUT)
      font.setStrikeOut(true);
    else
      font.setStrikeOut(false);

    doc->setDefaultFont(font);

    // Setup align
    Qt::Alignment a;
    a = align == ALIGN_RIGHT ? Qt::AlignRight :
        align == ALIGN_CENTER ? Qt::AlignHCenter :
        align == ALIGN_JUSTIFY ? Qt::AlignJustify :
        Qt::AlignLeft;
    QTextOption option(a);
    if (text_flags & TEXT_FLAGS_WORDWRAP) {
      option.setWrapMode(QTextOption::WordWrap);
    } else {
      option.setWrapMode(QTextOption::NoWrap);
    }

    if (in_width > 0)
      doc->setTextWidth(in_width);
    doc->setDefaultTextOption(option);
  }

  QString ElidedText(const QString &str, const FontInterface *f,
                     double width, Trimming trimming) {
    const QtFont *qtfont = down_cast<const QtFont*>(f);
    QFont font = *qtfont->GetQFont();
    QFontMetrics fm(font);
    Qt::TextElideMode mode =
        trimming == TRIMMING_PATH_ELLIPSIS ? Qt::ElideMiddle : Qt::ElideRight;

    return fm.elidedText(str, mode, static_cast<int>(width));
  }

  bool DrawText(double x, double y, double width, double height,
                const char *text, const FontInterface *f,
                const Color &c, Alignment align, VAlignment valign,
                Trimming trimming, int text_flags) {
    DLOG("DrawText:%s, %f, %f, %f, %f", text, x, y, width, height);
    QPainter *p = painter_;
    QString qt_text = QString::fromUtf8(text);
    QTextDocument doc(qt_text);
    SetupQTextDocument(&doc, f, text_flags, align, width);

    // taking care valign
    double text_height = doc.documentLayout()->documentSize().height();
    if (text_height < height) {
      if (valign == VALIGN_MIDDLE) {
        y += (height - text_height)/2;
        height -= (height - text_height)/2;
      } else if (valign == VALIGN_BOTTOM) {
        y += (height - text_height);
        height = text_height;
      }
    }
    // Handle trimming
    double text_width = doc.documentLayout()->documentSize().width();
    if (trimming != TRIMMING_NONE) {
      if (text_width > width && !(text_flags & TEXT_FLAGS_WORDWRAP)) {
        doc.setPlainText(ElidedText(qt_text, f, width, trimming));
      } else if (text_height > height && (text_flags & TEXT_FLAGS_WORDWRAP)) {
        double ypos = height - 8;
        if (ypos < 0) ypos = 0;
        int pos = doc.documentLayout()->hitTest(
            QPointF(width, ypos), Qt::FuzzyHit);
        if (pos >= 4 && pos < qt_text.length()) {
          qt_text.chop(qt_text.length() - pos + 3);
          qt_text.append("...");
          doc.setPlainText(qt_text);
        } else if (pos < 4) {
          doc.setPlainText("...");
        }
      }
    }

    QRectF rect(0, 0, width, height);

    QAbstractTextDocumentLayout::PaintContext ctx;
    p->save();
    ctx.clip = rect;
    p->translate(x, y);
    QColor color(c.RedInt(), c.GreenInt(), c.BlueInt());
    ctx.palette.setBrush(QPalette::Text, color);
    doc.documentLayout()->draw(p, ctx);
    p->restore();
    return true;
  }

  bool DrawTextWithTexture(double x, double y, double width,
                           double height, const char *text,
                           const FontInterface *f,
                           const CanvasInterface *texture,
                           Alignment align, VAlignment valign,
                           Trimming trimming, int text_flags) {
    GGL_UNUSED(x);
    GGL_UNUSED(y);
    GGL_UNUSED(width);
    GGL_UNUSED(height);
    GGL_UNUSED(f);
    GGL_UNUSED(texture);
    GGL_UNUSED(align);
    GGL_UNUSED(valign);
    GGL_UNUSED(trimming);
    GGL_UNUSED(text_flags);
    DLOG("DrawTextWithTexture: %s", text);
    ASSERT(0);
    return true;
  }

  bool IntersectRectClipRegion(double x, double y,
                               double w, double h) {
    if (w <= 0.0 || h <= 0.0) return false;
    painter_->setClipRect(QRectF(x, y, w, h), Qt::IntersectClip);
    return true;
  }

  bool IntersectRectangle(double x, double y, double w, double h) {
    QRect qrect(D2I(x), D2I(y), D2I(w), D2I(h));
    *region_ = region_->united(QRegion(qrect));
    return true;
  }

  bool IntersectGeneralClipRegion(const ClipRegion &region) {
    QRegion qregion;
    region_ = &qregion;

    if (region.EnumerateRectangles(NewSlot(this, &Impl::IntersectRectangle))) {
      painter_->setClipRegion(qregion, Qt::IntersectClip);
    }
    return true;
  }

  bool GetPointValue(double x, double y,
                     Color *color, double *opacity) const {
    // Canvas without image_ doesn't support GetPointValue
    if (!image_) return false;

    if (x < 0 || x >= width_ || y < 0 || y >= height_) return false;

    QColor qcolor = image_->pixel(D2I(x), D2I(y));
    if (color) {
      color->red = qcolor.redF();
      color->green = qcolor.greenF();
      color->blue = qcolor.blueF();
    }

    if (opacity) *opacity = qcolor.alphaF();

    return true;
  }

  void OnZoom(double zoom) {
    DLOG("zoom, width_, height_:%f, %f, %f", zoom, width_, height_);
    if (zoom == zoom_) return;
    ASSERT(image_); // Not support zoom for such canvas
    QImage* new_image = new QImage(D2I(width_*zoom), D2I(height_*zoom),
                                   QImage::Format_ARGB32_Premultiplied);
    if (!new_image) return;
    if (painter_) delete painter_;
    delete image_;
    image_ = new_image;
    MakeImageTransparent(image_);
    painter_ = new QPainter(image_);
    SetupPainter(painter_);
    painter_->scale(zoom, zoom);
    zoom_ = zoom;
  }

  // Return false if no scale at all
  bool GetScale(double *sx, double *sy) {
    if (image_->height() == height_ && image_->width() == width_) {
      if (sx) *sx = 1.0;
      if (sy) *sy = 1.0;
      return false;
    }

    if (sx) *sx = width_ / image_->width();
    if (sy) *sy = height_ / image_->height();
    return true;
  }

  QtCanvas *owner_;
  double width_, height_;
  double opacity_;
  double zoom_;
  Connection *on_zoom_connection_;
  QImage *image_;
  QPainter *painter_;
  QRegion *region_;
};

QtCanvas::QtCanvas(const QtGraphics *g, double w, double h, bool create_painter)
    : impl_(new Impl(this, g, w, h, create_painter)) {
}

QtCanvas::QtCanvas(double w, double h, QPainter *painter)
    : impl_(new Impl(this, w, h, painter)) {
}

QtCanvas::QtCanvas(const std::string &data, bool create_painter)
    : impl_(new Impl(this, data, create_painter)) {
}

QtCanvas::~QtCanvas() {
  delete impl_;
  impl_ = NULL;
}

void QtCanvas::Destroy() {
  delete this;
}

bool QtCanvas::ClearCanvas() {
  ClearRect(0, 0, impl_->width_, impl_->height_);
  return true;
}

bool QtCanvas::ClearRect(double x, double y, double w, double h) {
  QPainter *p = impl_->painter_;
  p->save();
  p->setCompositionMode(QPainter::CompositionMode_Source);
  p->eraseRect(QRectF(x, y, w, h));
  p->restore();

  return true;
}

bool QtCanvas::PopState() {
  impl_->painter_->restore();
  return true;
}

bool QtCanvas::PushState() {
  impl_->painter_->save();
  return true;
}

bool QtCanvas::MultiplyOpacity(double opacity) {
  if (opacity >= 0.0 && opacity <= 1.0) {
    // TODO: setOpacity is not recommended to use for performance issue.
    impl_->painter_->setOpacity(impl_->painter_->opacity()*opacity);
    return true;
  }
  return false;
}

bool QtCanvas::DrawLine(double x0, double y0, double x1, double y1,
                        double width, const Color &c) {
  return impl_->DrawLine(x0, y0, x1, y1, width, c);
}

void QtCanvas::RotateCoordinates(double radians) {
  impl_->painter_->rotate(RadiansToDegrees(radians));
}

void QtCanvas::TranslateCoordinates(double dx, double dy) {
  impl_->painter_->translate(dx, dy);
}

void QtCanvas::ScaleCoordinates(double cx, double cy) {
  impl_->painter_->scale(cx, cy);
}

bool QtCanvas::DrawFilledRect(double x, double y,
                              double w, double h, const Color &c) {
  return impl_->DrawFilledRect(x, y, w, h, c);
}

bool QtCanvas::IntersectRectClipRegion(double x, double y,
                                          double w, double h) {
  return impl_->IntersectRectClipRegion(x, y, w, h);
}

bool QtCanvas::IntersectGeneralClipRegion(const ClipRegion &region) {
  return impl_->IntersectGeneralClipRegion(region);
}

bool QtCanvas::DrawCanvas(double x, double y, const CanvasInterface *img) {
  return impl_->DrawCanvas(x, y, img);
}

bool QtCanvas::DrawRawImage(double x, double y,
                            const char *data, RawImageFormat format,
                            int width, int height, int stride) {
  GGL_UNUSED(stride);
  QImage::Format qt_format;
  if (format == RAWIMAGE_FORMAT_RGB24)
    qt_format = QImage::Format_RGB32;
  else
    qt_format = QImage::Format_ARGB32;
  QImage img(reinterpret_cast<const uchar*>(data), width, height, qt_format);
  impl_->painter_->drawImage(D2I(x), D2I(y), img);
  return true;
}

bool QtCanvas::DrawFilledRectWithCanvas(double x, double y,
                                           double w, double h,
                                           const CanvasInterface *img) {
  return impl_->DrawFilledRectWithCanvas(x, y, w, h, img);
}

bool QtCanvas::DrawCanvasWithMask(double x, double y,
                                    const CanvasInterface *img,
                                    double mx, double my,
                                    const CanvasInterface *mask) {
  return impl_->DrawCanvasWithMask(x, y, img, mx, my, mask);
}

bool QtCanvas::DrawText(double x, double y, double width, double height,
                           const char *text, const FontInterface *f,
                           const Color &c, Alignment align, VAlignment valign,
                           Trimming trimming,  int text_flags) {
  return impl_->DrawText(x, y, width, height, text, f,
                         c, align, valign, trimming, text_flags);
}


bool QtCanvas::DrawTextWithTexture(double x, double y, double w, double h,
                                   const char *text,
                                   const FontInterface *f,
                                   const CanvasInterface *texture,
                                   Alignment align, VAlignment valign,
                                   Trimming trimming, int text_flags) {
  return impl_->DrawTextWithTexture(x, y, w, h, text, f, texture,
                                    align, valign, trimming, text_flags);
}

bool QtCanvas::GetTextExtents(const char *text, const FontInterface *f,
                              int text_flags, double in_width,
                              double *width, double *height) {
  QTextDocument doc(QString::fromUtf8(text));
  if (in_width <= 0) {
    text_flags &= ~TEXT_FLAGS_WORDWRAP;
  }
  Impl::SetupQTextDocument(&doc, f, text_flags, ALIGN_LEFT, in_width);
  if (width)
    *width = doc.documentLayout()->documentSize().width();
  if (height)
    *height = doc.documentLayout()->documentSize().height();
  return true;
}

bool QtCanvas::GetPointValue(double x, double y,
                                Color *color, double *opacity) const {
  return impl_->GetPointValue(x, y, color, opacity);
}

double QtCanvas::GetWidth() const {
  return impl_->width_;
}

double QtCanvas::GetHeight() const {
  return impl_->height_;
}

bool QtCanvas::IsValid() const {
  if (impl_->painter_ != NULL)
    return true;
  else
    return false;
}

QImage* QtCanvas::GetImage() const {
  return impl_->image_;
}

QPainter* QtCanvas::GetQPainter() const {
  return impl_->painter_;
}

} // namespace qt
} // namespace ggadget
