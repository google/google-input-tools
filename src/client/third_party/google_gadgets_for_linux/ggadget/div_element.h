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

#ifndef GGADGET_DIV_ELEMENT_H__
#define GGADGET_DIV_ELEMENT_H__

#include <stdlib.h>
#include <ggadget/scrolling_element.h>

namespace ggadget {

/**
 * @ingroup Elements
 * Class of <a href=
 * "http://code.google.com/apis/desktop/docs/gadget_apiref.html#div">
 * div element</a>.
 */
class DivElement : public ScrollingElement {
 public:
  DEFINE_CLASS_ID(0xfca426268a584176, ScrollingElement);

  DivElement(View *view, const char *name);
  virtual ~DivElement();

 protected:
  virtual void DoClassRegister();

 public:
  /** Background mode of the div. It's for internal usage. */
  enum BackgroundMode {
    BACKGROUND_MODE_TILE,
    BACKGROUND_MODE_STRETCH,
  };

  /** Gets the background color or image of the element. */
  Variant GetBackground() const;
  /**
   * Sets the background color or image of the element. The image is
   * repeated (tiled) or stretched according to the current background mode.
   */
  void SetBackground(const Variant &background);

  BackgroundMode GetBackgroundMode() const;
  void SetBackgroundMode(BackgroundMode mode);

  /**
   * Set the border length of background, the border part will not be stretched
   * or tiled.
   * @param left, top, right, bottom border width/height.
   */
  void GetBackgroundBorder(double *left, double *top,
                           double *right, double *bottom) const;

  void SetBackgroundBorder(double left, double top,
                           double right, double bottom);

 public:
  static BasicElement *CreateInstance(View *view, const char *name);

 protected:
  /**
   * Used to subclass div elements.
   * No scriptable interfaces will be registered in this constructor.
   */
  DivElement(View *view, const char *tag_name, const char *name);

  virtual void Layout();
  virtual void DoDraw(CanvasInterface *canvas);
  virtual EventResult HandleKeyEvent(const KeyboardEvent &event);

 public:
  virtual bool HasOpaqueBackground() const;

 private:
  DISALLOW_EVIL_CONSTRUCTORS(DivElement);

  class Impl;
  Impl *impl_;
};

} // namespace ggadget

#endif // GGADGET_DIV_ELEMENT_H__
