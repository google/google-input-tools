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

#ifndef GGADGET_GTK_CAIRO_IMAGE_BASE_H__
#define GGADGET_GTK_CAIRO_IMAGE_BASE_H__

#include <ggadget/common.h>
#include <ggadget/image_interface.h>

namespace ggadget {
namespace gtk {

class CairoGraphics;
class CairoCanvas;

/**
 * This class is the base class for @c ImageInterface implementations.
 * This class provides default drawing methods implmenetations using the
 * result of GetCanvas(). These methods should be overriden if more
 * sophisticated drawing needed.
 */
class CairoImageBase : public ImageInterface {
 public:
  CairoImageBase(const std::string &tag, bool is_mask);

 protected:
  virtual ~CairoImageBase();

 public:
  /** Checks if this image object is valid. */
  virtual bool IsValid() const = 0;

 public: // Interface methods
  virtual void Destroy();
  virtual void Draw(CanvasInterface *canvas, double x, double y) const;
  virtual void StretchDraw(CanvasInterface *canvas,
                           double x, double y,
                           double width, double height) const;
  virtual ImageInterface *MultiplyColor(const Color &color) const;
  virtual bool GetPointValue(double x, double y,
                             Color *color, double *opacity) const;
  virtual std::string GetTag() const;

 private:
  class Impl;
  Impl *impl_;

  DISALLOW_EVIL_CONSTRUCTORS(CairoImageBase);
};

} // namespace gtk
} // namespace ggadget

#endif // GGADGET_GTK_CAIRO_IMAGE_BASE_H__
