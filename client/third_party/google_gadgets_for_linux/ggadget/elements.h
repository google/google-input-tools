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

#ifndef GGADGET_ELEMENTS_H__
#define GGADGET_ELEMENTS_H__

#include <ggadget/common.h>
#include <ggadget/event.h>
#include <ggadget/scriptable_helper.h>
#include <ggadget/scriptable_interface.h>
#include <ggadget/variant.h>
#include <ggadget/view_interface.h>
#include <ggadget/math_utils.h>
#include <ggadget/clip_region.h>

namespace ggadget {

template <typename R, typename P1> class Slot1;

class BasicElement;
class Connection;
class ElementFactory;
class CanvasInterface;
class View;
class ViewElement;

/**
 * @ingroup Elements
 *
 * Elements is used for storing and managing a set of BasicElement objects.
 * Please refer to: <a href=
 * "http://code.google.com/apis/desktop/docs/gadget_apiref.html#elements">
 * elements object</a> in Desktop Gadgets API.
 */
class Elements : public ScriptableHelperNativeOwnedDefault {
 public:
  DEFINE_CLASS_ID(0xe3bdb064cb794282, ScriptableInterface);

  /**
   * Create an Elements object and assign the given factory to it.
   * @param factory the factory used to create the child elements.
   * @param owner the parent element. Can be @c null for top level elements
   *     owned directly by a view.
   * @param view the containing view.
   */
  Elements(ElementFactory *factory, BasicElement *owner, View *view);

  virtual ~Elements();

 protected:
  virtual void DoClassRegister();

 public:
  /**
   * @return number of children.
   */
  size_t GetCount() const;

  //@{
  /**
   * Returns the element identified by the index.
   * @param child the index of the child.
   * @return the pointer to the specified element. If the parameter is out of
   *     range, @c NULL is returned.
   */
  BasicElement *GetItemByIndex(size_t child);
  const BasicElement *GetItemByIndex(size_t child) const;
  //@}

  //@{
  /**
   * Returns the element identified by the name.
   * @param child the name of the child.
   * @return the pointer to the specified element. If multiple elements are
   *     defined with the same name, returns the first one. Returns @c NULL if
   *     no elements match.
   */
  BasicElement *GetItemByName(const char *child);
  const BasicElement *GetItemByName(const char *child) const;
  //@}

  /**
   * Create a new element and add it to the end of the children list.
   * @param tag_name a string specified the element tag name.
   * @param name the name of the newly created element.
   * @return the pointer to the newly created element, or @c NULL when error
   *     occured.
   */
  BasicElement *AppendElement(const char *tag_name,
                              const char *name);

  /**
   * Create a new element before the specified element.
   * @param tag_name a string specified the element tag name.
   * @param before the newly created element will be inserted before the given
   *     element. If the specified element is not the direct child of the
   *     container or this parameter is @c NULL, this method will insert the
   *     newly created element at the end of the children list.
   * @param name the name of the newly created element.
   * @return the pointer to the newly created element, or @c NULL when error
   *     occured.
   */
  BasicElement *InsertElement(const char *tag_name,
                              const BasicElement *before,
                              const char *name);

  /**
   * Create a new element after the specified element.
   * @see InsertElement()
   */
  BasicElement *InsertElementAfter(const char *tag_name,
                                   const BasicElement *after,
                                   const char *name);

  /**
   * Appends an existing element to the end of the children list.
   * The specified element can be owned by another parent element, in this
   * case, it'll be reparented appropriately.
   *
   * @param element The element to insert.
   * @return true on success, false on failure.
   */
  bool AppendElement(BasicElement *element);

  /**
   * Inserts an existing element before the specified element.
   * The element to be inserted can be owned by another parent element, in this
   * case, it'll be reparented appropriately.
   *
   * @param element The element to insert.
   * @param before the element will be inserted before the given element.
   *     If the specified element is not the direct child of the
   *     container or this parameter is @c NULL, this method will insert the
   *     newly created element at the end of the children list.
   *     If the specified element is already a child of this element,
   *     only the order is changed so that it would be put before @c before
   * @return true on success, false on failure.
   */
  bool InsertElement(BasicElement *element, const BasicElement *before);

  /**
   * Inserts an existing element after the specified element.
   * @see InsertElement()
   */
  bool InsertElementAfter(BasicElement *element, const BasicElement *after);

  /**
   * Create a new element from XML definition and add it to the end of the
   * children list.
   * @param xml the XML definition of the element.
   * @return the pointer to the newly created element, or @c NULL when error
   *     occured.
   */
  BasicElement *AppendElementFromXML(const std::string &xml);

  /**
   * Create a new element from XML definition and insert it before the
   * specified element.
   * @param xml the XML definition of the element.
   * @param before the newly created element will be inserted before the given
   *     element. If the specified element is not the direct child of the
   *     container or this parameter is @c NULL, this method will insert the
   *     newly created element at the end of the children list.
   * @return the pointer to the newly created element, or @c NULL when error
   *     occured.
   */
  BasicElement *InsertElementFromXML(const std::string &xml,
                                     const BasicElement *before);

  /**
   * Create a new element from XML definition and insert it after the
   * specified element.
   * @see InsertElementFromXML()
   */
  BasicElement *InsertElementFromXMLAfter(const std::string &xml,
                                          const BasicElement *after);

  /**
   * Appends an element holded in a variant to the end of the children list.
   *
   * The specified variant may hold a pointer to an existing element, or the
   * XML definition of a new element.
   *
   * @param element A variant holding an existing element or the XML definition
   *                of a new element.
   * @return the pointer to the new element on success, or @c NULL on failure.
   */
  BasicElement *AppendElementVariant(const Variant &element);

  /**
   * Inserts an element held in a variant before the specified element.
   * The element to be inserted can be owned by another parent element, in this
   * case, it'll be reparented appropriately.
   *
   * The specified variant may hold a pointer to an existing element, or the
   * XML definition of a new element.
   *
   * @param element A variant holding an existing element or the XML definition
   *                of a new element.
   * @param before The element specified in element parameter will be inserted
   *     before this element. If the specified element is not the direct child
   *     of the container or this parameter is @c NULL, this method will insert
   *     the element at the end of the children list.
   * @return the pointer to the new element on success, or @c NULL on failure.
   */
  BasicElement *InsertElementVariant(const Variant &element,
                                     const BasicElement *before);

  /**
   * Inserts an element held in a variant after the specified element.
   * @see InsertElementVariant()
   */
  BasicElement *InsertElementVariantAfter(const Variant &element,
                                          const BasicElement *after);

  /**
   * Remove the specified element from the container.
   * If successful, the BasicElement will be destroyed and the pointer element
   * will become invalid.
   * @param element the element to remove.
   * @return @c true if removed successfully, or @c false if the specified
   *     element doesn't exists or not the direct child of the container.
   */
  bool RemoveElement(BasicElement *element);

  /**
   * Remove all elements from the container.
   */
  void RemoveAllElements();

  /**
   * Calculate the size of children.
   * This method is called just before @c Layout();
   */
   void CalculateSize();

  /**
   * Adjusts the layout (e.g. position, etc.) of children.
   * This method is called just before @c Draw().
   */
  void Layout();

  /**
   * Draw all the elements in this object onto a specified canvas.
   * The canvas shall already be prepared to be drawn directly without any
   * transformation, except the opacity and clip region.
   * @param canvas A canvas on which all elements shall be drawn.
   */
  void Draw(CanvasInterface *canvas);

  /**
   * Handler of the mouse events.
   * @param event the mouse event.
   * @param[out] fired_element the element who processed the event, or
   *     @c NULL if no one.
   * @param[out] in_element the child element where the mouse is in (including
   *     disabled child elements, but not invisible child elements).
   * @param[out] hittest result of this mouse event. It's the hittest value of
   *     in_element, if there is no in_element, the return value is undefined.
   * @return result of event handling.
   */
  EventResult OnMouseEvent(const MouseEvent &event,
                           BasicElement **fired_element,
                           BasicElement **in_element,
                           ViewInterface::HitTest *hittest);

  /**
   * Handler of the drag and drop events.
   * @param event the darg and drop event.
   * @param[out] fired_element the element who processed the event, or
   *     @c NULL if no one.
   * @return result of event handling.
   */
  EventResult OnDragEvent(const DragEvent &event,
                          BasicElement **fired_element);

  /**
   * Sets if the drawing contents can be scrolled within the parent.
   */
  void SetScrollable(bool scrollable);

  /**
   * Gets the maximum extents of children.
   * If not scrollable, the returned size equals to the size of the parent
   * element (or view if no parent). If scrollable, the returned value is the
   * actual max extents of children.
   */
  void GetChildrenExtents(double *width, double *height);

  /**
   * Sets a redraw mark, so that all things and children will be redrawed
   * during the next call of Draw().
   */
  void MarkRedraw();

  /**
   * Aggregates clip region of all children. It'll be called just
   * after calling Layout() and before calling Draw().
   * The clip region cache of children elements will be cleared.
   *
   * If either region is NULL or boundary is empty, then only clip region
   * cache will be cleared.
   *
   * @param boundary Clip region boundary in view's coordinates. All clip
   *        rectangles shall not beyond this boundary.
   * @param region Contains all clip rectangles in view's coordinates.
   */
  void AggregateClipRegion(const Rectangle &boundary, ClipRegion *region);

  /**
   * Connects a slot to the signal that will be fired when an element is added.
   */
  Connection *ConnectOnElementAdded(Slot1<void, BasicElement*> *slot);

  /**
   * Connects a slot to the signal that will be fired when an element is
   * removed. The element will be deleted immediately after firing the signal,
   * so the slot handler should not keep the element at all.
   */
  Connection *ConnectOnElementRemoved(Slot1<void, BasicElement*> *slot);

 private:
  class Impl;
  Impl *impl_;
  DISALLOW_EVIL_CONSTRUCTORS(Elements);
};

} // namespace ggadget

#endif // GGADGET_ELEMENTS_H__
