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

#ifndef GGADGET_TESTS_MOCKED_VIEW_HOST_H__
#define GGADGET_TESTS_MOCKED_VIEW_HOST_H__

#include "ggadget/graphics_interface.h"
#include "ggadget/canvas_interface.h"
#include "ggadget/clip_region.h"
#include "ggadget/view_interface.h"
#include "ggadget/view_host_interface.h"

using namespace ggadget;

class MockedCanvas : public ggadget::CanvasInterface {
 public:
  MockedCanvas(double w, double h) : w_(w), h_(h) { }
  virtual void Destroy() { delete this; }
  virtual double GetWidth() const { return w_; }
  virtual double GetHeight() const { return h_; }
  virtual bool PushState() { return true; }
  virtual bool PopState() { return true; }
  virtual bool MultiplyOpacity(double opacity) { return true; }
  virtual void RotateCoordinates(double radians) { }
  virtual void TranslateCoordinates(double dx, double dy) { }
  virtual void ScaleCoordinates(double cx, double cy) { }
  virtual bool ClearCanvas() { return true; }
  virtual bool DrawLine(double x0, double y0, double x1, double y1,
                        double width, const ggadget::Color &c) {
    return true;
  }
  virtual bool DrawFilledRect(double x, double y,
                              double w, double h, const ggadget::Color &c) {
    return true;
  }
  virtual bool ClearRect(double x, double y, double w, double h) {
    return true;
  }
  virtual bool DrawCanvas(double x, double y, const CanvasInterface *img) {
    return true;
  }
  virtual bool DrawRawImage(double x, double y,
                            const char *data, RawImageFormat format,
                            int width, int height, int stride) {
    return true;
  }
  virtual bool DrawFilledRectWithCanvas(double x, double y,
                                        double w, double h,
                                        const CanvasInterface *img) {
    return true;
  }
  virtual bool DrawCanvasWithMask(double x, double y,
                                  const CanvasInterface *img,
                                  double mx, double my,
                                  const CanvasInterface *mask) {
    return true;
  }
  virtual bool DrawText(double x, double y, double width, double height,
                        const char *text, const ggadget::FontInterface *f,
                        const ggadget::Color &c, Alignment align,
                        VAlignment valign, Trimming trimming, int text_flags) {
    return true;
  }
  virtual bool DrawTextWithTexture(double x, double y, double width,
                                   double height, const char *text,
                                   const ggadget::FontInterface *f,
                                   const CanvasInterface *texture,
                                   Alignment align, VAlignment valign,
                                   Trimming trimming, int text_flags) {
    return true;
  }
  virtual bool IntersectRectClipRegion(double x, double y,
                                       double w, double h) {
    return true;
  }
  virtual bool IntersectGeneralClipRegion(const ClipRegion &region) {
    return true;
  }
  virtual bool GetTextExtents(const char *text, const ggadget::FontInterface *f,
                              int text_flags, double in_width,
                              double *width, double *height) {
    return false;
  }
  virtual bool GetPointValue(double x, double y,
                             ggadget::Color *color, double *opacity) const {
    return false;
  }
 private:
  double w_, h_;
};

class MockedGraphics : public ggadget::GraphicsInterface {
 public:
  virtual ggadget::CanvasInterface *NewCanvas(double w, double h) const {
    return new MockedCanvas(w, h);
  }
  virtual ggadget::ImageInterface *NewImage(const std::string &tag,
                                            const std::string &data,
                                            bool is_mask) const {
      return NULL;
  }
  virtual ggadget::FontInterface *NewFont(
      const std::string &family, double pt_size,
      ggadget::FontInterface::Style style,
      ggadget::FontInterface::Weight weight) const {
    return NULL;
  }
  virtual ggadget::TextRendererInterface *NewTextRenderer() const {
    return NULL;
  }
  virtual double GetZoom() const {
    return 1.;
  }
  virtual void SetZoom(double z) { }
  virtual ggadget::Connection *ConnectOnZoom(
      ggadget::Slot1<void, double>* slot) const {
    return NULL;
  }
};

class MockedViewHost : public ggadget::ViewHostInterface {
 public:
  MockedViewHost(ViewHostInterface::Type type)
      : type_(type),
        view_(NULL),
        draw_queued_(false),
        resize_queued_(false) {
  }
  virtual ~MockedViewHost() {
  }
  virtual Type GetType() const { return type_; }
  virtual void Destroy() { delete this; }
  virtual void SetView(ggadget::ViewInterface *view) { view_ = view; }
  virtual ggadget::ViewInterface *GetView() const { return view_; }
  virtual ggadget::GraphicsInterface *NewGraphics() const {
    return new MockedGraphics;
  }
  virtual void *GetNativeWidget() const { return NULL; }
  virtual void ViewCoordToNativeWidgetCoord(double, double,
                                            double *, double *) const { }
  virtual void NativeWidgetCoordToViewCoord(double, double,
                                            double *, double *) const { }
  virtual void QueueDraw() { draw_queued_ = true; }
  virtual void QueueResize() { resize_queued_ = true; }
  virtual void EnableInputShapeMask(bool enable) { };
  virtual void SetResizable(ggadget::ViewInterface::ResizableMode mode) { }
  virtual void SetCaption(const std::string &caption) { }
  virtual void SetShowCaptionAlways(bool always) { }
  virtual void SetCursor(ViewInterface::CursorType type) { }
  virtual void ShowTooltip(const std::string &tooltip) { }
  virtual void ShowTooltipAtPosition(const std::string &tooltip,
                                     double x, double y) { }
  virtual bool ShowView(bool modal, int flags,
                        ggadget::Slot1<bool, int> *feedback_handler) {
    delete feedback_handler;
    return false;
  }
  virtual void CloseView() { }
  virtual bool ShowContextMenu(int button) { return false; }
  virtual void Alert(const ViewInterface *view, const char *message) { }
  virtual ConfirmResponse Confirm(const ViewInterface *view,
                                  const char *message, bool cancel_button) {
    return CONFIRM_NO;
  }
  virtual std::string Prompt(const ViewInterface *view, const char *message,
                             const char *default_value) {
    return std::string();
  }
  virtual int GetDebugMode() const {
    return ggadget::ViewInterface::DEBUG_DISABLED;
  }
  virtual void GetWindowPosition(int *x, int *y) { }
  virtual void SetWindowPosition(int x, int y) { }
  virtual void GetWindowSize(int *width, int *height) { };
  virtual void SetFocusable(bool focusable) { };
  virtual void SetOpacity(double opacity) { };
  virtual void SetFontScale(double scale) { };
  virtual void SetZoom(double zoom) { };
  virtual void BeginResizeDrag(int, ggadget::ViewInterface::HitTest) { }
  virtual void BeginMoveDrag(int) { }
  virtual ggadget::Connection *ConnectOnEndMoveDrag(
      ggadget::Slot2<void, int, int> *slot) {
    return NULL;
  };
  virtual ggadget::Connection *ConnectOnShowContextMenu(
      ggadget::Slot1<bool, MenuInterface*> *slot) {
    return NULL;
  };

  bool GetQueuedDraw() {
    bool b = draw_queued_;
    draw_queued_ = false;
    ggadget::CanvasInterface *canvas = new MockedCanvas(100, 100);
    view_->Layout();
    view_->Draw(canvas);
    canvas->Destroy();
    return b;
  }
  bool GetQueueResize() {
    bool b = resize_queued_;
    resize_queued_ = false;
    return b;
  }

 private:
  Type type_;
  ggadget::ViewInterface *view_;
  bool draw_queued_;
  bool resize_queued_;
};

#endif // GGADGET_TESTS_MOCKED_VIEW_HOST_H__
