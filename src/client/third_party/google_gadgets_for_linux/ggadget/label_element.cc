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

#include "label_element.h"
#include "graphics_interface.h"
#include "text_frame.h"
#include "view.h"
#include "small_object.h"

namespace ggadget {

class LabelElement::Impl : public SmallObject<> {
 public:
  Impl(BasicElement *owner, View *view) : text_(owner, view) { }
  ~Impl() {
  }

  DEFINE_DELEGATE_GETTER(GetTextFrame,
                         &(down_cast<LabelElement *>(src)->impl_->text_),
                         BasicElement, TextFrame);

  TextFrame text_;
};

LabelElement::LabelElement(View *view, const char *name)
    : BasicElement(view, "label", name, false),
      impl_(new Impl(this, view)) {
}

void LabelElement::DoClassRegister() {
  BasicElement::DoClassRegister();
  impl_->text_.RegisterClassProperties(Impl::GetTextFrame,
                                       Impl::GetTextFrameConst);
  RegisterProperty("innerText",
                   NewSlot(&TextFrame::GetText, Impl::GetTextFrameConst),
                   NewSlot(&TextFrame::SetText, Impl::GetTextFrame));
}

LabelElement::~LabelElement() {
  delete impl_;
}

TextFrame *LabelElement::GetTextFrame() {
  return &impl_->text_;
}

const TextFrame *LabelElement::GetTextFrame() const {
  return &impl_->text_;
}

void LabelElement::DoDraw(CanvasInterface *canvas) {
  // Text direction does not affect size.
  impl_->text_.SetRTL(IsTextRTL());
  impl_->text_.Draw(canvas, 0, 0, GetPixelWidth(), GetPixelHeight());
}

BasicElement *LabelElement::CreateInstance(View *view, const char *name) {
  return new LabelElement(view, name);
}

void LabelElement::GetDefaultSize(double *width, double *height) const {
  if (!WidthIsSpecified()) {
    impl_->text_.GetSimpleExtents(width, height);
  } else {
    impl_->text_.GetExtents(GetPixelWidth(), width, height);
  }
}

} // namespace ggadget
