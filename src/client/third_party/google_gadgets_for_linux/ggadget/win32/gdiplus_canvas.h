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

// The file contains the implementation of the canvas Interface on Gdi+.


#ifndef GGADGET_WIN32_GDIPLUS_CANVAS_H_
#define GGADGET_WIN32_GDIPLUS_CANVAS_H_

#include <string>
#include <ggadget/common.h>
#include <ggadget/scoped_ptr.h>
#ifdef DrawText
#undef DrawText
#endif
#include <ggadget/canvas_interface.h>

namespace Gdiplus {
class Bitmap;
class Graphics;
}  // namespace Gdiplus

namespace ggadget {
namespace win32 {

class GdiplusGraphics;

class GdiplusCanvas : public CanvasInterface {
 public:
  GdiplusCanvas();

  // Initializes the object with repect to the GdiplusGraphics object with size
  // w * h. If we need to draw on the canvas, set create_graphics true so that
  // we create a Gdiplus::Graphics object.
  bool Init(const GdiplusGraphics *g,
            double w, double h,
            bool create_graphics);

  // Initializes the object using the data stored in the string, the data is the
  // raw bytes of an image file with the size in data.size().
  // If we need to draw on the canvas, set create_graphics true so that we
  // create a Gdiplus::Graphics object.
  bool Init(const std::string &data, bool create_graphics);

  virtual ~GdiplusCanvas();
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
  // Checks if the canvas is valid.
  bool IsValid() const;
  // Returns the pointer to the Gdiplus Bitmap of the canvas
  Gdiplus::Bitmap* GetImage() const;
  // Returns the pointer to the Gdiplus Graphics.
  Gdiplus::Graphics *GetGdiplusGraphics() const;
  // Returns the zoom ratio.
  double GetZoom() const;
  // Returns the current opacity.
  double GetOpacity() const;

 private:
  class Impl;
  scoped_ptr<Impl> impl_;

  DISALLOW_EVIL_CONSTRUCTORS(GdiplusCanvas);
};

}  // namespace win32
}  // namespace ggadget

#endif  // GGADGET_WIN32_GDIPLUS_CANVAS_H_

