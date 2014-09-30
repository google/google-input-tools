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

#ifndef GGADGET_SCROLLING_ELEMENT_H__
#define GGADGET_SCROLLING_ELEMENT_H__

#include <stdlib.h>
#include <ggadget/basic_element.h>

namespace ggadget {

class ScrollBarElement;

/**
 * @ingroup Elements
 * Base class of all scrollable element, such as div.
 */
class ScrollingElement : public BasicElement {
 public:
  DEFINE_CLASS_ID(0x17107e53044c40f2, BasicElement);

 protected:
  ScrollingElement(View *view, const char *tag_name, const char *name,
                   bool children);
  virtual ~ScrollingElement();
  virtual void DoRegister();
  virtual void DoClassRegister();

  virtual void AggregateMoreClipRegion(const Rectangle &boundary,
                                       ClipRegion *region);

 public:
  virtual void MarkRedraw();
  virtual bool IsChildInVisibleArea(const BasicElement *child) const;
  virtual void EnsureAreaVisible(const Rectangle &rect,
                                 const BasicElement *source);

 public:
  /** Gets the autoscroll property. */
  bool IsAutoscroll() const;
  /**
   * Sets the autoscroll property.
   * @c true if the div automatically shows scrollbars if necessary; @c false
   * if it doesn't show scrollbars. Default is @c false.
   * For now only vertical scrollbar is supported.
   */
  void SetAutoscroll(bool autoscroll);

  bool IsXScrollable();
  void SetXScrollable(bool x_scrollable);
  bool IsYScrollable();
  void SetYScrollable(bool y_scrollable);

  /** Scroll horizontally. */
  void ScrollX(int distance);
  /** Scroll vertically. */
  void ScrollY(int distance);

  /** Gets X absolute position. */
  int GetScrollXPosition() const;
  /** Sets X absolute position. */
  void SetScrollXPosition(int pos);
  /** Gets Y absolute position. */
  int GetScrollYPosition() const;
  /** Sets Y absolute position. */
  void SetScrollYPosition(int pos);

  /** Gets X page step value. */
  int GetXPageStep() const;
  /** Sets X page step value. */
  void SetXPageStep(int value);

  /** Gets Y page step value. */
  int GetYPageStep() const;
  /** Sets Y page step value. */
  void SetYPageStep(int value);

  /** Gets X line step value. */
  int GetXLineStep() const;
  /** Sets X line step value. */
  void SetXLineStep(int value);

  /** Gets Y line step value. */
  int GetYLineStep() const;
  /** Sets Y line step value. */
  void SetYLineStep(int value);

  /** Gets pixel width of client area */
  virtual double GetClientWidth() const;
  /** Gets pixel height of client area */
  virtual double GetClientHeight() const;

  virtual EventResult OnMouseEvent(const MouseEvent &event, bool direct,
                                   BasicElement **fired_element,
                                   BasicElement **in_element,
                                   ViewInterface::HitTest *hittest);

  /**
   * Overrides because this element supports scrolling.
   * @see BasicElement::SelfCoordToChildCoord()
   *
   * Derived classes shall override this method if they have private children
   * to be handled specially.
   */
  virtual void SelfCoordToChildCoord(const BasicElement *child,
                                     double x, double y,
                                     double *child_x, double *child_y) const;

  /**
   * Overrides because this element supports scrolling.
   * @see BasicElement::ChildCoordToSelfCoord()
   *
   * Derived classes shall override this method if they have private children
   * to be handled specially.
   */
  virtual void ChildCoordToSelfCoord(const BasicElement *child,
                                     double x, double y,
                                     double *self_x, double *self_y) const;


  /**
   * Register a slot to listen to on-scrolled event.
   * When the scrollbar is scrolled by user, this slot will be called.
   */
  Connection *ConnectOnScrolledEvent(Slot0<void> *slot);

  //@{
  /**
   * Returns the vertical scrollbar element.
   * It will be @c NULL if autoScroll is @c false.
   */
  ScrollBarElement *GetScrollBar();
  const ScrollBarElement *GetScrollBar() const;
  //@}

 protected:
  /**
   * Draws scrollbar on the canvas. Subclasses must call this in their
   * @c DoDraw() method.
   */
  void DrawScrollbar(CanvasInterface *canvas);

  /**
   * Updates the scrollbar's range and layout. Subclasses must call this in
   * their @c Layout() method.
   * If y_range equals to zero, then means the scroll bar should be hid.
   * @return @c true if the visibility of the scroll bar changed, and the
   *     caller must update layout again, for example, recursively call
   *     @c Layout() again. Otherwise, returns @c false.
   */
  bool UpdateScrollBar(int x_range, int y_range);

 private:
  DISALLOW_EVIL_CONSTRUCTORS(ScrollingElement);

  class Impl;
  Impl *impl_;
};

} // namespace ggadget

#endif // GGADGET_SCROLLING_ELEMENT_H__
