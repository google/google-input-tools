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

#include "anchor_element.h"
#include "color.h"
#include "event.h"
#include "graphics_interface.h"
#include "string_utils.h"
#include "text_frame.h"
#include "texture.h"
#include "view.h"
#include "small_object.h"

namespace ggadget {

static const Color kDefaultColor(0.0, 0.0, 1.0);

class AnchorElement::Impl : public SmallObject<> {
 public:
  Impl(BasicElement *owner, View *view)
    : text_(owner, view),
      overcolor_texture_(new Texture(kDefaultColor, 1.0)),
      mouseover_(false) {
    text_.SetColor(kDefaultColor, 1.0);
    text_.SetUnderline(true);
  }

  ~Impl() {
    delete overcolor_texture_;
    overcolor_texture_ = NULL;
  }

  DEFINE_DELEGATE_GETTER(GetTextFrame,
                         &(down_cast<AnchorElement *>(src)->impl_->text_),
                         BasicElement, TextFrame);

  TextFrame text_;
  Texture *overcolor_texture_;
  std::string href_;
  bool mouseover_;
};

AnchorElement::AnchorElement(View *view, const char *name)
    : BasicElement(view, "a", name, true),
      impl_(new Impl(this, view)) {
  SetCursor(ViewInterface::CURSOR_HAND);
  SetEnabled(true);
}

void AnchorElement::DoClassRegister() {
  BasicElement::DoClassRegister();
  impl_->text_.RegisterClassProperties(Impl::GetTextFrame,
                                       Impl::GetTextFrameConst);
  RegisterProperty("overColor",
                   NewSlot(&AnchorElement::GetOverColor),
                   NewSlot(&AnchorElement::SetOverColor));
  RegisterProperty("href",
                   NewSlot(&AnchorElement::GetHref),
                   NewSlot(&AnchorElement::SetHref));
  RegisterProperty("innerText",
                   NewSlot(&TextFrame::GetText, Impl::GetTextFrameConst),
                   NewSlot(&TextFrame::SetText, Impl::GetTextFrame));
}

AnchorElement::~AnchorElement() {
  delete impl_;
}

void AnchorElement::DoDraw(CanvasInterface *canvas) {
  DrawChildren(canvas);
  if (impl_->mouseover_) {
    impl_->text_.DrawWithTexture(canvas, 0, 0,
                                 GetPixelWidth(), GetPixelHeight(),
                                 impl_->overcolor_texture_);
  } else {
    impl_->text_.Draw(canvas, 0, 0, GetPixelWidth(), GetPixelHeight());
  }
}

Variant AnchorElement::GetOverColor() const {
  return Variant(Texture::GetSrc(impl_->overcolor_texture_));
}

void AnchorElement::SetOverColor(const Variant &color) {
  if (color != GetOverColor()) {
    delete impl_->overcolor_texture_;
    impl_->overcolor_texture_ = GetView()->LoadTexture(color);
    if (impl_->mouseover_) {
      QueueDraw();
    }
  }
}

std::string AnchorElement::GetHref() const {
  return impl_->href_;
}

void AnchorElement::SetHref(const char *href) {
  impl_->href_ = href;
}

TextFrame *AnchorElement::GetTextFrame() {
  return &impl_->text_;
}

const TextFrame *AnchorElement::GetTextFrame() const {
  return &impl_->text_;
}

EventResult AnchorElement::HandleMouseEvent(const MouseEvent &event) {
  EventResult result = EVENT_RESULT_HANDLED;
  switch (event.GetType()) {
    case Event::EVENT_MOUSE_OUT:
      impl_->mouseover_ = false;
      QueueDraw();
      break;
    case Event::EVENT_MOUSE_OVER:
      impl_->mouseover_ = true;
      QueueDraw();
      break;
    case Event::EVENT_MOUSE_CLICK:
      // Some gadgets use the HTML convention href="#" to make the anchor have
      // no action.
      if (!impl_->href_.empty() && impl_->href_ != "#") {
         GetView()->OpenURL(impl_->href_.c_str()); // ignore return
      }
      break;
    default:
      result = EVENT_RESULT_UNHANDLED;
      break;
  }
  return result;
}

BasicElement *AnchorElement::CreateInstance(View *view, const char *name) {
  return new AnchorElement(view, name);
}

void AnchorElement::GetDefaultSize(double *width, double *height) const {
  impl_->text_.GetSimpleExtents(width, height);
}

} // namespace ggadget
