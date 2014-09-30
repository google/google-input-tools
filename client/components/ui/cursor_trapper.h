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
#ifndef GOOPY_COMPONENTS_UI_CURSOR_TRAPPER_H_
#define GOOPY_COMPONENTS_UI_CURSOR_TRAPPER_H_

#include "base/macros.h"  // For DISALLOW_COPY_AND_ASSIGN

namespace ggadget {

class BasicElement;
class ViewHostInterface;

}  // namespace ggadget

namespace ime_goopy {
namespace components {

// A class to trap the mouse cursor with a skin element.
// It saves the relative position of cursor from the element when constructed
// and restores the cursor position when destroyed.
class CursorTrapper {
 public:
  CursorTrapper();
  ~CursorTrapper();
  void Save(ggadget::BasicElement* element);
  void Restore();

 private:
  ggadget::BasicElement* element_;
  double offset_in_element_x_;
  double offset_in_element_y_;
  DISALLOW_COPY_AND_ASSIGN(CursorTrapper);
};

}  // namespace components
}  // namespace ime_goopy

#endif  // GOOPY_COMPONENTS_UI_CURSOR_TRAPPER_H_
