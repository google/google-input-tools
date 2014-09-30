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
#include <algorithm>

#include <QtCore/QUrl>
#include <QtGui/QDesktopWidget>
#include <QtGui/QPalette>
#include <QtGui/QPainter>
#include <QtGui/QLayout>
#include <QtGui/QMouseEvent>
#include <ggadget/graphics_interface.h>
#include <ggadget/main_loop_interface.h>
#include <ggadget/clip_region.h>
#include <ggadget/math_utils.h>
#include <ggadget/string_utils.h>
#include <ggadget/qt/qt_canvas.h>
#include <ggadget/qt/utilities.h>
#include <ggadget/qt/qt_menu.h>
#include "qt_view_widget.h"

#if defined(Q_WS_X11) && defined(HAVE_X11)
#include <QtGui/QX11Info>
#include <QtGui/QBitmap>
#include <X11/extensions/shape.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#endif

namespace ggadget {
namespace qt {

static const double kDragThreshold = 3;
#ifdef _DEBUG
static const uint64_t kFPSCountDuration = 5000;
#endif

// update input mask once per second.
static const uint64_t kUpdateMaskInterval = 1000;

// Minimal interval between queue draws.
static const unsigned int kQueueDrawInterval = 40;

// Maximum live duration of queue draw timer.
static const uint64_t kQueueDrawTimerDuration = 1000;

static QRegion CreateClipRegion(const ClipRegion *view_region, double zoom) {
  QRegion qregion;
  QRect qrect;
  size_t count = view_region->GetRectangleCount();
  if (count) {
    Rectangle rect;
    for (size_t i = 0; i < count; ++i) {
      rect = view_region->GetRectangle(i);
      if (zoom != 1.0) {
        rect.Zoom(zoom);
        rect.Integerize(true);
      }
      qrect.setX(static_cast<int>(rect.x));
      qrect.setY(static_cast<int>(rect.y));
      qrect.setWidth(static_cast<int>(rect.w));
      qrect.setHeight(static_cast<int>(rect.h));
      qregion += qrect;
    }
  }
  return qregion;
}

class QtViewWidget::Impl {
 public:
  Impl(QtViewWidget* owner, ViewInterface* view, QtViewWidget::Flags flags)
    : owner_(owner),
      view_(view),
      drag_files_(NULL),
      drag_urls_(NULL),
      composite_(flags.testFlag(QtViewWidget::FLAG_COMPOSITE)),
      movable_(flags.testFlag(QtViewWidget::FLAG_MOVABLE)),
      enable_input_mask_(false),
      support_input_mask_(flags.testFlag(QtViewWidget::FLAG_INPUT_MASK) &&
                          flags.testFlag(QtViewWidget::FLAG_COMPOSITE)),
      offscreen_pixmap_(NULL),
      mouse_drag_moved_(false),
      child_(NULL),
      zoom_(view->GetGraphics()->GetZoom()),
      last_mask_time_(0),
#ifdef _DEBUG
      last_fps_time_(0),
      draw_count_(0),
#endif
      mouse_down_hittest_(ViewInterface::HT_CLIENT),
      resize_drag_(false),
      old_width_(0),
      old_height_(0),
      last_redraw_time_(0),
      redraw_timer_(0),
      draw_queued_(false),
      self_redraw_(false) {
  }

  ~Impl() {
    if (child_) {
      // Because I don't own child_, just let it go
      child_->setParent(NULL);
    }
    if (redraw_timer_) owner_->killTimer(redraw_timer_);
    if (offscreen_pixmap_) delete offscreen_pixmap_;
    if (drag_files_) delete [] drag_files_;
    if (drag_urls_) delete [] drag_urls_;
  }

  void paintEvent(QPaintEvent* event) {
    if (!view_) return;
    QPainter p(owner_);
    p.setClipRegion(event->region());
    
    int w = owner_->width();
    int h = owner_->height();

#if 0
    DLOG("paint %p: s:%d, ow:%d, oh:%d, w:%d, h:%d, vw:%f, vh:%f, uw:%d, uh:%d",
         view_, self_redraw_, old_width_, old_height_, w, h,
         view_->GetWidth(), view_->GetHeight(),
         event->rect().width(), event->rect().height());
#endif

    if (!self_redraw_) {
      view_->Layout();
      view_->AddRectangleToClipRegion(
          Rectangle(0, 0, view_->GetWidth(), view_->GetHeight()));
    }
    self_redraw_ = false;

    if (old_width_ != w || old_height_ != h) {
      view_->AddRectangleToClipRegion(
          Rectangle(0, 0, view_->GetWidth(), view_->GetHeight()));
      old_width_ = w;
      old_height_ = h;
      delete offscreen_pixmap_;
      offscreen_pixmap_ = NULL;
    }

    uint64_t current_time = GetGlobalMainLoop()->GetCurrentTime();
    if (enable_input_mask_ &&
        (current_time - last_mask_time_ > kUpdateMaskInterval)) {
      last_mask_time_ = current_time;
      // Only update input mask once per second.
      if (!offscreen_pixmap_) {
        offscreen_pixmap_ = new QPixmap(w, h);
        offscreen_pixmap_->fill(Qt::transparent);
        view_->AddRectangleToClipRegion(
            Rectangle(0, 0, view_->GetWidth(), view_->GetHeight()));
      }

      QRegion clip_region = CreateClipRegion(view_->GetClipRegion(), zoom_);
      QPainter poff(offscreen_pixmap_);
      poff.setClipRegion(clip_region);
      poff.setCompositionMode(QPainter::CompositionMode_Clear);
      poff.fillRect(owner_->rect(), Qt::transparent);
      poff.scale(zoom_, zoom_);

      QtCanvas canvas(w, h, &poff);
      view_->Draw(&canvas);
      SetInputMask(offscreen_pixmap_);

      p.setCompositionMode(QPainter::CompositionMode_Source);
      p.drawPixmap(0, 0, *offscreen_pixmap_);
    } else {
      if (composite_) {
        p.setCompositionMode(QPainter::CompositionMode_Source);
        p.fillRect(owner_->rect(), Qt::transparent);
      }
      p.scale(zoom_, zoom_);
      QtCanvas canvas(w, h, &p);
      view_->Draw(&canvas);
    }

#ifdef _DEBUG
    ++draw_count_;
    uint64_t duration = current_time - last_fps_time_;
    if (duration >= kFPSCountDuration) {
      last_fps_time_ = current_time;
      DLOG("FPS of View %s: %f", view_->GetCaption().c_str(),
           static_cast<double>(draw_count_ * 1000) /
           static_cast<double>(duration));
      draw_count_ = 0;
    }
#endif
  }

  void mouseMoveEvent(QMouseEvent* event) {
    if (!view_) return;
    int buttons = GetMouseButtons(event->buttons());
    if (buttons != MouseEvent::BUTTON_NONE) {
      if (!mouse_drag_moved_) {
        // Ignore tiny movement of mouse.
        QPoint offset = QCursor::pos() - mouse_pos_;
        if (std::abs(static_cast<double>(offset.x())) < kDragThreshold &&
            std::abs(static_cast<double>(offset.y())) < kDragThreshold)
          return;
      }
    }

    MouseEvent e(Event::EVENT_MOUSE_MOVE,
                 event->x() / zoom_, event->y() / zoom_, 0, 0,
                 buttons, 0);

    if (view_->OnMouseEvent(e) != ggadget::EVENT_RESULT_UNHANDLED)
      event->accept();
    else if (buttons != MouseEvent::BUTTON_NONE) {
      // Send fake mouse up event to the view so that we can start to drag
      // the window. Note: no mouse click event is sent in this case, to prevent
      // unwanted action after window move.
      if (!mouse_drag_moved_) {
        mouse_drag_moved_ = true;
        MouseEvent e(Event::EVENT_MOUSE_UP,
                     event->x() / zoom_, event->y() / zoom_,
                     0, 0, buttons, 0);
        // Ignore the result of this fake event.
        view_->OnMouseEvent(e);

        resize_drag_ = true;
        origi_geometry_ = owner_->window()->geometry();
        top_ = bottom_ = left_ = right_ = 0;
        if (mouse_down_hittest_ == ViewInterface::HT_LEFT)
          left_ = 1;
        else if (mouse_down_hittest_ == ViewInterface::HT_RIGHT)
          right_ = 1;
        else if (mouse_down_hittest_ == ViewInterface::HT_TOP)
          top_ = 1;
        else if (mouse_down_hittest_ == ViewInterface::HT_BOTTOM)
          bottom_ = 1;
        else if (mouse_down_hittest_ == ViewInterface::HT_TOPLEFT)
          top_ = 1, left_ = 1;
        else if (mouse_down_hittest_ == ViewInterface::HT_TOPRIGHT)
          top_ = 1, right_ = 1;
        else if (mouse_down_hittest_ == ViewInterface::HT_BOTTOMLEFT)
          bottom_ = 1, left_ = 1;
        else if (mouse_down_hittest_ == ViewInterface::HT_BOTTOMRIGHT)
          bottom_ = 1, right_ = 1;
        else
          resize_drag_ = false;
      }

      if (resize_drag_) {
        QPoint p = QCursor::pos() - mouse_pos_;
        QRect rect = origi_geometry_;
        rect.setTop(rect.top() + top_ * p.y());
        rect.setBottom(rect.bottom() + bottom_ * p.y());
        rect.setLeft(rect.left() + left_ * p.x());
        rect.setRight(rect.right() + right_ * p.x());
        double w = rect.width();
        double h = rect.height();
        if (w != view_->GetWidth() || h != view_->GetHeight()) {
          if (view_->OnSizing(&w, &h))
            view_->SetSize(w, h);
        }
      }  else {
        QPoint offset = QCursor::pos() - mouse_pos_;
        if (movable_) owner_->window()->move(owner_->window()->pos() + offset);
        mouse_pos_ = QCursor::pos();
        emit owner_->moved(offset.x(), offset.y());
      }
    }
  }
  void mousePressEvent(QMouseEvent* event) {
    if (!view_) return;
    if (!owner_->hasFocus()) {
      owner_->setFocus(Qt::MouseFocusReason);
      SimpleEvent e(Event::EVENT_FOCUS_IN);
      view_->OnOtherEvent(e);
    }

    mouse_down_hittest_ = view_->GetHitTest();
    mouse_drag_moved_ = false;
    resize_drag_ = false;
    // Remember the position of mouse, it may be used to move the view or
    // resize the view
    mouse_pos_ = QCursor::pos();
    EventResult handler_result = EVENT_RESULT_UNHANDLED;
    int button = GetMouseButton(event->button());

    MouseEvent e(Event::EVENT_MOUSE_DOWN,
                 event->x() / zoom_, event->y() / zoom_, 0, 0, button, 0);
    handler_result = view_->OnMouseEvent(e);

    if (handler_result != ggadget::EVENT_RESULT_UNHANDLED) {
      event->accept();
    }
  }
  void mouseReleaseEvent(QMouseEvent *event) {
    if (!view_ || mouse_drag_moved_)
      return;

    EventResult handler_result = ggadget::EVENT_RESULT_UNHANDLED;
    int button = GetMouseButton(event->button());

    MouseEvent e(Event::EVENT_MOUSE_UP,
                 event->x() / zoom_, event->y() / zoom_, 0, 0, button, 0);
    handler_result = view_->OnMouseEvent(e);
    if (handler_result != ggadget::EVENT_RESULT_UNHANDLED)
      event->accept();

    MouseEvent e1(event->button() == Qt::LeftButton ? Event::EVENT_MOUSE_CLICK :
                  Event::EVENT_MOUSE_RCLICK,
                  event->x() / zoom_, event->y() / zoom_, 0, 0, button, 0);
    handler_result = view_->OnMouseEvent(e1);

    if (handler_result != ggadget::EVENT_RESULT_UNHANDLED)
      event->accept();
  }
  void dragEnterEvent(QDragEnterEvent *event) {
    if (!view_) return;
    DLOG("drag enter");

    bool accept = false;
    delete [] drag_files_;
    delete [] drag_urls_;
    drag_files_ = drag_urls_ = NULL;
    drag_text_ = std::string();
    drag_files_and_urls_.clear();

    if (event->mimeData()->hasText()) {
      drag_text_ = event->mimeData()->text().toUtf8().data();
      accept = true;
    }
    if (event->mimeData()->hasUrls()) {
      QList<QUrl> urls = event->mimeData()->urls();
      drag_files_ = new const char *[urls.size() + 1];
      drag_urls_ = new const char *[urls.size() + 1];
      if (!drag_files_ || !drag_urls_) {
        delete [] drag_files_;
        delete [] drag_urls_;
        drag_files_ = drag_urls_ = NULL;
        return;
      }

      size_t files_count = 0;
      size_t urls_count = 0;
      for (int i = 0; i < urls.size(); i++) {
        std::string url = urls[i].toEncoded().data();
        if (url.length()) {
          if (IsValidFileURL(url.c_str())) {
            std::string path = GetPathFromFileURL(url.c_str());
            if (path.length()) {
              drag_files_and_urls_.push_back(path);
              drag_files_[files_count++] =
                  drag_files_and_urls_[drag_files_and_urls_.size() - 1].c_str();
            }
          } else if (IsValidURL(url.c_str())) {
            drag_files_and_urls_.push_back(url);
            drag_urls_[urls_count++] =
                drag_files_and_urls_[drag_files_and_urls_.size() - 1].c_str();
          }
        }
      }

      drag_files_[files_count] = NULL;
      drag_urls_[urls_count] = NULL;
      accept = accept || files_count || urls_count;
    }

    if (accept)
      event->acceptProposedAction();
  }
  void dragLeaveEvent(QDragLeaveEvent *event) {
    GGL_UNUSED(event);
    if (!view_) return;
    DLOG("drag leave");
    DragEvent drag_event(Event::EVENT_DRAG_OUT, 0, 0);
    drag_event.SetDragFiles(drag_files_);
    drag_event.SetDragUrls(drag_urls_);
    drag_event.SetDragText(drag_text_.length() ? drag_text_.c_str() : NULL);
    view_->OnDragEvent(drag_event);
  }
  void dragMoveEvent(QDragMoveEvent *event) {
    if (!view_) return;
    DragEvent drag_event(Event::EVENT_DRAG_MOTION,
                         event->pos().x(), event->pos().y());
    drag_event.SetDragFiles(drag_files_);
    drag_event.SetDragUrls(drag_urls_);
    drag_event.SetDragText(drag_text_.length() ? drag_text_.c_str() : NULL);
    if (view_->OnDragEvent(drag_event) != EVENT_RESULT_UNHANDLED)
      event->acceptProposedAction();
    else
      event->ignore();
  }
  void dropEvent(QDropEvent *event) {
    if (!view_) return;
    LOG("drag drop");
    DragEvent drag_event(Event::EVENT_DRAG_DROP,
                         event->pos().x(), event->pos().y());
    drag_event.SetDragFiles(drag_files_);
    drag_event.SetDragUrls(drag_urls_);
    drag_event.SetDragText(drag_text_.length() ? drag_text_.c_str() : NULL);
    if (view_->OnDragEvent(drag_event) == EVENT_RESULT_UNHANDLED) {
      event->ignore();
    }
  }

  void EnableInputShapeMask(bool enable) {
    if (!support_input_mask_) return;
    if (enable_input_mask_ != enable) {
      enable_input_mask_ = enable;
      if (!enable) {
        SetInputMask(NULL);
        delete offscreen_pixmap_;
        offscreen_pixmap_ = NULL;
      }
    }
  }

  void SetInputMask(QPixmap *pixmap) {
#if defined(Q_WS_X11) && defined(HAVE_X11)
    if (!pixmap) {
      XShapeCombineMask(QX11Info::display(),
                        owner_->winId(),
                        ShapeInput,
                        0, 0,
                        None,
                        ShapeSet);
      return;
    }
    QBitmap bm = pixmap->createMaskFromColor(QColor(0, 0, 0, 0), Qt::MaskInColor);
    XShapeCombineMask(QX11Info::display(),
                      owner_->winId(),
                      ShapeInput,
                      0, 0,
                      bm.handle(),
                      ShapeSet);
#endif
  }
  void AdjustToViewSize() {
    if (!view_) return;
    int w = D2I(view_->GetWidth() * zoom_);
    int h = D2I(view_->GetHeight() * zoom_);
    if (resize_drag_) {
      int dtop, dleft, dw, dh;
      dw = w - origi_geometry_.width();
      dh = h - origi_geometry_.height();
      dtop = dleft = 0;
      if (top_) {
        dtop = -dh;
        dh = 0;
      }
      if (left_) {
        dleft = -dw;
        dw = 0;
      }

      DLOG("offset: (%d, %d, %d, %d)", dleft, dtop, dw, dh);
      origi_geometry_.adjust(dleft, dtop, dw, dh);
      mouse_pos_ = QCursor::pos();
      if (movable_) {
        emit owner_->window()->setGeometry(origi_geometry_);
      } else {
        emit owner_->geometryChanged(dleft, dtop, dw, dh);
      }
      return;
    }
    owner_->resize(w, h);
  }

  QtViewWidget *owner_;
  ViewInterface *view_;
  const char **drag_files_;
  const char **drag_urls_;
  std::string drag_text_;
  std::vector<std::string> drag_files_and_urls_;
  bool composite_;
  bool movable_;
  bool enable_input_mask_;
  bool support_input_mask_;
  QPixmap *offscreen_pixmap_;
  QPoint mouse_pos_;
  bool mouse_drag_moved_;
  QWidget *child_;
  double zoom_;
  uint64_t last_mask_time_;
#ifdef _DEBUG
  uint64_t last_fps_time_;
  int draw_count_;
#endif
  ViewInterface::HitTest mouse_down_hittest_;
  bool resize_drag_;
  QRect origi_geometry_;
  int old_width_, old_height_;
  // used as coefficient of mouse move in window resize
  int top_, bottom_, left_, right_;

  uint64_t last_redraw_time_;
  int redraw_timer_;
  bool draw_queued_;
  bool self_redraw_;
};

QtViewWidget::QtViewWidget(ViewInterface *view, QtViewWidget::Flags flags)
  : impl_(new Impl(this, view, flags)) {
  setMouseTracking(true);
  setAcceptDrops(true);
  AdjustToViewSize();
  if (!flags.testFlag(QtViewWidget::FLAG_WM_DECORATED)) {
    setWindowFlags(Qt::FramelessWindowHint);
    SetUndecoratedWMProperties();
  }
  setAutoFillBackground(false);
  setAttribute(Qt::WA_InputMethodEnabled);

  if (impl_->composite_) {
    setAttribute(Qt::WA_OpaquePaintEvent);
    setAttribute(Qt::WA_NoSystemBackground);
#if QT_VERSION >= QT_VERSION_CHECK(4,5,0)
    setAttribute(Qt::WA_TranslucentBackground);
#endif
  }
}

QtViewWidget::~QtViewWidget() {
  DLOG("QtViewWidget freed");
  delete impl_;
}

void QtViewWidget::QueueDraw() {
  if (isVisible() && updatesEnabled()) {
    impl_->draw_queued_ = true;

    if (!impl_->redraw_timer_) {
      // Can't call view's GetCaption() here, because at this point, view might
      // not be fully initialized yet.
      DLOG("Install queue draw timer of view: %p", impl_->view_);
      impl_->redraw_timer_ = startTimer(kQueueDrawInterval);
    }
  }
}

void QtViewWidget::timerEvent(QTimerEvent *event) {
  GGL_UNUSED(event);
  uint64_t current_time = GetGlobalMainLoop()->GetCurrentTime();
  if (impl_->draw_queued_) {
    impl_->draw_queued_ = false;
    if (isVisible() && updatesEnabled()) {
      impl_->view_->Layout();
      QRegion clip_region = CreateClipRegion(impl_->view_->GetClipRegion(),
                                             impl_->zoom_);
      if (!clip_region.isEmpty()) {
        impl_->self_redraw_ = true;
        repaint(clip_region);
      }
    }
    impl_->last_redraw_time_ = current_time;
  }

  if (current_time - impl_->last_redraw_time_ > kQueueDrawTimerDuration) {
    DLOG("Remove queue draw timer of view: %p (%s)",
         impl_->view_,
         impl_->view_->GetCaption().c_str());
    killTimer(impl_->redraw_timer_);
    impl_->redraw_timer_ = 0;
  }
}

void QtViewWidget::closeEvent(QCloseEvent *event) {
  emit closeBySystem();
  event->ignore();
}

void QtViewWidget::paintEvent(QPaintEvent *event) {
  impl_->paintEvent(event);
}

void QtViewWidget::mouseDoubleClickEvent(QMouseEvent * event) {
  if (!impl_->view_) return;
  Event::Type type;
  if (Qt::LeftButton == event->button())
    type = Event::EVENT_MOUSE_DBLCLICK;
  else
    type = Event::EVENT_MOUSE_RDBLCLICK;
  MouseEvent e(type, event->x() / impl_->zoom_, event->y() / impl_->zoom_,
               0, 0, 0, 0);
  if (impl_->view_->OnMouseEvent(e) != ggadget::EVENT_RESULT_UNHANDLED)
    event->accept();
}

void QtViewWidget::mouseMoveEvent(QMouseEvent* event) {
  impl_->mouseMoveEvent(event);
}

void QtViewWidget::mousePressEvent(QMouseEvent* event) {
  impl_->mousePressEvent(event);
}

void QtViewWidget::mouseReleaseEvent(QMouseEvent *event) {
  impl_->mouseReleaseEvent(event);
}

void QtViewWidget::enterEvent(QEvent *event) {
  if (!impl_->view_) return;
  MouseEvent e(Event::EVENT_MOUSE_OVER,
               0, 0, 0, 0,
               MouseEvent::BUTTON_NONE, 0);
  if (impl_->view_->OnMouseEvent(e) != ggadget::EVENT_RESULT_UNHANDLED)
    event->accept();
}

void QtViewWidget::leaveEvent(QEvent *event) {
  if (!impl_->view_) return;
  MouseEvent e(Event::EVENT_MOUSE_OUT,
               0, 0, 0, 0,
               MouseEvent::BUTTON_NONE, 0);
  if (impl_->view_->OnMouseEvent(e) != ggadget::EVENT_RESULT_UNHANDLED)
    event->accept();
}

void QtViewWidget::wheelEvent(QWheelEvent * event) {
  if (!impl_->view_) return;

  int delta_x = 0, delta_y = 0;
  if (event->orientation() == Qt::Horizontal) {
    delta_x = event->delta();
  } else {
    delta_y = event->delta();
  }
  MouseEvent e(Event::EVENT_MOUSE_WHEEL,
               event->x() / impl_->zoom_, event->y() / impl_->zoom_,
               delta_x, delta_y,
               GetMouseButtons(event->buttons()),
               0);

  if (impl_->view_->OnMouseEvent(e) != EVENT_RESULT_UNHANDLED)
    event->accept();
  else
    event->ignore();
}

void QtViewWidget::keyPressEvent(QKeyEvent *event) {
  if (!impl_->view_) return;

  // For key down event
  EventResult handler_result = ggadget::EVENT_RESULT_UNHANDLED;
  // For key press event
  EventResult handler_result2 = ggadget::EVENT_RESULT_UNHANDLED;

  // Key down event
  int mod = GetModifiers(event->modifiers());
  unsigned int key_code = GetKeyCode(event->key());
  if (key_code) {
    KeyboardEvent e(Event::EVENT_KEY_DOWN, key_code, mod, event);
    handler_result = impl_->view_->OnKeyEvent(e);
  } else {
    LOG("Unknown key: 0x%x", event->key());
  }

  // Key press event
  QString text = event->text();
  if (!text.isEmpty() && !text.isNull()) {
    KeyboardEvent e2(Event::EVENT_KEY_PRESS, text[0].unicode(), mod, event);
    handler_result2 = impl_->view_->OnKeyEvent(e2);
  }

  if (handler_result == ggadget::EVENT_RESULT_UNHANDLED &&
      handler_result2 == ggadget::EVENT_RESULT_UNHANDLED)
    QWidget::keyPressEvent(event);
}

void QtViewWidget::keyReleaseEvent(QKeyEvent *event) {
  if (!impl_->view_) return;

  EventResult handler_result = ggadget::EVENT_RESULT_UNHANDLED;

  int mod = GetModifiers(event->modifiers());
  unsigned int key_code = GetKeyCode(event->key());
  if (key_code) {
    KeyboardEvent e(Event::EVENT_KEY_UP, key_code, mod, event);
    handler_result = impl_->view_->OnKeyEvent(e);
  } else {
    LOG("Unknown key: 0x%x", event->key());
  }

  if (handler_result == ggadget::EVENT_RESULT_UNHANDLED)
    QWidget::keyReleaseEvent(event);
}

// We treat inputMethodEvent as special KeyboardEvent.
void QtViewWidget::inputMethodEvent(QInputMethodEvent *event) {
  if (!impl_->view_) return;
  KeyboardEvent e(Event::EVENT_KEY_DOWN, 0, 0, event);
  if (impl_->view_->OnKeyEvent(e) != ggadget::EVENT_RESULT_UNHANDLED)
    event->accept();
}

void QtViewWidget::dragEnterEvent(QDragEnterEvent *event) {
  impl_->dragEnterEvent(event);
}

void QtViewWidget::dragLeaveEvent(QDragLeaveEvent *event) {
  impl_->dragLeaveEvent(event);
}

void QtViewWidget::dragMoveEvent(QDragMoveEvent *event) {
  impl_->dragMoveEvent(event);
}

void QtViewWidget::dropEvent(QDropEvent *event) {
  impl_->dropEvent(event);
}

void QtViewWidget::resizeEvent(QResizeEvent *event) {
  /**
   * TODO: Only after ggl-plasma gets the layout issue fixed can we apply this
   * procession to view host other than options.
   */
  ViewInterface* view = impl_->view_;
  if (!view ||
      view->GetViewHost()->GetType() != ViewHostInterface::VIEW_HOST_OPTIONS)
    return;
  QSize s = event->size();
  DLOG("resizeEvent: %d, %d", s.width(), s.height());
  double w = s.width();
  double h = s.height();
  if (w == view->GetWidth() && h == view->GetHeight())
    return;
  if (view->OnSizing(&w, &h)) {
    view->SetSize(w, h);
  }
}

void QtViewWidget::focusInEvent(QFocusEvent *event) {
  GGL_UNUSED(event);
  if (!impl_->view_) return;
  SimpleEvent e(Event::EVENT_FOCUS_IN);
  impl_->view_->OnOtherEvent(e);
}

void QtViewWidget::focusOutEvent(QFocusEvent *event) {
  GGL_UNUSED(event);
  if (!impl_->view_) return;
  SimpleEvent e(Event::EVENT_FOCUS_OUT);
  impl_->view_->OnOtherEvent(e);
}

QSize QtViewWidget::sizeHint() const {
  ViewInterface* view = impl_->view_;
  if (!view) return QWidget::sizeHint();
  double zoom = impl_->zoom_;
  int w = D2I(view->GetWidth() * zoom);
  int h = D2I(view->GetHeight() * zoom);
  if (w == 0 || h == 0) {
    double dw, dh;
    view->GetDefaultSize(&dw, &dh);
    w = D2I(dw * zoom);
    h = D2I(dh * zoom);
  }

  DLOG("sizeHint: %d, %d", w, h);
  return QSize(w, h);
}

void QtViewWidget::EnableInputShapeMask(bool enable) {
  impl_->EnableInputShapeMask(enable);
}

void QtViewWidget::AdjustToViewSize() {
  impl_->AdjustToViewSize();
}

void QtViewWidget::SetKeepAbove(bool above) {
  Qt::WindowFlags f = windowFlags();
  if (above)
    f |= Qt::WindowStaysOnTopHint;
  else
    f &= ~Qt::WindowStaysOnTopHint;
  setWindowFlags(f);
  SetUndecoratedWMProperties();
  show();
}

void QtViewWidget::SetView(ViewInterface *view) {
  if (impl_->view_ != view) {
    impl_->view_ = view;
    if (view) {
      impl_->zoom_ = view->GetGraphics()->GetZoom();
      impl_->AdjustToViewSize();
    }
  }
}

#if defined(Q_WS_X11) && defined(HAVE_X11)
static void SetWMState(Window w, const char *property_name) {
  Display *dpy = QX11Info::display();
  Atom property = XInternAtom(dpy, property_name, False);
  Atom net_wm_state = XInternAtom(dpy, "_NET_WM_STATE", False);
  XChangeProperty(dpy, w, net_wm_state, XA_ATOM, 32, PropModeAppend,
                  (unsigned char *)&property, 1);
}
#endif

void QtViewWidget::SetUndecoratedWMProperties() {
#if defined(Q_WS_X11) && defined(HAVE_X11)
  SetWMState(winId(), "_NET_WM_STATE_SKIP_TASKBAR");
  SetWMState(winId(), "_NET_WM_STATE_SKIP_PAGER");

  // Shows on all desktops
  Display *dpy = QX11Info::display();
  int32_t desktop = -1;
  Atom property = XInternAtom(dpy, "_NET_WM_DESKTOP", False);
  XChangeProperty(dpy, winId(), property, XA_CARDINAL, 32, PropModeReplace,
                  (unsigned char *)&desktop, 1);
#endif
}

void QtViewWidget::SetChild(QWidget *widget) {
  if (impl_->child_) {
    impl_->child_->setParent(NULL);
  }
  impl_->child_ = widget;
  if (widget) {
    widget->setParent(this);
    // this will expose parent widget so its paintEvent will be triggered.
    widget->move(0, 10);
  }
}

void QtViewWidget::UnsetMinimumSizeHint() {
  setMinimumSize(0, 0);
}

}
}
#include "qt_view_widget.moc"
