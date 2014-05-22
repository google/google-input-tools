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

#include "checkbox_element.h"
#include "canvas_interface.h"
#include "elements.h"
#include "gadget_consts.h"
#include "graphics_interface.h"
#include "image_interface.h"
#include "logger.h"
#include "scriptable_event.h"
#include "string_utils.h"
#include "text_frame.h"
#include "view.h"
#include "small_object.h"

namespace ggadget {

static const int kImageTextGap = 2;

enum CheckedState {
  STATE_NORMAL,
  STATE_CHECKED,
  STATE_COUNT
};

#define IMAGE(postfix) (is_checkbox_ ? kCheckBox##postfix : kRadio##postfix)
#define IMPL_IMAGE(postfix) \
    (impl_->is_checkbox_ ? kCheckBox##postfix : kRadio##postfix)

class CheckBoxElement::Impl : public SmallObject<> {
 public:
  Impl(CheckBoxElement *owner, View *view, bool is_checkbox)
    : owner_(owner),
      text_(owner, view),
      value_(STATE_CHECKED),
      is_checkbox_(is_checkbox),
      mousedown_(false),
      mouseover_(false),
      checkbox_on_right_(false),
      default_rendering_(false) {
    for (int i = 0; i < STATE_COUNT; i++) {
      image_[i] = NULL;
      downimage_[i] = NULL;
      overimage_[i] = NULL;
      disabledimage_[i] = NULL;
    }
    text_.SetTrimming(CanvasInterface::TRIMMING_CHARACTER);
    text_.SetVAlign(CanvasInterface::VALIGN_MIDDLE);
  }

  ~Impl() {
    for (int i = 0; i < STATE_COUNT; i++) {
      DestroyImage(image_[i]);
      DestroyImage(downimage_[i]);
      DestroyImage(overimage_[i]);
      DestroyImage(disabledimage_[i]);
    }
  }

  ImageInterface *GetCurrentImage() {
    ImageInterface *img = NULL;
    if (!owner_->IsEnabled()) {
      img = disabledimage_[value_];
    } else if (mousedown_) {
      img = downimage_[value_];
    } else if (mouseover_) {
      img = overimage_[value_];
    }

    if (!img) { // Leave this case as fallback if the exact image is NULL.
      img = image_[value_];
    }
    return img;
  }

  void ResetPeerRadioButtons() {
    BasicElement *parent = owner_->GetParentElement();
    Elements *peers = parent ?
                      parent->GetChildren() : owner_->GetView()->GetChildren();
    // Radio buttons under the same parent should transfer checked
    // state automatically. This function should only be called
    // when this radio button's value is set to true.
    size_t childcount = peers->GetCount();
    for (size_t i = 0; i < childcount; i++) {
      BasicElement *child = peers->GetItemByIndex(i);
      if (child != owner_ && child->IsInstanceOf(CheckBoxElement::CLASS_ID)) {
        CheckBoxElement *radio = down_cast<CheckBoxElement *>(child);
        if (!radio->IsCheckBox()) {
          radio->SetValue(false);
        }
      }
    }
  }

  void LoadImage(ImageInterface **image, const Variant &src) {
    if (src != Variant(GetImageTag(*image))) {
      DestroyImage(*image);
      *image = owner_->GetView()->LoadImage(src, false);
      owner_->QueueDraw();
    }
  }

  void EnsureDefaultImages() {
    if (default_rendering_) {
      View *view = owner_->GetView();
      if (!image_[STATE_NORMAL]) {
        image_[STATE_NORMAL] = view->LoadImageFromGlobal(IMAGE(Image), false);
      }
      if (!overimage_[STATE_NORMAL]) {
        overimage_[STATE_NORMAL] = view->LoadImageFromGlobal(
            IMAGE(OverImage), false);
      }
      if (!downimage_[STATE_NORMAL]) {
        downimage_[STATE_NORMAL] = view->LoadImageFromGlobal(
            IMAGE(DownImage), false);
      }
      if (!image_[STATE_CHECKED]) {
        image_[STATE_CHECKED] = view->LoadImageFromGlobal(
            IMAGE(CheckedImage), false);
      }
      if (!overimage_[STATE_CHECKED]) {
        overimage_[STATE_CHECKED] = view->LoadImageFromGlobal(
            IMAGE(CheckedOverImage), false);
      }
      if (!downimage_[STATE_CHECKED]) {
        downimage_[STATE_CHECKED] = view->LoadImageFromGlobal(
            IMAGE(CheckedDownImage), false);
      }
      // No default disabled images.
    }
  }

  void DestroyDefaultImages() {
    if (GetImageTag(image_[STATE_NORMAL]) == IMAGE(Image)) {
      DestroyImage(image_[STATE_NORMAL]);
      image_[STATE_NORMAL] = NULL;
    }
    if (GetImageTag(overimage_[STATE_NORMAL]) == IMAGE(OverImage)) {
      DestroyImage(overimage_[STATE_NORMAL]);
      overimage_[STATE_NORMAL] = NULL;
    }
    if (GetImageTag(downimage_[STATE_NORMAL]) == IMAGE(DownImage)) {
      DestroyImage(downimage_[STATE_NORMAL]);
      downimage_[STATE_NORMAL] = NULL;
    }
    if (GetImageTag(image_[STATE_CHECKED]) == IMAGE(CheckedImage)) {
      DestroyImage(image_[STATE_CHECKED]);
      image_[STATE_CHECKED] = NULL;
    }
    if (GetImageTag(overimage_[STATE_CHECKED]) == IMAGE(CheckedOverImage)) {
      DestroyImage(overimage_[STATE_CHECKED]);
      overimage_[STATE_CHECKED] = NULL;
    }
    if (GetImageTag(downimage_[STATE_CHECKED]) == IMAGE(CheckedDownImage)) {
      DestroyImage(downimage_[STATE_CHECKED]);
      downimage_[STATE_CHECKED] = NULL;
    }
    // No default disabled images.
  }

  DEFINE_DELEGATE_GETTER(GetTextFrame,
                         &(down_cast<CheckBoxElement *>(src)->impl_->text_),
                         BasicElement, TextFrame);

  CheckBoxElement *owner_;
  TextFrame text_;
  ImageInterface *image_[STATE_COUNT], *downimage_[STATE_COUNT],
                 *overimage_[STATE_COUNT], *disabledimage_[STATE_COUNT];
  EventSignal onchange_event_;

  CheckedState value_           : 2;
  bool is_checkbox_             : 1;
  bool mousedown_               : 1;
  bool mouseover_               : 1;
  bool checkbox_on_right_       : 1;
  bool default_rendering_       : 1;
};

CheckBoxElement::CheckBoxElement(View *view, const char *name, bool is_checkbox)
    : BasicElement(view, is_checkbox ? "checkbox" : "radio", name, false),
      impl_(new Impl(this, view, is_checkbox)) {
  SetEnabled(true);
}

void CheckBoxElement::DoClassRegister() {
  BasicElement::DoClassRegister();
  impl_->text_.RegisterClassProperties(Impl::GetTextFrame,
                                       Impl::GetTextFrameConst);
  RegisterProperty("value",
                   NewSlot(&CheckBoxElement::GetValue),
                   NewSlot(&CheckBoxElement::SetValue));
  RegisterProperty("image",
                   NewSlot(&CheckBoxElement::GetImage),
                   NewSlot(&CheckBoxElement::SetImage));
  RegisterProperty("downImage",
                   NewSlot(&CheckBoxElement::GetDownImage),
                   NewSlot(&CheckBoxElement::SetDownImage));
  RegisterProperty("overImage",
                   NewSlot(&CheckBoxElement::GetOverImage),
                   NewSlot(&CheckBoxElement::SetOverImage));
  RegisterProperty("disabledImage",
                   NewSlot(&CheckBoxElement::GetDisabledImage),
                   NewSlot(&CheckBoxElement::SetDisabledImage));
  RegisterProperty("checkedImage",
                   NewSlot(&CheckBoxElement::GetCheckedImage),
                   NewSlot(&CheckBoxElement::SetCheckedImage));
  RegisterProperty("checkedDownImage",
                   NewSlot(&CheckBoxElement::GetCheckedDownImage),
                   NewSlot(&CheckBoxElement::SetCheckedDownImage));
  RegisterProperty("checkedOverImage",
                   NewSlot(&CheckBoxElement::GetCheckedOverImage),
                   NewSlot(&CheckBoxElement::SetCheckedOverImage));
  RegisterProperty("checkedDisabledImage",
                   NewSlot(&CheckBoxElement::GetCheckedDisabledImage),
                   NewSlot(&CheckBoxElement::SetCheckedDisabledImage));
  RegisterProperty("caption",
                   NewSlot(&TextFrame::GetText, Impl::GetTextFrameConst),
                   NewSlot(&TextFrame::SetText, Impl::GetTextFrame));
  RegisterProperty("checkboxOnRight",
                   NewSlot(&CheckBoxElement::IsCheckBoxOnRight),
                   NewSlot(&CheckBoxElement::SetCheckBoxOnRight));

  // Undocumented property.
  RegisterProperty("defaultRendering",
                   NewSlot(&CheckBoxElement::IsDefaultRendering),
                   NewSlot(&CheckBoxElement::SetDefaultRendering));

  RegisterClassSignal(kOnChangeEvent, &Impl::onchange_event_,
                      &CheckBoxElement::impl_);
}

CheckBoxElement::~CheckBoxElement() {
  delete impl_;
  impl_ = NULL;
}

void CheckBoxElement::DoDraw(CanvasInterface *canvas) {
  impl_->EnsureDefaultImages();
  ImageInterface *img = impl_->GetCurrentImage();

  const double h = GetPixelHeight();
  double textx = 0;
  double textwidth = GetPixelWidth();
  if (img) {
    double imgw = img->GetWidth();
    textwidth -= imgw;
    double imgx;
    if (impl_->checkbox_on_right_) {
      imgx = textwidth + kImageTextGap;
    } else {
      textx = imgw + kImageTextGap;
      imgx = 0.;
    }
    img->Draw(canvas, imgx, (h - img->GetHeight()) / 2.);
  }
  impl_->text_.Draw(canvas, textx, 0., textwidth, h);
}

bool CheckBoxElement::IsCheckBoxOnRight() const {
  return impl_->checkbox_on_right_;
}

void CheckBoxElement::SetCheckBoxOnRight(bool right) {
  if (right != impl_->checkbox_on_right_) {
    impl_->checkbox_on_right_ = right;
    QueueDraw();
  }
}

bool CheckBoxElement::IsCheckBox() const {
  return impl_->is_checkbox_;
}

bool CheckBoxElement::GetValue() const {
  return (impl_->value_ == STATE_CHECKED);
}

void CheckBoxElement::SetValue(bool value) {
  if (value != (impl_->value_ == STATE_CHECKED)) {
    QueueDraw();
    impl_->value_ = value ? STATE_CHECKED : STATE_NORMAL;
    SimpleEvent event(Event::EVENT_CHANGE);
    ScriptableEvent s_event(&event, this, NULL);
    GetView()->FireEvent(&s_event, impl_->onchange_event_);
  }

  if (!impl_->is_checkbox_ && value) {
    impl_->ResetPeerRadioButtons();
  }
}

Variant CheckBoxElement::GetImage() const {
  std::string tag = GetImageTag(impl_->image_[STATE_NORMAL]);
  return Variant(tag == IMPL_IMAGE(Image) ? "" : tag);
}

void CheckBoxElement::SetImage(const Variant &img) {
  impl_->LoadImage(&impl_->image_[STATE_NORMAL], img);
}

Variant CheckBoxElement::GetDisabledImage() const {
  return Variant(GetImageTag(impl_->disabledimage_[STATE_NORMAL]));
}

void CheckBoxElement::SetDisabledImage(const Variant &img) {
  impl_->LoadImage(&impl_->disabledimage_[STATE_NORMAL], img);
}

Variant CheckBoxElement::GetOverImage() const {
  std::string tag = GetImageTag(impl_->overimage_[STATE_NORMAL]);
  return Variant(tag == IMPL_IMAGE(OverImage) ? "" : tag);
}

void CheckBoxElement::SetOverImage(const Variant &img) {
  impl_->LoadImage(&impl_->overimage_[STATE_NORMAL], img);
}

Variant CheckBoxElement::GetDownImage() const {
  std::string tag = GetImageTag(impl_->downimage_[STATE_NORMAL]);
  return Variant(tag == IMPL_IMAGE(DownImage) ? "" : tag);
}

void CheckBoxElement::SetDownImage(const Variant &img) {
  impl_->LoadImage(&impl_->downimage_[STATE_NORMAL], img);
}

Variant CheckBoxElement::GetCheckedImage() const {
  std::string tag = GetImageTag(impl_->image_[STATE_CHECKED]);
  return Variant(tag == IMPL_IMAGE(CheckedImage) ? "" : tag);
}

void CheckBoxElement::SetCheckedImage(const Variant &img) {
  impl_->LoadImage(&impl_->image_[STATE_CHECKED], img);
}

Variant CheckBoxElement::GetCheckedDisabledImage() const {
  return Variant(GetImageTag(impl_->disabledimage_[STATE_CHECKED]));
}

void CheckBoxElement::SetCheckedDisabledImage(const Variant &img) {
  impl_->LoadImage(&impl_->disabledimage_[STATE_CHECKED], img);
}

Variant CheckBoxElement::GetCheckedOverImage() const {
  std::string tag = GetImageTag(impl_->overimage_[STATE_CHECKED]);
  return Variant(tag == IMPL_IMAGE(CheckedOverImage) ? "" : tag);
}

void CheckBoxElement::SetCheckedOverImage(const Variant &img) {
  impl_->LoadImage(&impl_->overimage_[STATE_CHECKED], img);
}

Variant CheckBoxElement::GetCheckedDownImage() const {
  std::string tag = GetImageTag(impl_->downimage_[STATE_CHECKED]);
  return Variant(tag == IMPL_IMAGE(CheckedDownImage) ? "" : tag);
}

void CheckBoxElement::SetCheckedDownImage(const Variant &img) {
  impl_->LoadImage(&impl_->downimage_[STATE_CHECKED], img);
}

bool CheckBoxElement::IsDefaultRendering() const {
  return impl_->default_rendering_;
}

void CheckBoxElement::SetDefaultRendering(bool default_rendering) {
  if (default_rendering != impl_->default_rendering_) {
    impl_->default_rendering_ = default_rendering;
    if (!default_rendering)
      impl_->DestroyDefaultImages();
    QueueDraw();
  }
}

TextFrame *CheckBoxElement::GetTextFrame() {
  return &impl_->text_;
}

const TextFrame *CheckBoxElement::GetTextFrame() const {
  return &impl_->text_;
}

EventResult CheckBoxElement::HandleMouseEvent(const MouseEvent &event) {
  EventResult result = EVENT_RESULT_HANDLED;
  switch (event.GetType()) {
    case Event::EVENT_MOUSE_DOWN:
      if (event.GetButton() & MouseEvent::BUTTON_LEFT) {
        impl_->mousedown_ = true;
        QueueDraw();
      }
      break;
    case Event::EVENT_MOUSE_UP:
      if (impl_->mousedown_) {
        impl_->mousedown_ = false;
        QueueDraw();
      }
      break;
    case Event::EVENT_MOUSE_OUT:
      impl_->mouseover_ = false;
      QueueDraw();
      break;
    case Event::EVENT_MOUSE_OVER:
      impl_->mouseover_ = true;
      QueueDraw();
      break;
    case Event::EVENT_MOUSE_CLICK: {
      // Toggle checked state and fire event
      if (impl_->is_checkbox_) {
        impl_->value_ = (impl_->value_ == STATE_NORMAL) ?
                          STATE_CHECKED : STATE_NORMAL;
      } else {
        if (impl_->value_ == STATE_CHECKED) {
          break; // Radio buttons don't change state in this situation.
        }
        impl_->value_ = STATE_CHECKED;
        impl_->ResetPeerRadioButtons();
      }
      QueueDraw();
      SimpleEvent event(Event::EVENT_CHANGE);
      ScriptableEvent s_event(&event, this, NULL);
      GetView()->FireEvent(&s_event, impl_->onchange_event_);
      break;
    }
    default:
      result = EVENT_RESULT_UNHANDLED;
      break;
  }
  return result;
}

Connection *CheckBoxElement::ConnectOnChangeEvent(Slot0<void> *handler) {
  return impl_->onchange_event_.Connect(handler);
}

void CheckBoxElement::GetDefaultSize(double *width, double *height) const {
  impl_->EnsureDefaultImages();

  double image_width = 0, image_height = 0;
  ImageInterface *image = impl_->GetCurrentImage();
  if (image) {
    image_width = image->GetWidth();
    image_height = image->GetHeight();
  }

  double text_width = 0, text_height = 0;
  impl_->text_.GetSimpleExtents(&text_width, &text_height);

  *width = image_width + text_width + kImageTextGap;
  *height = std::max(image_height, text_height);
}

BasicElement *CheckBoxElement::CreateCheckBoxInstance(View *view,
                                                      const char *name) {
  return new CheckBoxElement(view, name, true);
}

BasicElement *CheckBoxElement::CreateRadioInstance(View *view,
                                                   const char *name) {
  return new CheckBoxElement(view, name, false);
}

} // namespace ggadget
