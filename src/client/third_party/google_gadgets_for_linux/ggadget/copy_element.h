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

#ifndef GGADGET_COPY_ELEMENT_H__
#define GGADGET_COPY_ELEMENT_H__

#include <ggadget/basic_element.h>

namespace ggadget {

/**
 * @ingroup Elements
 * An element to copy the content of another element.
 *
 * It's an internal element used by DecoratedViewHost to support fake View
 * image when the view is popped out.
 */
class CopyElement : public BasicElement {
 public:
  DEFINE_CLASS_ID(0x21507d12bbc44fd9, BasicElement);

  CopyElement(View *view, const char *name);
  virtual ~CopyElement();

  /**
   * Checks if content if this CopyElement is frozen.
   * A frozen CopyElement only contains a still image of the source element.
   */
  bool IsFrozen() const;

  /** Sets frozen state. */
  void SetFrozen(bool frozen);

  double GetSrcWidth() const;
  double GetSrcHeight() const;

 protected:
  virtual void DoClassRegister();

 public:
  virtual bool IsPointIn(double x, double y) const;

 public:
  /**
   * Gets and sets the source element to copy.
   *
   * It can be an element object or name of an element object in the view.
   */
  Variant GetSrc() const;
  void SetSrc(const Variant &src);

 public:
  static BasicElement *CreateInstance(View *view, const char *name);

 protected:
  virtual void DoDraw(CanvasInterface *canvas);
  virtual void GetDefaultSize(double *width, double *height) const;

 public:
  virtual void MarkRedraw();
  virtual bool HasOpaqueBackground() const;

 private:
  DISALLOW_EVIL_CONSTRUCTORS(CopyElement);

  class Impl;
  Impl *impl_;
};

} // namespace ggadget

#endif // GGADGET_COPY_ELEMENT_H__
