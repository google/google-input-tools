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

#ifndef GGADGET_TEXT_FRAME_H__
#define GGADGET_TEXT_FRAME_H__

#include <ggadget/canvas_interface.h>
#include <ggadget/text_formats.h>
#include <ggadget/variant.h>
#include <ggadget/small_object.h>

namespace ggadget {

struct Color;
class Texture;
class BasicElement;
class View;

/**
 * @ingroup Utilities
 *
 * A helper class to handle text drawing.
 */
class TextFrame : public SmallObject<> {
 public:
  TextFrame(BasicElement *owner, View *view);
  ~TextFrame();

  /** Attaches the TextFrame object to a new view. */
  void SetView(View *view);

  void RegisterClassProperties(
      TextFrame *(*delegate_getter)(BasicElement *),
      const TextFrame *(*delegate_getter_const)(BasicElement *));

  /**
   * Gets and sets the text of the frame. The text can be plain text or
   * markup Text.
   */
  std::string GetText() const;

  /**
   * Returns @c true if the text changed.
   * Use std::string as the parameter type to allow arbitrary binary data.
   * Some gadgets (such as gmail gadget) relies on this
   */
  bool SetText(const std::string &text);

  /**
   * Set text and their formats.
   * Returns @c true if the text changed.
   */
  bool SetTextWithFormats(const std::string &text,
                          const TextFormats& formats);

  void DrawWithTexture(CanvasInterface *canvas, double x, double y,
                       double width, double height, Texture *texture);
  void Draw(CanvasInterface *canvas, double x, double y,
            double width, double height);

  /**
   * Returns the width and height required to display the text
   * without wrapping or trimming.
   */
  void GetSimpleExtents(double *width, double *height);

  /**
   * Returns the width and height required to display to text within given
   * in_width without trimming.
   */
  void GetExtents(double in_width, double *width, double *height);

  /**
   * Draws the caret on canvas at caret_pos with color.
   * The caret_pos is counted by UTF16 codepoints.
   */
  void DrawCaret(CanvasInterface* canvas, int caret_pos, const Color& color);

  /**
   * Sets the default format of the text frame.
   */
  void SetDefaultFormat(const TextFormat& default_format);

 public: // registered properties
  /** Gets and sets the text horizontal alignment. */
  CanvasInterface::Alignment GetAlign() const;
  void SetAlign(CanvasInterface::Alignment align);

  /** Gets and sets whether the text is bold. */
  bool IsBold() const;
  void SetBold(bool bold);

  /**
   * Gets and sets the text color or image texture of the text. The image is
   * repeated if necessary, not stretched.
   */
  Variant GetColor() const;
  void SetColor(const Variant &color);
  void SetColor(const Color &color, double opacity);

  /**
   * Gets and sets the text font. Setting font to blank or NULL will reset
   * the font to default.
   */
  std::string GetFont() const;
  void SetFont(const char *font);

  /** Gets and sets whether the text is italicized. */
  bool IsItalic() const;
  void SetItalic(bool italic);

  /**
   * Gets and sets the text size in points. Setting size to -1 will reset the
   * font size to default. The gadget host may allow the user to change the
   * default font size.
   */
  double GetSize() const;
  void SetSize(double size);

  /**
   * Same as GetSize(), except that if size is default, GetCurrentSize()
   * returns the current default point size instead of -1.
   */
  double GetCurrentSize() const;

  /** Gets and sets whether the text is struke-out. */
  bool IsStrikeout() const;
  void SetStrikeout(bool strikeout);

  /** Gets and sets the trimming mode when the text is too large to display. */
  CanvasInterface::Trimming GetTrimming() const;
  void SetTrimming(CanvasInterface::Trimming trimming);

  /** Gets and sets whether the text is underlined. */
  bool IsUnderline() const;
  void SetUnderline(bool underline);

  /** Gets and sets the text vertical alignment. */
  CanvasInterface::VAlignment GetVAlign() const;
  void SetVAlign(CanvasInterface::VAlignment valign);

  /** Gets and sets whether to wrap the text when it's too large to display. */
  bool IsWordWrap() const;
  void SetWordWrap(bool wrap);

  /** Gets and sets the basic reading direction. True means right to left. */
  bool IsRTL() const;
  void SetRTL(bool rtl);

 private:
  DISALLOW_EVIL_CONSTRUCTORS(TextFrame);
  class Impl;
  Impl *impl_;
};

} // namespace ggadget

#endif // GGADGET_TEXT_FRAME_H__
