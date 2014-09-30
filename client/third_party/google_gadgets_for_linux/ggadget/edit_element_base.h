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

#ifndef GGADGET_EDIT_ELEMENT_BASE_H__
#define GGADGET_EDIT_ELEMENT_BASE_H__

#include <string>
#include <ggadget/canvas_interface.h>
#include <ggadget/basic_element.h>
#include <ggadget/scrolling_element.h>

namespace ggadget {

/**
 * @ingroup Elements
 * Base class of <a href=
 * "http://code.google.com/apis/desktop/docs/gadget_apiref.html#edit">
 * edit element</a>.
 *
 * Real EditElement implementation shall inherit
 * this class and implement its pure virtual functions.
 *
 * All scriptable properties will be registered in EditElementBase class,
 * derived class does not need to care about them.
 */
class EditElementBase : public ScrollingElement {
 public:
  DEFINE_CLASS_ID(0x6C5D2E793806428F, ScrollingElement);

  EditElementBase(View *view, const char *name);
  virtual ~EditElementBase();

 protected:
  virtual void DoClassRegister();

 public:
  virtual bool IsTabStopDefault() const;
  virtual void Layout();

  /**
   * Connects a specified slot to onchange event signal.
   * The signal will be fired when FireOnChangeEvent() method is called by
   * derived class.
   */
  Connection *ConnectOnChangeEvent(Slot0<void> *slot);

 public:
  /** Gets the background color or texture of the text */
  virtual Variant GetBackground() const = 0;
  /** Sets the background color or texture of the text */
  virtual void SetBackground(const Variant &background) = 0;

  /** Gets whether the text is bold. */
  virtual bool IsBold() const = 0;
  /** Sets whether the text is bold. */
  virtual void SetBold(bool bold) = 0;

  /** Gets the text color of the text. */
  virtual std::string GetColor() const = 0;
  /** Sets the text color of the text. */
  virtual void SetColor(const char *color) = 0;

  /** Gets the text font. */
  virtual std::string GetFont() const = 0;
  /** Sets the text font. */
  virtual void SetFont(const char *font) = 0;

  /** Gets whether the text is italicized. */
  virtual bool IsItalic() const = 0;
  /** Sets whether the text is italicized. */
  virtual void SetItalic(bool italic) = 0;

  /** Gets whether the edit can display multiple lines of text. */
  virtual bool IsMultiline() const = 0;
  /**
   * Sets whether the edit can display multiple lines of text.
   * If @c false, incoming "\n"s are ignored, as are presses of the Enter key.
   */
  virtual void SetMultiline(bool multiline) = 0;

  /**
   * Gets the character that should be displayed each time the user
   * enters a character.
   */
  virtual std::string GetPasswordChar() const = 0;
  /**
   * Sets the character that should be displayed each time the user
   * enters a character. By default, the value is @c "\0", which means that the
   * typed character is displayed as-is.
   */
  virtual void SetPasswordChar(const char *passwordChar) = 0;

  /** Gets the text size in points. */
  double GetSize() const;
  /**
   * Sets the text size in points. Setting size to -1 will reset the
   * font size to default. The gadget host may allow the user to change the
   * default font size.
   * Subclasses should implement @c DoGetSize() and @c GetSize().
   */
  void SetSize(double size);

  /**
   * Same as GetSize(), except that if size is default, GetCurrentSize()
   * returns the current default point size instead of -1.
   */
  double GetCurrentSize() const;

  /** Gets whether the text is struke-out. */
  virtual bool IsStrikeout() const = 0;
  /** Sets whether the text is struke-out. */
  virtual void SetStrikeout(bool strikeout) = 0;

  /** Gets whether the text is underlined. */
  virtual bool IsUnderline() const = 0;
  /** Sets whether the text is underlined. */
  virtual void SetUnderline(bool underline) = 0;

  /** Gets the value of the element. */
  virtual std::string GetValue() const = 0;
  /** Sets the value of the element. */
  virtual void SetValue(const char *value) = 0;

  /** Gets whether to wrap the text when it's too large to display. */
  virtual bool IsWordWrap() const = 0;
  /** Sets whether to wrap the text when it's too large to display. */
  virtual void SetWordWrap(bool wrap) = 0;

  /** Gets whether the edit element is readonly */
  virtual bool IsReadOnly() const = 0;
  /** Sets whether the edit element is readonly */
  virtual void SetReadOnly(bool readonly) = 0;

  /** Gets detectUrls property, introduced in 5.8 */
  virtual bool IsDetectUrls() const = 0;
  /** Sets detectUrls property, introduced in 5.8 */
  virtual void SetDetectUrls(bool detect_urls) = 0;

  /** Gets the ideal bounding rect for the edit element which is large enough
   * for displaying the content without scrolling. */
  virtual void GetIdealBoundingRect(int *width, int *height) = 0;

  /**
   * Selects the specified characters in the edit box. The first character has
   * index 0. Use -1 to indicate the last character.
   *
   * Added in 5.5 out-of-beta
   */
  virtual void Select(int start, int end) = 0;

  /**
   * Selects all characters in the edit box.
   *
   * Added in 5.5 out-of-beta
   */
  virtual void SelectAll() = 0;

  /** Gets the text horizontal alignment setting. */
  virtual CanvasInterface::Alignment GetAlign() const = 0;

  /** Sets the text horizontal alignment setting. */
  virtual void SetAlign(CanvasInterface::Alignment align) = 0;

  /** Gets the text vertical alignment setting. */
  virtual CanvasInterface::VAlignment GetVAlign() const = 0;

  /** Sets the text vertical alignment setting. */
  virtual void SetVAlign(CanvasInterface::VAlignment valign) = 0;

 public:
  /**
   * Derived class shall call this method if the value is changed.
   */
  void FireOnChangeEvent() const;

 protected:
  /** Informs the derived class that the font size has changed. */
  virtual void OnFontSizeChange() = 0;

 private:
  DISALLOW_EVIL_CONSTRUCTORS(EditElementBase);

  class Impl;
  Impl *impl_;
};

} // namespace ggadget

#endif // GGADGET_EDIT_ELEMENT_BASE_H__
