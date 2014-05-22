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

#ifndef GGADGET_BUTTON_ELEMENT_H__
#define GGADGET_BUTTON_ELEMENT_H__

#include <stdlib.h>
#include <ggadget/basic_element.h>

namespace ggadget {

class MouseEvent;
class TextFrame;

/**
 * @ingroup Elements
 * Class of <a href=
 * "http://code.google.com/apis/desktop/docs/gadget_apiref.html#button">
 * button element</a>.
 */
class ButtonElement : public BasicElement {
 public:
  DEFINE_CLASS_ID(0xb6fb01fd48134377, BasicElement);

  /** Enums to specify the icon's position in the button. */
  enum IconPosition {
    ICON_LEFT = 0,
    ICON_RIGHT,
    ICON_TOP,
    ICON_BOTTOM
  };

  ButtonElement(View *view, const char *name);
  virtual ~ButtonElement();

 protected:
  virtual void DoClassRegister();

 public:
  /** Gets the file name of default button image. */
  Variant GetImage() const;
  /** Sets the file name of default button image. */
  void SetImage(const Variant &img);

  /** Gets the file name of disabled button image. */
  Variant GetDisabledImage() const;
  /** Sets the file name of disabled button image. */
  void SetDisabledImage(const Variant &img);

  /** Gets the file name of mouse over button image. */
  Variant GetOverImage() const;
  /** Sets the file name of mouse over button image. */
  void SetOverImage(const Variant &img);

  /** Gets the file name of mouse down button image. */
  Variant GetDownImage() const;
  /** Sets the file name of mouse down button image. */
  void SetDownImage(const Variant &img);

  /** Gets the file name of icon image. */
  Variant GetIconImage() const;
  /** Sets the file name of icon image. */
  void SetIconImage(const Variant &img);

  /** Gets the file name of icon image for disabled button. */
  Variant GetIconDisabledImage() const;
  /** Sets the file name of icon image for disabled button. */
  void SetIconDisabledImage(const Variant &img);

  //@{
  /** Gets the text frame containing the caption of this button. */
  TextFrame *GetTextFrame();
  const TextFrame *GetTextFrame() const;
  //@}

  /**
   * Gets whether the image is stretched normally or stretched only the
   * middle area.
   */
  bool IsStretchMiddle() const;
  /**
   * Sets whether the image is stretched normally or stretched only the
   * middle area.
   */
  void SetStretchMiddle(bool stretch_middle);

  /** Gets the icon position. */
  IconPosition GetIconPosition() const;
  /** Sets the icon position. */
  void SetIconPosition(IconPosition position);

  /** Gets if the button should be rendered with default images. */
  bool IsDefaultRendering() const;
  /** Sets if the button should be rendered with default images. */
  void SetDefaultRendering(bool default_rendering);

 public:
  static BasicElement *CreateInstance(View *view, const char *name);

 protected:
  virtual void DoDraw(CanvasInterface *canvas);
  virtual EventResult HandleMouseEvent(const MouseEvent &event);
  virtual void GetDefaultSize(double *width, double *height) const;

 public:
  virtual bool HasOpaqueBackground() const;

 private:
  DISALLOW_EVIL_CONSTRUCTORS(ButtonElement);

  class Impl;
  Impl *impl_;
};

} // namespace ggadget

#endif // GGADGET_BUTTON_ELEMENT_H__
