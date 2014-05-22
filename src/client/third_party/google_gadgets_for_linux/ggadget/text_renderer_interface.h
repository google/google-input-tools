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

#ifndef GGADGET_TEXT_RENDERER_INTERFACE_H__
#define GGADGET_TEXT_RENDERER_INTERFACE_H__

#include "ggadget/canvas_interface.h"
#include "ggadget/text_formats.h"

#ifdef DrawText
#undef DrawText
#endif

namespace ggadget {
struct Color;
class Rectangle;

class TextRendererInterface {
 public:
  virtual void Destroy() = 0;
  // Sets the text content and the corresponding formats.
  virtual void SetTextAndFormat(const std::string& text,
                                const TextFormats& formats) = 0;
  // Sets the layout attributes for text.
  virtual void SetAlignment(CanvasInterface::Alignment align) = 0;
  virtual void SetVAlignment(CanvasInterface::VAlignment valign) = 0;
  virtual void SetWordWrap(bool word_wrap) = 0;
  virtual void SetLayoutRectangle(double x, double y,
                                  double width, double height) = 0;
  virtual void SetTrimming(CanvasInterface::Trimming trimming) = 0;
  // Draws the text onto canvas.
  virtual bool DrawText(CanvasInterface* canvas) = 0;
  // Draws the text onto canvas using src as texture, the color specified in
  // formats will be omitted.
  virtual bool DrawTextWithTexture(const CanvasInterface* texture,
                                   CanvasInterface* canvas) = 0;
  // Gets the bounding width and height of the text area.
  virtual bool GetTextExtents(double* width, double* height) = 0;
  // Gets the bounding boxes of given text range in the formatted text.
  // Notice if the text range is break into several lines, or the range overlaps
  // more than one format ranges, there may be several bounding boxes.
  // Calling this function with a zero length range will get a rectangle with 1
  // pixel width before the character at range.start.
  // Returns the count of bounding boxes.
  virtual int GetTextRangeBoundingBoxes(
      const Range& range,
      std::vector<Rectangle>* bounding_boxes) = 0;
  // Sets the default format for text rendering. Any unspecified field of text
  // formats will take the value of the default format.
  virtual void SetDefaultFormat(const TextFormat& default_format) = 0;
  // Draws the caret on |canvas| at |caret_pos| with |color|.
  // Notice that |caret_pos| is counted by utf16 codepoints.
  // This method is better called after calling DrawText.
  virtual void DrawCaret(CanvasInterface* canvas,
                         int caret_pos,
                         const Color& color) = 0;
};

}  // namespace ggadget
#endif  // GGADGET_TEXT_RENDERER_INTERFACE_H__
