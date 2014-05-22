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

#ifndef GGADGET_SCROLLBAR_ELEMENT_H__
#define GGADGET_SCROLLBAR_ELEMENT_H__

#include <cstdlib>
#include <ggadget/slot.h>
#include <ggadget/basic_element.h>

namespace ggadget {

class MouseEvent;

/**
 * @ingroup Elements
 * Class of <a href=
 * "http://code.google.com/apis/desktop/docs/gadget_apiref.html#scrollbar">
 * scrollbar element</a>.
 */
class ScrollBarElement : public BasicElement {
 public:
  DEFINE_CLASS_ID(0x789974a8a14a43c9, BasicElement);

  enum Orientation {
    ORIENTATION_VERTICAL,
    ORIENTATION_HORIZONTAL
  };

  ScrollBarElement(View *view, const char *name);
  virtual ~ScrollBarElement();

 public:
  /** Gets the file name of the background image. */
  Variant GetBackground() const;
  /** Sets the file name of the background image. */
  void SetBackground(const Variant &img);

  /** Gets the file name of the thumb grippy image. */
  Variant GetGrippyImage() const;
  /**
   * Sets the file name of the thumb grippy image.
   * If grippy image is set, the thumb will be displayed proportionally.
   */
  void SetGrippyImage(const Variant &img);

  /** Gets the file name of the left/up button down image. */
  Variant GetLeftDownImage() const;
  /** Sets the file name of the left/up button down image. */
  void SetLeftDownImage(const Variant &img);

  /** Gets the file name of the left/up image. */
  Variant GetLeftImage() const;
  /** Sets the file name of the left/up image. */
  void SetLeftImage(const Variant &img);

  /** Gets the file name of the left/up hover image. */
  Variant GetLeftOverImage() const;
  /** Sets the file name of the left/up hover image. */
  void SetLeftOverImage(const Variant &img);

  /** Gets the file name of the right/down button down image. */
  Variant GetRightDownImage() const;
  /** Sets the file name of the right/down button down image. */
  void SetRightDownImage(const Variant &img);

  /** Gets the file name of the right/down image. */
  Variant GetRightImage() const;
  /** Sets the file name of the right/down image. */
  void SetRightImage(const Variant &img);

  /** Gets the file name of the right/down hover image. */
  Variant GetRightOverImage() const;
  /** Sets the file name of the right/down hover image. */
  void SetRightOverImage(const Variant &img);

  /** Gets the file name of the thumb button down image. */
  Variant GetThumbDownImage() const;
  /** Sets the file name of the thumb button down image. */
  void SetThumbDownImage(const Variant &img);

  /** Gets the file name of the thumb image. */
  Variant GetThumbImage() const;
  /** Sets the file name of the thumb image. */
  void SetThumbImage(const Variant &img);

  /** Gets the file name of the thumb hover image. */
  Variant GetThumbOverImage() const;
  /** Sets the file name of the thumb hover image. */
  void SetThumbOverImage(const Variant &img);

   /** Gets the scrollbar orientation (horizontal, vertical). */
  Orientation GetOrientation() const;
   /** Sets the scrollbar orientation (horizontal, vertical). */
  void SetOrientation(Orientation o);

  /** Gets the max scrollbar value. */
  int GetMax() const;
  /** Sets the max scrollbar value. */
  void SetMax(int value);

  /** Gets the min scrollbar value. */
  int GetMin() const;
  /** Sets the min scrollbar value. */
  void SetMin(int value);

  /** Gets the page step value. */
  int GetPageStep() const;
  /** Sets the page step value. */
  void SetPageStep(int value);

  /** Gets the line step value. */
  int GetLineStep() const;
  /** Sets the line step value. */
  void SetLineStep(int value);

  /** Gets the scroll position of the thumb. */
  int GetValue() const;
  /** Sets the scroll position of the thumb. */
  void SetValue(int value);

  /** Gets if the button should be rendered with default images. */
  bool IsDefaultRendering() const;
  /** Sets if the button should be rendered with default images. */
  void SetDefaultRendering(bool default_rendering);

  Connection *ConnectOnChangeEvent(Slot0<void> *slot);

 protected:
  virtual void DoClassRegister();
  virtual void DoDraw(CanvasInterface *canvas);
  virtual EventResult HandleMouseEvent(const MouseEvent &event);
  friend class ScrollingElement;

 public:
  virtual void Layout();
  virtual bool HasOpaqueBackground() const;

 public:
  static BasicElement *CreateInstance(View *view, const char *name);

 private:
  DISALLOW_EVIL_CONSTRUCTORS(ScrollBarElement);

  class Impl;
  Impl *impl_;
};

} // namespace ggadget

#endif // GGADGET_SCROLLBAR_ELEMENT_H__
