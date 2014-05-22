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

#ifndef GGADGET_TESTS_MOCKED_ELEMENT_H__
#define GGADGET_TESTS_MOCKED_ELEMENT_H__

#include "ggadget/basic_element.h"
#include "ggadget/common.h"
#include "ggadget/view.h"
#include "ggadget/elements.h"

class MuffinElement : public ggadget::BasicElement {
 public:
  MuffinElement(ggadget::View *view, const char *name)
      : ggadget::BasicElement(view, "muffin", name, true) {
  }

  virtual ~MuffinElement() {
  }

  virtual void DoDraw(ggadget::CanvasInterface *canvas) { }

 public:
  DEFINE_CLASS_ID(0x6c0dee0e5bbe11dc, ggadget::BasicElement)

 public:
  static ggadget::BasicElement *CreateInstance(ggadget::View *view,
                                               const char *name) {
    return new MuffinElement(view, name);
  }
};

class PieElement : public ggadget::BasicElement {
 public:
  PieElement(ggadget::View *view, const char *name)
      : ggadget::BasicElement(view, "pie", name, true) {
  }

  virtual ~PieElement() {
  }

  virtual void DoDraw(ggadget::CanvasInterface *canvas) { }

 public:
  DEFINE_CLASS_ID(0x829defac5bbe11dc, ggadget::BasicElement)

 public:
  static ggadget::BasicElement *CreateInstance(ggadget::View *view,
                                               const char *name) {
    return new PieElement(view, name);
  }
};

#endif // GGADGETS_TESTS_MOCKED_ELEMENT_H__
