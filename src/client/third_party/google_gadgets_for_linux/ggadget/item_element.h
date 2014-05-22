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

#ifndef GGADGET_ITEM_ELEMENT_H__
#define GGADGET_ITEM_ELEMENT_H__

#include <ggadget/basic_element.h>

namespace ggadget {

class Texture;

/**
 * @ingroup Elements
 * Class of <a href=
 * "http://code.google.com/apis/desktop/docs/gadget_apiref.html#item">
 * item element</a>.
 */
class ItemElement : public BasicElement {
 public:
  DEFINE_CLASS_ID(0x93a09b61fb8a4fda, BasicElement);

  ItemElement(View *view, const char *tag_name, const char *name);
  virtual ~ItemElement();

 protected:
  virtual void DoClassRegister();

 public:
  /** Gets whether this item is currently selected. */
  bool IsSelected() const;
  /** Sets whether this item is currently selected. */
  void SetSelected(bool selected);

  /** Gets the background color or image of the element. */
  Variant GetBackground() const;
  /**
   * Sets the background color or image of the element.
   * The image is repeated if necessary, not stretched.
   */
  void SetBackground(const Variant &background);

  /** Gets whether the mouse pointer is over this item. */
  bool IsMouseOver() const;

  /**
   * Sets whether mouseover/selected overlays should be drawn.
   * This method is used in Draw() calls to temporarily disable overlay drawing.
   */
  void SetDrawOverlay(bool draw);

  /** Sets the current index of the item in the parent. */
  void SetIndex(int index);

  /** Gets the text of the label contained inside this element. */
  std::string GetLabelText() const;
  /**
   * Sets the text of the label contained inside this element.
   * AddLabelWithText will create the Label, the other methods will
   * assume it exists.
   */
  void SetLabelText(const char *text);

  /** Adds a new label with specifid text. */
  bool AddLabelWithText(const char *text);

 public:
  static BasicElement *CreateInstance(View *view, const char *name);

  /** For backward compatibility of listitem. */
  static BasicElement *CreateListItemInstance(View *view, const char *name);

 protected:
  virtual void DoDraw(CanvasInterface *canvas);
  virtual void GetDefaultSize(double *width, double *height) const;
  virtual void GetDefaultPosition(double *x, double *y) const;
  virtual EventResult HandleMouseEvent(const MouseEvent &event);
  virtual EventResult HandleKeyEvent(const KeyboardEvent &event);

 public:
  virtual bool HasOpaqueBackground() const;

 private:
  DISALLOW_EVIL_CONSTRUCTORS(ItemElement);

  class Impl;
  Impl *impl_;
};

} // namespace ggadget

#endif // GGADGET_ITEM_ELEMENT_H__
