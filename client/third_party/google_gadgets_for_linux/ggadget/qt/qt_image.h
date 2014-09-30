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

#ifndef GGADGET_QT_QT_IMAGE_H__
#define GGADGET_QT_QT_IMAGE_H__

#include <QtGui/QPixmap>
#include <ggadget/common.h>
#include <ggadget/color.h>
#include <ggadget/canvas_interface.h>
#include <ggadget/image_interface.h>

namespace ggadget {
namespace qt {
class QtGraphics;
/**
 * This class realizes the ImageInterface using Qt's QPixmap
 */
class QtImage : public ImageInterface {
 public:
  QtImage(QtGraphics *g, const std::string &tag,
          const std::string &data, bool is_mask);
  /** Check if the QtImage object is valid. */
  bool IsValid() const;

 public:
  virtual ~QtImage();

  virtual void Destroy();
  virtual const CanvasInterface *GetCanvas() const;
  virtual void Draw(CanvasInterface *canvas, double x, double y) const;
  virtual void StretchDraw(CanvasInterface *canvas,
                           double x, double y,
                           double width, double height) const;
  virtual double GetWidth() const;
  virtual double GetHeight() const;
  virtual ImageInterface* MultiplyColor(const Color &color) const;
  virtual bool GetPointValue(double x, double y,
                             Color *color, double *opacity) const;
  virtual std::string GetTag() const;
  virtual bool IsFullyOpaque() const;

 private:
  QtImage(double width, double height);
  class Impl;
  Impl *impl_;

  DISALLOW_EVIL_CONSTRUCTORS(QtImage);
};

} // namespace qt
} // namespace ggadget

#endif // GGADGET_QT_QT_IMAGE_H__
