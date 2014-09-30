/*
  Copyright 2013 Google Inc.

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

#ifndef GGADGET_MAC_QUARTZ_IMAGE_H_
#define GGADGET_MAC_QUARTZ_IMAGE_H_

#include "ggadget/image_interface.h"
#include "ggadget/scoped_ptr.h"

namespace ggadget {
namespace mac {

class QuartzImage : public ImageInterface {
 public:
  QuartzImage();
  bool Init(const std::string& tag, const std::string& data, bool is_mask);
  bool Init(int width, int height);
  bool IsValid() const;
  virtual ~QuartzImage();
  virtual void Destroy();
  virtual const CanvasInterface *GetCanvas() const;
  virtual void Draw(CanvasInterface *canvas, double x, double y) const;
  virtual void StretchDraw(CanvasInterface *canvas,
                           double x, double y,
                           double width, double height) const;
  virtual double GetWidth() const;
  virtual double GetHeight() const;
  virtual ImageInterface *MultiplyColor(const Color &color) const;
  virtual bool GetPointValue(double x, double y,
                             Color *color, double *opacity) const;
  virtual std::string GetTag() const;
  virtual bool IsFullyOpaque() const;

 private:
  class Impl;
  scoped_ptr<Impl> impl_;
};

} // namespace mac
} // namespace ggadget

#endif // GGADGET_MAC_QUARTZ_IMAGE_H_
