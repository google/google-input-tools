/*
  CopyrighT 2013 GOOGLE INC.

  LICensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

#include "ggadget/mac/single_view_host.h"

#import <AppKit/AppKit.h>
#import <Quartz/Quartz.h>

#include "ggadget/mac/content_view.h"
#include "ggadget/mac/quartz_canvas.h"
#include "ggadget/mac/quartz_graphics.h"
#include "ggadget/mac/scoped_cftyperef.h"

namespace ggadget {
namespace mac {

class SingleViewHost::Impl {
 public:
  Impl() : window_(NULL),
           content_view_(NULL),
           view_(NULL),
           view_size_changed_(false),
           canvas_changed_(false),
           frame_rect_changed_(false) {
  }

  ~Impl() {
  }

  void Detach() {
    if (window_ != NULL) {
      [window_ close];
      content_view_ = NULL;
      window_ = NULL;
    }
  }

  void CreateWindowForView() {
    ASSERT(view_ != NULL);
    NSRect rect;
    rect.origin.x = 0;
    rect.origin.y = 0;
    rect.size = GetSizeFromView(true /* get_default_size */);

    // Create new content view
    content_view_ = [[ContentView alloc] initWithFrame:rect
                                               content:view_
                                              drawFunc:NewSlot(this,
                                                               &Impl::Draw)];
    [content_view_ setCursor:cursor_];
    window_ = [[NSWindow alloc] initWithContentRect:rect
                                          styleMask:NSBorderlessWindowMask
                                            backing:NSBackingStoreBuffered
                                              defer:YES];
    [window_ setOpaque:NO];
    [window_ setContentView:content_view_];
    [window_ makeKeyWindow];
    [window_ enableCursorRects];
    [window_ invalidateCursorRectsForView:content_view_];
  }

  void CreateCanvasForView() {
    canvas_.reset(down_cast<QuartzCanvas*>(
        view_->GetGraphics()->NewCanvas(view_->GetWidth(),
                                        view_->GetHeight())));
    canvas_changed_ = true;
  }

  NSSize GetSizeFromView(bool get_default_size) {
    double zoom = view_->GetGraphics()->GetZoom();
    NSSize size;
    if (get_default_size) {
      double w, h;
      view_->GetDefaultSize(&w, &h);
      size.width = w * zoom;
      size.height = h * zoom;
    } else {
      size.width = view_->GetWidth() * zoom;
      size.height = view_->GetHeight() * zoom;
    }
    return size;
  }

  bool UpdateViewSize() {
    view_size_changed_ = false;
    view_->Layout();
    NSSize new_size = GetSizeFromView(false /* get_default_size */);
    int dx = static_cast<int>(round(new_size.width - view_size_.width));
    int dy = static_cast<int>(round(new_size.height - view_size_.height));

    if (dx != 0 || dy != 0) {
      canvas_.reset(NULL);
      view_size_ = new_size;
      return true;
    }
    return false;
  }

  void UpdateCanvasIfNeeded() {
    if (canvas_.get() == NULL) {
      CreateCanvasForView();
    }
    if (canvas_changed_) {
      UpdateCanvas();
    }
  }

  void UpdateCanvas() {
    canvas_->ClearCanvas();
    view_->Draw(canvas_.get());
    canvas_changed_ = false;
  }

  void UpdateWindowFrame() {
    if (frame_rect_changed_) {
      NSRect content_rect;
      content_rect.origin = NSMakePoint(0.0, 0.0);
      content_rect.size = GetSizeFromView(false /* get_default_size */);
      NSRect frame_rect = [window_ frame];

      frame_rect.size = [window_ frameRectForContentRect:content_rect].size;
      frame_rect.origin = GetOriginToEnsureInScreen(frame_rect.origin,
                                                    frame_rect.size);
      [window_ setFrame:frame_rect display:NO];
      frame_rect_changed_ = false;
    }
  }

  void SetWindowOrigin(double x, double y) {
    NSPoint origin = GetOriginToEnsureInScreen(NSMakePoint(x, y),
                                               [window_ frame].size);
    [window_ setFrameOrigin:origin];
  }

  NSPoint GetOriginToEnsureInScreen(NSPoint origin, NSSize size) {
    NSScreen *screen = [window_ screen];
    NSRect screen_frame = [screen visibleFrame];
    CGFloat screen_max_x = screen_frame.origin.x + screen_frame.size.width;
    CGFloat screen_max_y = screen_frame.origin.y + screen_frame.size.height;

    origin.x = std::max(screen_frame.origin.x,
                        std::min(origin.x, screen_max_x - size.width));
    origin.y = std::max(screen_frame.origin.y,
                        std::min(origin.y, screen_max_y - size.height));
    return origin;
  }

  void Draw() {
    if (view_size_changed_) {
      frame_rect_changed_ = UpdateViewSize() || frame_rect_changed_;
    }
    UpdateCanvasIfNeeded();
    UpdateWindowFrame();
    CGContextRef context = reinterpret_cast<CGContextRef>(
        [[NSGraphicsContext currentContext] graphicsPort]);
    CGContextClearRect(context, NSRectToCGRect([content_view_ frame]));

    // Draw image
    // Resize to frame
    ScopedCFTypeRef<CGImageRef> image(canvas_->CreateImage());
    CGContextDrawImage(context,
                       NSRectToCGRect([content_view_ frame]),
                       image.get());
  }

  void SetCursor(NSCursor *cursor) {
    cursor_ = cursor;
    if (content_view_) {
      [content_view_ setCursor:cursor];
      [window_ invalidateCursorRectsForView:content_view_];
    }
  }

  NSWindow *window_;
  ContentView *content_view_;
  bool frame_rect_changed_;
  ViewInterface *view_;
  NSSize view_size_;
  bool view_size_changed_;
  bool canvas_changed_;
  scoped_ptr<QuartzCanvas> canvas_;
  NSCursor *cursor_;
};

SingleViewHost::SingleViewHost() : impl_(new Impl) {
}

SingleViewHost::~SingleViewHost() {
}

ViewHostInterface::Type SingleViewHost::GetType() const {
  // We only support VIEW_HOST_MAIN
  return ViewHostInterface::VIEW_HOST_MAIN;
}

void SingleViewHost::Destroy() {
  delete this;
}

void SingleViewHost::SetView(ViewInterface *view) {
  if (impl_->view_ == view) {
    return;
  }
  impl_->Detach();
  if (!view) {
    return;
  }

  // Create a new window with view
  impl_->view_ = view;
  impl_->UpdateViewSize();
  impl_->CreateWindowForView();
}

ViewInterface *SingleViewHost::GetView() const {
  return impl_->view_;
}

GraphicsInterface *SingleViewHost::NewGraphics() const {
  return new QuartzGraphics(/* zoom */ 1.0);
}

void *SingleViewHost::GetNativeWidget() const {
  if (!impl_->window_) {
    return NULL;
  }
  return (__bridge void*) impl_->window_;
}

void SingleViewHost::ViewCoordToNativeWidgetCoord(
    double x, double y, double *widget_x, double *widget_y) const {
  // TODO: implement this.
}

void SingleViewHost::NativeWidgetCoordToViewCoord(
    double x, double y, double *view_x, double *view_y) const {
  // TODO: implement this.
}

void SingleViewHost::QueueDraw() {
  impl_->view_size_changed_ = true;
  impl_->canvas_changed_ = true;
  if (impl_->content_view_ != NULL) {
    // setNeedsDisplay: is not thread-safe. We need to make sure it is called in
    // the main thread.
    dispatch_queue_t main_queue = dispatch_get_main_queue();
    dispatch_async(main_queue, ^{
      [impl_->content_view_ setNeedsDisplay:YES];
    });
  }
}

void SingleViewHost::QueueResize() {
  QueueDraw();
}

void SingleViewHost::EnableInputShapeMask(bool enable) {
  // TODO: implement this.
}

void SingleViewHost::SetResizable(ViewInterface::ResizableMode mode) {
  // TODO: implement this.
}

void SingleViewHost::SetCaption(const std::string &caption) {
  if (impl_->window_ != NULL) {
    [impl_->window_ setTitle:[NSString stringWithUTF8String:caption.c_str()]];
  }
}

void SingleViewHost::SetShowCaptionAlways(bool always) {
  GGL_UNUSED(always);
  // Not supported
}

void SingleViewHost::SetCursor(ViewInterface::CursorType type) {
  NSCursor *cursor = nil;
  switch (type) {
    case ViewInterface::CURSOR_DEFAULT:
    case ViewInterface::CURSOR_ARROW:
      cursor = [NSCursor arrowCursor];
      break;
    case ViewInterface::CURSOR_IBEAM:
      cursor = [NSCursor IBeamCursor];
      break;
    case ViewInterface::CURSOR_WAIT:
      // Cocoa only shows wait cursor when app is frozen
      break;
    case ViewInterface::CURSOR_CROSS:
      cursor = [NSCursor crosshairCursor];
      break;
    case ViewInterface::CURSOR_UPARROW:
      // FIXME: use a custom image for this?
      break;
    case ViewInterface::CURSOR_SIZE:
      // FIXME: load custom image
      break;
    case ViewInterface::CURSOR_SIZENWSE:
      // FIXME: load custom image
      break;
    case ViewInterface::CURSOR_SIZENESW:
      // FIXME: load custom image
      break;
    case ViewInterface::CURSOR_SIZEWE:
      cursor = [NSCursor resizeLeftRightCursor];
      break;
    case ViewInterface::CURSOR_SIZENS:
      cursor = [NSCursor resizeUpDownCursor];
      break;
    case ViewInterface::CURSOR_SIZEALL:
      // FIXME: load custom image
      break;
    case ViewInterface::CURSOR_NO:
      // FIXME: load custom image
      break;
    case ViewInterface::CURSOR_HAND:
      cursor = [NSCursor pointingHandCursor];
      break;
    case ViewInterface::CURSOR_BUSY:
      // FIXME: load custom image
      break;
    case ViewInterface::CURSOR_HELP:
      // FIXME: load custom image
      break;
  }
  if (cursor == nil) {
    // Sets the not supported cursor types to default arrow cursor
    cursor = [NSCursor arrowCursor];
  }
  impl_->SetCursor(cursor);
}

void SingleViewHost::ShowTooltip(const std::string &tooltip) {
  // TODO: implement this.
}

void SingleViewHost::ShowTooltipAtPosition(const std::string &tooltip,
                                           double x, double y) {
  // TODO: implement this.
}

bool SingleViewHost::ShowView(bool modal, int flags,
                              Slot1<bool, int> *feedback_handler) {
  ASSERT(impl_->window_);
  ASSERT(impl_->view_);

  GGL_UNUSED(modal);
  GGL_UNUSED(flags);
  GGL_UNUSED(feedback_handler);
  QueueDraw();
  [impl_->window_ orderFront:nil];
  return true;
}

void SingleViewHost::CloseView() {
  if (impl_->window_) {
    [impl_->window_ close];
  }
}

bool SingleViewHost::ShowContextMenu(int button) {
  // TODO: implement this.
  return true;
}

void SingleViewHost::BeginResizeDrag(int button,
                                     ViewInterface::HitTest hittest) {
  // TODO: implement this.
}

void SingleViewHost::BeginMoveDrag(int button) {
  [impl_->content_view_ beginMoveDrag];
}

void SingleViewHost::Alert(const ViewInterface *view, const char *message) {
}

ViewHostInterface::ConfirmResponse SingleViewHost::Confirm(
    const ViewInterface *view,
    const char *message,
    bool cancel_button) {
  return CONFIRM_CANCEL;
}

std::string SingleViewHost::Prompt(const ViewInterface *view,
                                   const char *message,
                                   const char *default_value) {
  // Not supported
  return "";
}

int SingleViewHost::GetDebugMode() const {
  // Not supported
  return 0;
}

void SingleViewHost::SetWindowOrigin(double x, double y) {
  impl_->SetWindowOrigin(x, y);
}

} // namespace mac
} // namespace ggadget
