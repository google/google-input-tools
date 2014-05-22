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

#ifndef GOOPY_COMPONENTS_WIN_FRONTEND_COMPOSITION_WINDOW_H_
#define GOOPY_COMPONENTS_WIN_FRONTEND_COMPOSITION_WINDOW_H_

#include <windows.h>
#include <vector>
#include "base/basictypes.h"

namespace ime_goopy {
namespace components {
namespace win_frontend {

class CompositionWindowList {
 public:
  struct SegmentMarkerLayout {
    POINT from;
    POINT to;
    bool highlighted;
  };

  struct CompositionWindowLayout {
    RECT window_position_in_screen_coordinate;
    RECT text_area;
    RECT caret_rect;
    POINT base_position;
    LOGFONT log_font;
    std::wstring text;
    std::vector<SegmentMarkerLayout> marker_layouts;
  };

  CompositionWindowList() {}
  virtual ~CompositionWindowList() {}

  virtual void Initialize() = 0;
  virtual void AsyncHide() = 0;
  virtual void AsyncQuit() = 0;
  virtual void Destroy() = 0;
  virtual void Hide() = 0;
  virtual void UpdateLayout(
      const std::vector<CompositionWindowLayout> &layouts) = 0;

  static CompositionWindowList *CreateInstance();

 private:
  DISALLOW_COPY_AND_ASSIGN(CompositionWindowList);
};

}  // namespace win_frontend
}  // namespace components
}  // namespace ime_goopy
#endif  // GOOPY_COMPONENTS_WIN_FRONTEND_COMPOSITION_WINDOW_H_
