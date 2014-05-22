/*
  Copyright 2014 Google Inc.

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
#include "components/ui/cursor_trapper.h"

#include "components/ui/skin_ui_component_utils.h"
#include "third_party/google_gadgets_for_linux/ggadget/basic_element.h"
#include "third_party/google_gadgets_for_linux/ggadget/view.h"

namespace ime_goopy {
namespace components {

CursorTrapper::CursorTrapper()
    : element_(NULL),
      offset_in_element_x_(0),
      offset_in_element_y_(0) {
}

CursorTrapper::~CursorTrapper() {
}

void CursorTrapper::Save(ggadget::BasicElement* element) {
  element_ = element;
  if (!element || !element->GetView() || !element->GetView()->GetViewHost()) {
    element_ = NULL;
    return;
  }
  ggadget::View* view = element_->GetView();

  Point<int> cursor_on_view = SkinUIComponentUtils::GetCursorPosOnView(
      view->GetViewHost());

  Point<double> element_origin;
  element_->SelfCoordToViewCoord(0, 0, &element_origin.x, &element_origin.y);
  view->ViewCoordToNativeWidgetCoord(element_origin.x, element_origin.y,
                                     &element_origin.x, &element_origin.y);

  offset_in_element_x_ = cursor_on_view.x - element_origin.x;
  offset_in_element_y_ = cursor_on_view.y - element_origin.y;
}

void CursorTrapper::Restore() {
  if (!element_)
    return;
  ggadget::View* view = element_->GetView();

  Point<double> element_origin;
  element_->SelfCoordToViewCoord(0, 0, &element_origin.x, &element_origin.y);
  view->ViewCoordToNativeWidgetCoord(element_origin.x, element_origin.y,
                                     &element_origin.x, &element_origin.y);

  Point<int> cursor_on_view;
  cursor_on_view.x = element_origin.x + offset_in_element_x_;
  cursor_on_view.y = element_origin.y + offset_in_element_y_;

  SkinUIComponentUtils::SetCursorPosOnView(view->GetViewHost(),
                                           cursor_on_view);

  element_ = NULL;
}

}  // namespace components
}  // namespace ime_goopy
