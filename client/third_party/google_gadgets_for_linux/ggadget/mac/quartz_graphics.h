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

#ifndef GGADGET_MAC_QUARTZ_GRAPHICS_H_
#define GGADGET_MAC_QUARTZ_GRAPHICS_H_

#include <string>

#include "ggadget/common.h"
#include "ggadget/graphics_interface.h"
#include "ggadget/scoped_ptr.h"
#include "ggadget/slot.h"

namespace ggadget {

class TextFormat;

} // namespace ggadget

namespace ggadget {
namespace mac {

class CtFont;

// This class realizes the GraphicsInterface using the Quartz graphics library.
class QuartzGraphics : public GraphicsInterface {
 public:
  explicit QuartzGraphics(double zoom);
  virtual ~QuartzGraphics();
  virtual CanvasInterface* NewCanvas(double w, double h) const;
  virtual ImageInterface* NewImage(const std::string& tag,
                                   const std::string& data,
                                   bool is_mask) const;
  virtual FontInterface* NewFont(const std::string &family,
                                 double pt_size,
                                 FontInterface::Style style,
                                 FontInterface::Weight weight) const;
  virtual TextRendererInterface* NewTextRenderer() const;
  virtual void SetZoom(double zoom);
  virtual double GetZoom() const;
  virtual Connection* ConnectOnZoom(Slot1<void, double>* slot) const;
  FontInterface* NewFont(const TextFormat &format) const;

 private:
  class Impl;
  scoped_ptr<Impl> impl_;
  DISALLOW_EVIL_CONSTRUCTORS(QuartzGraphics);
};

} // namespace mac
} // namespace ggadget

#endif // GGADGET_MAC_QUARTZ_GRAPHICS_H_
