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

#ifndef GGADGET_ANCHOR_ELEMENT_H__
#define GGADGET_ANCHOR_ELEMENT_H__

#include <stdlib.h>
#include <ggadget/basic_element.h>

namespace ggadget {

class TextFrame;

/**
 * @ingroup Elements
 * Class of <a href=
 * "http://code.google.com/apis/desktop/docs/gadget_apiref.html#a">
 * anchor element</a>.
 */
class AnchorElement : public BasicElement {
 public:
  DEFINE_CLASS_ID(0x50ef5c291807400c, BasicElement);

  AnchorElement(View *view, const char *name);
  virtual ~AnchorElement();

 protected:
  virtual void DoClassRegister();

 public:
  /**
   * Gets the mouseover text color or texture image of the element.
   */
  Variant GetOverColor() const;
  /**
   * Sets the mouseover text color or texture image of the element.
   * The image is repeated if necessary, not stretched.
   */
  void SetOverColor(const Variant &color);

  /** Gets the URL to be launched when this link is clicked. */
  std::string GetHref() const;
  /** Sets the URL to be launched when this link is clicked. */
  void SetHref(const char *href);

  //@{
  /** Gets the text frame containing the text content of this anchor. */
  TextFrame *GetTextFrame();
  const TextFrame *GetTextFrame() const;
  //@}

 public:
  static BasicElement *CreateInstance(View *view, const char *name);

 protected:
  virtual void DoDraw(CanvasInterface *canvas);
  virtual EventResult HandleMouseEvent(const MouseEvent &event);
  virtual void GetDefaultSize(double *width, double *height) const;

 private:
  DISALLOW_EVIL_CONSTRUCTORS(AnchorElement);

  class Impl;
  Impl *impl_;
};

} // namespace ggadget

#endif // GGADGET_ANCHOR_ELEMENT_H__
