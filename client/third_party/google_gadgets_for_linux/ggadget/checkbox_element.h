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

#ifndef GGADGET_CHECKBOX_ELEMENT_H__
#define GGADGET_CHECKBOX_ELEMENT_H__

#include <stdlib.h>
#include "basic_element.h"

namespace ggadget {

class MouseEvent;
class TextFrame;

/**
 * @ingroup Elements
 * Class of <a href=
 * "http://code.google.com/apis/desktop/docs/gadget_apiref.html#checkbox">
 * checkbox element</a>.
 */
class CheckBoxElement : public BasicElement {
 public:
  DEFINE_CLASS_ID(0xe53dbec04fe34ea3, BasicElement);

  CheckBoxElement(View *view, const char *name, bool is_checkbox);
  virtual ~CheckBoxElement();

 protected:
  virtual void DoClassRegister();

 public:
  /** Gets the file name of default checkbox image. */
  Variant GetImage() const;
  /** Sets the file name of default checkbox image. */
  void SetImage(const Variant &img);

  /** Gets the file name of disabled checkbox image. */
  Variant GetDisabledImage() const;
  /** Sets the file name of disabled checkbox image. */
  void SetDisabledImage(const Variant &img);

  /** Gets the file name of mouse over checkbox image. */
  Variant GetOverImage() const;
  /** Sets the file name of mouse over checkbox image. */
  void SetOverImage(const Variant &img);

  /** Gets the file name of mouse down checkbox image. */
  Variant GetDownImage() const;
  /** Sets the file name of mouse down checkbox image. */
  void SetDownImage(const Variant &img);

  /** Gets the file name of default checkbox image. */
  Variant GetCheckedImage() const;
  /** Sets the file name of default checkbox image. */
  void SetCheckedImage(const Variant &img);

  /** Gets the file name of disabled checked checkbox image. */
  Variant GetCheckedDisabledImage() const;
  /** Sets the file name of disabled checked checkbox image. */
  void SetCheckedDisabledImage(const Variant &img);

  /** Gets the file name of mouse over checked checkbox image. */
  Variant GetCheckedOverImage() const;
  /** Sets the file name of mouse over checked checkbox image. */
  void SetCheckedOverImage(const Variant &img);

  /** Gets the file name of mouse down checked checkbox image. */
  Variant GetCheckedDownImage() const;
  /** Sets the file name of mouse down checked checkbox image. */
  void SetCheckedDownImage(const Variant &img);

  /** Gets whether the checkbox is checked. A checked state is true. */
  bool GetValue() const;
  /** Sets whether the checkbox is checked. A checked state is true. */
  void SetValue(bool value);

  /** Gets whether the checkbox is on the right side. Undocumented. */
  bool IsCheckBoxOnRight() const;
  /** Sets whether the checkbox is on the right side. Undocumented. */
  void SetCheckBoxOnRight(bool right);

  //@{
  /** Gets the text frame containing the caption of this checkbox. */
  TextFrame *GetTextFrame();
  const TextFrame *GetTextFrame() const;
  //@}

  /** Gets if the button should be rendered with default images. */
  bool IsDefaultRendering() const;
  /** Sets if the button should be rendered with default images. */
  void SetDefaultRendering(bool default_rendering);

  /**
   * Gets if this element is a checkbox.
   * @return true if it's a checkbox element, false if it's a radio button.
   */
  bool IsCheckBox() const;

  /**
   * Connects a slot to onchange event signal, which will be emitted when the
   * value is changed.
   */
  Connection *ConnectOnChangeEvent(Slot0<void> *handler);

 public:
  static BasicElement *CreateCheckBoxInstance(View *view, const char *name);
  static BasicElement *CreateRadioInstance(View *view, const char *name);

 protected:
  virtual void DoDraw(CanvasInterface *canvas);
  virtual EventResult HandleMouseEvent(const MouseEvent &event);
  virtual void GetDefaultSize(double *width, double *height) const;

 private:
  DISALLOW_EVIL_CONSTRUCTORS(CheckBoxElement);

  class Impl;
  Impl *impl_;
};

} // namespace ggadget

#endif // GGADGET_CHECKBOX_ELEMENT_H__
