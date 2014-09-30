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
#ifndef GOOPY_COMPONENTS_WIN_FRONTEND_COMPOSITION_WINDOW_LAYOUTER_H_
#define GOOPY_COMPONENTS_WIN_FRONTEND_COMPOSITION_WINDOW_LAYOUTER_H_
#include <windows.h>
#include <string>
#include <vector>
#include "base/basictypes.h"
#include "components/win_frontend/composition_window.h"

namespace ime_goopy {
namespace components {
namespace win_frontend {

// A class used to compute the layout of composition line window based on the
// input context.
// TODO(haicsun): Add RTL support for CompositionWindowList and
// CompositionWindowLayouter.
class CompositionWindowLayouter {
 public:
  CompositionWindowLayouter();
  // Returns false if the text could not be layouted.
  // |client_rect|: the client region of the window a input context belongs to.
  // |caret_rect|: the composition starting point/active point.
  // |log_font|: the logic font that device context uses.
  // |text|: the text to be layouted to multiple line windows.
  // |caret|: the caret position inside |text|.
  // |layouts|: the returned layout information, see CompositionWindowList for
  // more information about the structure.
  bool Layout(
      const RECT& client_rect,
      const RECT& caret_rect,
      const LOGFONT& log_font,
      const std::wstring& text,
      int caret,
      std::vector<CompositionWindowList::CompositionWindowLayout>* layouts);

 private:
  // Gathers information used to draw a line window inside client rect from
  // context.
  // Returns true if success.
  // dc : the device context the |text| will be drawn.
  // text : the composition text that needs to be drawn on the composition
  // window list.
  // index : the index of text starting from which the characters are not
  // assigned to any line window.
  // caret : the caret position of composition text.
  // offset : the top-left corner of current line window.
  // new_line : the layout structure readable by CompositionWindowList class
  // that contains information needed to draw the line window.
  bool GetOneLine(
      const RECT& client_rect,
      const LOGFONT& log_font,
      const std::wstring& text,
      HDC dc,
      int* index,
      int caret,
      POINT* offset,
      CompositionWindowList::CompositionWindowLayout* new_line);

  // Methods used to construct parts of line window layout.
  void AddWindowPositionToLayout(
      const RECT& client_rect,
      const POINT& offset,
      int line_window_width,
      int line_window_height,
      CompositionWindowList::CompositionWindowLayout* window_layout);

  void AddMarkerToLayout(
      int line_window_width,
      int line_window_height,
      CompositionWindowList::CompositionWindowLayout* window_layout);

  void AddTextAreaToLayout(
      int line_window_width,
      int line_window_height,
      CompositionWindowList::CompositionWindowLayout* window_layout);

  void AddCaretToLayout(
      int index, int allowable_chars, int caret,
      int line_window_height,
      int* width_array,
      CompositionWindowList::CompositionWindowLayout* window_layout);

  DISALLOW_COPY_AND_ASSIGN(CompositionWindowLayouter);
};

}  // namespace win_frontend
}  // namespace components
}  // namespace ime_goopy
#endif  // GOOPY_COMPONENTS_WIN_FRONTEND_COMPOSITION_WINDOW_LAYOUTER_H_
