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

#ifndef GGADGET_OBJECT_ELEMENT_H__
#define GGADGET_OBJECT_ELEMENT_H__

#include <stdlib.h>
#include <ggadget/basic_element.h>

namespace ggadget {

/**
 * @ingroup Elements
 * Class of <a href=
 * "http://code.google.com/apis/desktop/docs/gadget_apiref.html#object">
 * object element</a>.
 */
class ObjectElement : public BasicElement {
 public:
  DEFINE_CLASS_ID(0x5b128d3ef8da40e8, BasicElement);

  ObjectElement(View *view, const char *name);
  virtual ~ObjectElement();

  static BasicElement *CreateInstance(View *view, const char *name);

 public:
  /**
   * This class can not expose the real object to scripts, and so can not have
   * the real object as children. It must override the default Layout and Draw
   * operations and delegate by itself.
   * @see BasicElement::Layout.
   */
  virtual void Layout();

  /**
   * Returns the real object wrapped in this element.
   * Currently, it's only used by the xml utilities for the special process
   * of the param element.
   */
  BasicElement *GetObject();

  /** Gets the class id of this object. */
  const std::string& GetObjectClassId() const;
  /**
   * Sets the class id of this object.
   * Each class id indicates a specific kind of object, such as mediaplayer.
   * The class id of the object must be compatible with that on windows plaform.
   */
  void SetObjectClassId(const std::string& classId);

 protected:
  virtual void DoClassRegister();
  virtual void DoDraw(CanvasInterface *canvas);
  virtual EventResult HandleMouseEvent(const MouseEvent &event);
  virtual EventResult HandleDragEvent(const DragEvent &event);
  virtual EventResult HandleKeyEvent(const KeyboardEvent &event);
  virtual EventResult HandleOtherEvent(const Event &event);
  virtual void AggregateMoreClipRegion(const Rectangle &boundary,
                                       ClipRegion *region);

 private:
  DISALLOW_EVIL_CONSTRUCTORS(ObjectElement);
  class Impl;
  Impl *impl_;
};

} // namespace ggadget

#endif // GGADGET_OBJECT_ELEMENT_H__
