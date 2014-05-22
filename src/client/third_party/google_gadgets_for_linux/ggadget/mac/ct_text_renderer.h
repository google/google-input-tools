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

#ifndef GGDAGET_MAC_CT_TEXT_RENDERER_H_
#define GGDAGET_MAC_CT_TEXT_RENDERER_H_

#include <string>
#include <vector>

#include "ggadget/scoped_ptr.h"
#include "ggadget/text_formats.h"
#include "ggadget/text_renderer_interface.h"

namespace ggadget {

class CanvasInterface;

} // namespace ggadget

namespace ggadget {
namespace mac {

class QuartzGraphics;

class CtTextRenderer : public TextRendererInterface {
 public:
  explicit CtTextRenderer(const QuartzGraphics* graphics);
  virtual ~CtTextRenderer();
  virtual void Destroy();
  virtual void SetTextAndFormat(const std::string& text,
                                const TextFormats& formats);
  virtual void SetAlignment(CanvasInterface::Alignment align);
  virtual void SetVAlignment(CanvasInterface::VAlignment valign);
  virtual void SetWordWrap(bool word_wrap);
  virtual void SetLayoutRectangle(double x, double y,
                                  double width, double height);
  virtual void SetTrimming(CanvasInterface::Trimming trimming);
  virtual bool DrawText(CanvasInterface* canvas);
  virtual bool DrawTextWithTexture(const CanvasInterface* texture,
                                   CanvasInterface* canvas);
  virtual bool GetTextExtents(double* width, double* height);
  virtual int GetTextRangeBoundingBoxes(
      const Range& range,
      std::vector<Rectangle>* bounding_boxes);
  virtual void SetDefaultFormat(const TextFormat& default_format);
  virtual void DrawCaret(CanvasInterface* canvas,
                         int caret_pos,
                         const Color& color);

 private:
  class Impl;
  scoped_ptr<Impl> impl_;
  DISALLOW_EVIL_CONSTRUCTORS(CtTextRenderer);
};

} // namespace mac
} // namespace ggadget

#endif // GGDAGET_MAC_CT_TEXT_RENDERER_H_
