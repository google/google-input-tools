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

#ifndef GGADGET_LABEL_ELEMENT_H__
#define GGADGET_LABEL_ELEMENT_H__

#include <stdlib.h>
#include <ggadget/basic_element.h>

namespace ggadget {

class TextFrame;

/**
 * @ingroup Elements
 * Class of <a href=
 * "http://code.google.com/apis/desktop/docs/gadget_apiref.html#label">
 * label element</a>.
 */
class LabelElement : public BasicElement {
 public:
  DEFINE_CLASS_ID(0x4b128d3ef8da40e6, BasicElement);

  LabelElement(View *view, const char *name);
  virtual ~LabelElement();

  //@{
  /** Gets the text frame containing the text content of this label. */
  TextFrame *GetTextFrame();
  const TextFrame *GetTextFrame() const;
  //@}

 public:
  static BasicElement *CreateInstance(View *view, const char *name);

 protected:
  virtual void DoClassRegister();
  virtual void DoDraw(CanvasInterface *canvas);
  virtual void GetDefaultSize(double *width, double *height) const;

 private:
  DISALLOW_EVIL_CONSTRUCTORS(LabelElement);

  class Impl;
  Impl *impl_;
};

} // namespace ggadget

#endif // GGADGET_LABEL_ELEMENT_H__
