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

#include "details_view_decorator.h"

#include <string>
#include <algorithm>
#include "logger.h"
#include "common.h"
#include "gadget_consts.h"
#include "gadget_interface.h"
#include "signals.h"
#include "slot.h"
#include "view.h"
#include "view_host_interface.h"
#include "button_element.h"
#include "img_element.h"
#include "label_element.h"
#include "text_frame.h"
#include "messages.h"
#include "small_object.h"

namespace ggadget {
static const double kVDDetailsButtonHeight = 22;

class DetailsViewDecorator::Impl : public SmallObject<> {
 public:
  Impl(DetailsViewDecorator *owner)
    : owner_(owner),
      feedback_handler_(NULL),
      flags_(0) {
  }
  ~Impl() {
    delete feedback_handler_;
  }

  void OnRemoveButtonClicked() {
    flags_ = ViewInterface::DETAILS_VIEW_FLAG_REMOVE_BUTTON;
    owner_->PostCloseSignal();
  }

  void OnRemoveButtonMouseOver(ButtonElement *remove_button) {
    remove_button->SetIconImage(Variant(kVDDetailsButtonNegfbOver));
  }

  void OnRemoveButtonMouseOut(ButtonElement *remove_button) {
    remove_button->SetIconImage(Variant(kVDDetailsButtonNegfbNormal));
  }

  void OnNegativeButtonClicked() {
    flags_ = ViewInterface::DETAILS_VIEW_FLAG_NEGATIVE_FEEDBACK;
    owner_->PostCloseSignal();
  }

 public:
  DetailsViewDecorator *owner_;
  Slot1<bool, int> *feedback_handler_;
  int flags_;
};

DetailsViewDecorator::DetailsViewDecorator(ViewHostInterface *host)
  : FramedViewDecoratorBase(host, "details_view"),
    impl_(new Impl(this)) {
  SetCaptionWordWrap(true);
}

DetailsViewDecorator::~DetailsViewDecorator() {
  delete impl_;
  impl_ = NULL;
}

bool DetailsViewDecorator::ShowDecoratedView(
    bool modal, int flags, Slot1<bool, int> *feedback_handler) {
  delete impl_->feedback_handler_;
  impl_->feedback_handler_ = feedback_handler;
  RemoveActionElements();
  if (flags & DETAILS_VIEW_FLAG_NO_FRAME) {
    SetFrameVisible(false);
  } else {
    SetFrameVisible(true);
    if (flags & DETAILS_VIEW_FLAG_TOOLBAR_OPEN) {
      SetCaptionClickable(true);
    }
    if (flags & DETAILS_VIEW_FLAG_NEGATIVE_FEEDBACK) {
      ButtonElement *negative_button = new ButtonElement(this, NULL);
      negative_button->SetImage(Variant(kVDDetailsButtonBkgndNormal));
      negative_button->SetOverImage(Variant(kVDDetailsButtonBkgndOver));
      negative_button->SetDownImage(Variant(kVDDetailsButtonBkgndClick));
      negative_button->SetStretchMiddle(true);
      negative_button->SetPixelHeight(kVDDetailsButtonHeight);
      negative_button->SetVisible(true);
      negative_button->GetTextFrame()->SetText(GMS_("DONT_SHOW_CONTENT_ITEM"));
      negative_button->ConnectOnClickEvent(
          NewSlot(impl_, &Impl::OnNegativeButtonClicked));
      AddActionElement(negative_button);
    }
    if (flags & DETAILS_VIEW_FLAG_REMOVE_BUTTON) {
      ButtonElement *remove_button = new ButtonElement(this, NULL);
      remove_button->SetImage(Variant(kVDDetailsButtonBkgndNormal));
      remove_button->SetOverImage(Variant(kVDDetailsButtonBkgndOver));
      remove_button->SetDownImage(Variant(kVDDetailsButtonBkgndClick));
      remove_button->SetStretchMiddle(true);
      remove_button->GetTextFrame()->SetText(GMS_("REMOVE_CONTENT_ITEM"));
      remove_button->SetPixelHeight(kVDDetailsButtonHeight);
      remove_button->SetIconImage(Variant(kVDDetailsButtonNegfbNormal));
      remove_button->SetIconPosition(ButtonElement::ICON_RIGHT);
      remove_button->ConnectOnClickEvent(
          NewSlot(impl_, &Impl::OnRemoveButtonClicked));
      remove_button->ConnectOnMouseOverEvent(
          NewSlot(impl_, &Impl::OnRemoveButtonMouseOver, remove_button));
      remove_button->ConnectOnMouseOutEvent(
          NewSlot(impl_, &Impl::OnRemoveButtonMouseOut, remove_button));
      AddActionElement(remove_button);
    }
  }

  return FramedViewDecoratorBase::ShowDecoratedView(modal, 0, NULL);
}

void DetailsViewDecorator::CloseDecoratedView() {
  if (impl_->feedback_handler_) {
    // To make sure that openUrl can work.
    bool old_interaction = false;
    GadgetInterface *gadget = GetGadget();
    if (gadget)
      old_interaction = gadget->SetInUserInteraction(true);

    (*impl_->feedback_handler_)(impl_->flags_);
    delete impl_->feedback_handler_;
    impl_->feedback_handler_ = NULL;

    if (gadget)
      gadget->SetInUserInteraction(old_interaction);
  }
  ViewDecoratorBase::CloseDecoratedView();
}

void DetailsViewDecorator::OnCaptionClicked() {
  impl_->flags_ = ViewInterface::DETAILS_VIEW_FLAG_TOOLBAR_OPEN;
  PostCloseSignal();
}

} // namespace ggadget
