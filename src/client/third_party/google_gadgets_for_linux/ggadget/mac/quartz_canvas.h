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

// The file contains the implementation of the canvas interface on Quartz.


#ifndef GGADGET_MAC_QUARTZ_CANVAS_
#define GGADGET_MAC_QUARTZ_CANVAS_

#include <Quartz/Quartz.h>

#include "ggadget/canvas_interface.h"
#include "ggadget/mac/quartz_graphics.h"
#include "ggadget/scoped_ptr.h"

namespace ggadget {
namespace mac {

// This class realizes the CanvasInterface using the Quartz graphics library.
class QuartzCanvas : public CanvasInterface {
 public:
  QuartzCanvas();
  virtual ~QuartzCanvas();
  bool Init(const QuartzGraphics *graphics, double w, double h,
            RawImageFormat rawImageFormat);
  bool Init(const QuartzGraphics *graphics, double w, double h,
            CGContextRef context);
  virtual void Destroy();
  virtual double GetWidth() const;
  virtual double GetHeight() const;
  virtual bool PushState();
  virtual bool PopState();
  virtual bool MultiplyOpacity(double opacity);
  virtual void RotateCoordinates(double radians);
  virtual void TranslateCoordinates(double dx, double dy);
  virtual void ScaleCoordinates(double cx, double cy);
  virtual bool ClearCanvas();
  virtual bool ClearRect(double x, double y, double w, double h);
  virtual bool DrawLine(double x0, double y0, double x1, double y1,
                        double width, const Color &c);
  virtual bool DrawFilledRect(double x, double y,
                              double w, double h, const Color &c);
  virtual bool DrawCanvas(double x, double y, const CanvasInterface *img);
  virtual bool DrawRawImage(double x, double y,
                            const char *data, RawImageFormat format,
                            int width, int height, int stride);
  virtual bool DrawFilledRectWithCanvas(double x, double y,
                                        double w, double h,
                                        const CanvasInterface *img);
  virtual bool DrawCanvasWithMask(double x, double y,
                                  const CanvasInterface *img,
                                  double mx, double my,
                                  const CanvasInterface *mask);
  virtual bool DrawText(double x, double y, double width, double height,
                        const char *text, const FontInterface *f,
                        const Color &c, Alignment align, VAlignment valign,
                        Trimming trimming, int text_flags);
  virtual bool DrawTextWithTexture(double x, double y, double width,
                                   double height, const char *text,
                                   const FontInterface *f,
                                   const CanvasInterface *texture,
                                   Alignment align, VAlignment valign,
                                   Trimming trimming, int text_flags);
  virtual bool IntersectRectClipRegion(double x, double y,
                                       double w, double h);
  virtual bool IntersectGeneralClipRegion(const ClipRegion &region);
  virtual bool GetTextExtents(const char *text, const FontInterface *f,
                              int text_flags, double in_width,
                              double *width, double *height);
  virtual bool GetPointValue(double x, double y,
                             Color *color, double *opacity) const;

  /** Checks if the canvas is valid */
  bool IsValid() const;

  /** Get the zoom factor. */
  double GetZoom() const;

  /** Creates a image from bitmap context **/
  CGImageRef CreateImage() const;

  CGContextRef GetContext() const;

  void SetBlendMode(CGBlendMode mode);

 private:
  class Impl;
  scoped_ptr<Impl> impl_;

  DISALLOW_EVIL_CONSTRUCTORS(QuartzCanvas);
};

} // namespace mac
} // namespace ggadget

#endif // GGADGET_MAC_QUARTZ_CANVAS_
