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

#ifndef GGADGET_LINEAR_ELEMENT_H__
#define GGADGET_LINEAR_ELEMENT_H__

#include <ggadget/div_element.h>

namespace ggadget {

/**
 * @ingroup Elements
 * An element that layouts all its children in a specified direction one after
 * one without overlapping.
 *
 * Additional properties supported by this element:
 * - orientation
 *   "horizontal" or "vertical", indicating children's layout orientation.
 * - padding
 *   Number of pixels between two children.
 * - hAutoSizing
 *   Indicates if the element's width will be adjusted automatically according
 *   to its children's size.
 * - vAutoSizing
 *   Indicates if the element's height will be adjusted automatically according
 *   to its children's size.
 * - width/height
 *   Support value "auto", which will set corresponding xAutoSizing property to
 *   true.
 *
 * An extra "linearLayoutDir" property will be registered to each child, whose
 * value can be "forward" or "backward", to control the child's layout direction
 * in the parent linear element.
 */
class LinearElement : public DivElement {
 public:
  DEFINE_CLASS_ID(0xe75c3d7707eb4412, DivElement);

  enum Orientation {
    ORIENTATION_HORIZONTAL = 0,
    ORIENTATION_VERTICAL,
  };

  /** Layout direction of its children */
  enum LayoutDirection {
    /**
     * Layouts the child from left to right (horizontal orientation), or from
     * top to bottom (vertical orientation).
     */
    LAYOUT_FORWARD = 0,
    /**
     * Layouts the child from right to left (horizontal orientation), or from
     * bottom to top (vertical orientation).
     */
    LAYOUT_BACKWARD,
  };

  LinearElement(View *view, const char *name);
  virtual ~LinearElement();

  /** Gets the layout orientation (horizontal, vertical). */
  Orientation GetOrientation() const;
  /** Sets the layout orientation (horizontal, vertical). */
  void SetOrientation(Orientation orientation);

  /** Gets the padding pixel between two children. */
  double GetPadding() const;
  /** Sets the padding pixel between two children. */
  void SetPadding(double padding);

  /**
   * Checks/sets if this element's width will be adjusted automatically
   * according to its children's size.
   */
  bool IsHorizontalAutoSizing() const;
  void SetHorizontalAutoSizing(bool auto_sizing);

  /**
   * Checks/sets if this element's height will be adjusted automatically
   * according to its children's size.
   */
  bool IsVerticalAutoSizing() const;
  void SetVerticalAutoSizing(bool auto_sizing);

  /** Gets and sets a child's layout direction. */
  LayoutDirection GetChildLayoutDirection(BasicElement *child) const;
  void SetChildLayoutDirection(BasicElement *child, LayoutDirection dir);

  /** Gets or sets if a child will auto stretch. */
  bool IsChildAutoStretch(BasicElement *child) const;
  void SetChildAutoStretch(BasicElement *child, bool auto_stretch);

  // Overridden from BasicElement:
  virtual double GetMinWidth() const;
  virtual double GetMinHeight() const;

  static BasicElement *CreateInstance(View *view, const char *name);

 protected:
  /**
   * Used to subclass linear elements.
   * No scriptable interfaces will be registered in this constructor.
   */
  LinearElement(View *view, const char *tag_name, const char *name);

  virtual void DoClassRegister();
  virtual void CalculateSize();
  virtual void Layout();
  virtual void BeforeChildrenLayout();

 private:
  DISALLOW_EVIL_CONSTRUCTORS(LinearElement);

  class Impl;
  Impl *impl_;
};

} // namespace ggadget

#endif // GGADGET_LINEAR_ELEMENT_H__
