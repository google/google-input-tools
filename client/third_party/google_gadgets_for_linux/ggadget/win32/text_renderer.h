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

#ifndef GGADGET_WIN32_TEXT_RENDERER_H__
#define GGADGET_WIN32_TEXT_RENDERER_H__

#include <string>
#include <vector>
#include "ggadget/canvas_interface.h"
#include "ggadget/scoped_ptr.h"
#include "ggadget/text_formats.h"
#include "ggadget/text_renderer_interface.h"

#undef DrawText

namespace ggadget {
namespace win32 {
class GdiplusGraphics;

// TextRenderer is the Windows implementation of TextRendererInterface using
// Uniscribe.
// See
// http://msdn.microsoft.com/en-us/library/windows/desktop/dd317792(v=vs.85).aspx
// for reference. The only difference between the document and this
// implementation is that we use GDI+ instead of GDI to draw the glyphs.
class TextRenderer : public TextRendererInterface {
 public:
  // Constructs a TextRenderer. TextRenderer will not own |graphics|.
  explicit TextRenderer(const GdiplusGraphics* graphics);
  ~TextRenderer();
  virtual void Destroy();
  virtual void SetTextAndFormat(const std::string& text,
                                const TextFormats& formats);
  virtual void SetAlignment(CanvasInterface::Alignment align);
  virtual void SetVAlignment(CanvasInterface::VAlignment valign);
  virtual void SetWordWrap(bool word_wrap);
  virtual void SetLayoutRectangle(double x, double y,
                                  double width, double height);
  virtual void SetTrimming(CanvasInterface::Trimming trimming);
  virtual void SetDefaultFormat(const TextFormat& default_format);
  virtual bool DrawText(CanvasInterface* canvas);
  virtual bool DrawTextWithTexture(const CanvasInterface* texture,
                                   CanvasInterface* canvas);
  virtual bool GetTextExtents(double* width, double* height);
  virtual int GetTextRangeBoundingBoxes(const Range& range,
                                        std::vector<Rectangle>* bounding_boxes);
  virtual void DrawCaret(CanvasInterface* canvas,
                         int caret_pos,
                         const Color& color);

 private:
  class Impl;
  scoped_ptr<Impl> impl_;
  DISALLOW_EVIL_CONSTRUCTORS(TextRenderer);
};

}  // namespace win32
}  // namespace ggadget
#endif  // GGADGET_WIN32_TEXT_RENDERER_H__
