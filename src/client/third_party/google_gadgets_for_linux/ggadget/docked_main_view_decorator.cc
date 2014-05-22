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

#include "docked_main_view_decorator.h"

#include <string>
#include <algorithm>
#include "logger.h"
#include "common.h"
#include "gadget_consts.h"
#include "elements.h"
#include "signals.h"
#include "slot.h"
#include "view.h"
#include "view_host_interface.h"
#include "div_element.h"
#include "img_element.h"
#include "messages.h"
#include "menu_interface.h"
#include "small_object.h"

namespace ggadget {

static const double kVDMainDockedResizeBorderWidth = 3;

class DockedMainViewDecorator::Impl : public SmallObject<> {
 public:
  struct ResizeBorderInfo {
    double x;           // relative x
    double y;           // relative y
    double pin_x;       // relative pin x
    double pin_y;       // relative pin y
    double width;       // pixel width < 0 means relative width = 1.0
    double height;      // pixel height < 0 means relative height = 1.0
    const char *img;
    ViewInterface::CursorType cursor;
    ViewInterface::HitTest hittest;
  };

  static const ResizeBorderInfo kResizeBordersInfo[];

  Impl(DockedMainViewDecorator *owner)
    : owner_(owner) {
    for (size_t i = 0; i < 4; ++i)
      resize_borders_[i] = NULL;
  }

  void UndockMenuCallback(const char *) {
    on_undock_signal_();
  }

  void SetupResizeBorder(int borders) {
    bool visibles[4] = {
      (borders & BORDER_TOP) != 0,
      (borders & BORDER_LEFT) != 0,
      (borders & BORDER_BOTTOM) != 0,
      (borders & BORDER_RIGHT) != 0
    };
    for (size_t i = 0; i < 4; ++i) {
      if (!visibles[i] && resize_borders_[i]) {
        owner_->GetChildren()->RemoveElement(resize_borders_[i]);
        resize_borders_[i] = NULL;
      } else if (visibles[i] && !resize_borders_[i]) {
        resize_borders_[i] = new ImgElement(owner_, NULL);
        resize_borders_[i]->SetStretchMiddle(true);
        resize_borders_[i]->SetVisible(false);
        resize_borders_[i]->SetEnabled(false);
        resize_borders_[i]->SetSrc(Variant(kResizeBordersInfo[i].img));
        resize_borders_[i]->SetRelativeX(kResizeBordersInfo[i].x);
        resize_borders_[i]->SetRelativeY(kResizeBordersInfo[i].y);
        resize_borders_[i]->SetRelativePinX(kResizeBordersInfo[i].pin_x);
        resize_borders_[i]->SetRelativePinY(kResizeBordersInfo[i].pin_y);
        if (kResizeBordersInfo[i].width < 0)
          resize_borders_[i]->SetRelativeWidth(1);
        if (kResizeBordersInfo[i].height < 0)
          resize_borders_[i]->SetRelativeHeight(1);
        resize_borders_[i]->SetCursor(kResizeBordersInfo[i].cursor);
        resize_borders_[i]->SetHitTest(kResizeBordersInfo[i].hittest);
        owner_->InsertDecoratorElement(resize_borders_[i], true);
      }
    }
  }

 public:
  DockedMainViewDecorator *owner_;
  ImgElement *resize_borders_[4];
  Signal0<void> on_undock_signal_;
};

const DockedMainViewDecorator::Impl::ResizeBorderInfo
DockedMainViewDecorator::Impl::kResizeBordersInfo[] = {
  // Top
  { 0, 0, 0, 0, -1, 0, kVDMainDockedBorderH,
    ViewInterface::CURSOR_SIZENS, ViewInterface::HT_TOP },
  // Left
  { 0, 0, 0, 0, 0, -1, kVDMainDockedBorderV,
    ViewInterface::CURSOR_SIZEWE, ViewInterface::HT_LEFT },
  // Bottom
  { 0, 1, 0, 1, -1, 0, kVDMainDockedBorderH,
    ViewInterface::CURSOR_SIZENS, ViewInterface::HT_BOTTOM },
  // Right
  { 1, 0, 1, 0, 0, -1, kVDMainDockedBorderV,
    ViewInterface::CURSOR_SIZEWE, ViewInterface::HT_RIGHT },
};

DockedMainViewDecorator::DockedMainViewDecorator(ViewHostInterface *host)
  : MainViewDecoratorBase(host, "main_view_docked", true, false, true),
    impl_(new Impl(this)) {
  SetDecoratorShowHideTimeout(0, 0);
  SetResizeBorderVisible(BORDER_BOTTOM);
}

DockedMainViewDecorator::~DockedMainViewDecorator() {
  delete impl_;
  impl_ = NULL;
}

void DockedMainViewDecorator::SetResizeBorderVisible(int borders) {
  impl_->SetupResizeBorder(borders);
  UpdateViewSize();
}

Connection *DockedMainViewDecorator::ConnectOnUndock(Slot0<void> *slot) {
  return impl_->on_undock_signal_.Connect(slot);
}

void DockedMainViewDecorator::GetMargins(double *left, double *top,
                                         double *right, double *bottom) const {
  ButtonBoxPosition button_position = GetButtonBoxPosition();
  ButtonBoxOrientation button_orientation = GetButtonBoxOrientation();

  double *btn_edge = NULL;
  double btn_margin = 0;
  double btn_width, btn_height;
  GetButtonBoxSize(&btn_width, &btn_height);

  if (button_orientation == HORIZONTAL) {
    btn_margin = btn_height;
    if (button_position == TOP_LEFT || button_position == TOP_RIGHT)
      btn_edge = top;
    else
      btn_edge = bottom;
  } else {
    btn_margin = btn_width;
    if (button_position == TOP_LEFT || button_position == BOTTOM_LEFT)
      btn_edge = left;
    else
      btn_edge = right;
  }

  *left = (impl_->resize_borders_[1] ? kVDMainDockedResizeBorderWidth : 0);
  *top = (impl_->resize_borders_[0] ? kVDMainDockedResizeBorderWidth : 0);
  *right = (impl_->resize_borders_[3] ? kVDMainDockedResizeBorderWidth : 0);
  *bottom = (impl_->resize_borders_[2] ? kVDMainDockedResizeBorderWidth : 0);

  if (!IsMinimized())
    *btn_edge = btn_margin;
}

void DockedMainViewDecorator::OnAddDecoratorMenuItems(MenuInterface *menu) {
  int priority = MenuInterface::MENU_ITEM_PRI_DECORATOR;

  AddCollapseExpandMenuItem(menu);

  if (impl_->on_undock_signal_.HasActiveConnections()) {
    menu->AddItem(GM_("MENU_ITEM_UNDOCK_FROM_SIDEBAR"), 0, 0,
                  NewSlot(impl_, &Impl::UndockMenuCallback), priority);
  }

  MainViewDecoratorBase::OnAddDecoratorMenuItems(menu);
}

void DockedMainViewDecorator::OnShowDecorator() {
  for (size_t i = 0; i < 4; ++i) {
    if (impl_->resize_borders_[i])
      impl_->resize_borders_[i]->SetVisible(true);
  }
  SetButtonBoxVisible(true);
}

void DockedMainViewDecorator::OnHideDecorator() {
  for (size_t i = 0; i < 4; ++i) {
    if (impl_->resize_borders_[i])
      impl_->resize_borders_[i]->SetVisible(false);
  }
  SetButtonBoxVisible(false);
}

} // namespace ggadget
