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
#include "components/win_frontend/composition_window_layouter.h"
#include <string>
#include <vector>
#include "base/logging.h"
#include "base/scoped_ptr.h"
#include "common/atl.h"

namespace ime_goopy {
namespace components {
namespace win_frontend {

CompositionWindowLayouter::CompositionWindowLayouter() {
}

// Fits given text with caret into line windows.
// The basic idea here is:
// 1) Calculates the line start offset, which could be determined by
//    client_rect and caret_rect.
//    start_offset.x = client_rect.right - caret_rect.right;
//    start_offset.y = caret_rect.top;
//
// 2) Computes the maximium number of characters that fits in the left space
//    from start_offset to the right edge of client_rect without line break by
//    evaluate it by a compatible DC with given log_font, and gets the
//    coordiantion information of space occupied.
//
// 3) Fills the new added CompositionWindowList::CompositionWindowLayout
//    structure with information generated in prevous step, and pops those
//    characters from text. If text is empty, then the process is over.
//
// 4) if caret is in-between the characters of current line, also calculates the
//    caret position by adding character extents one by one to the line
//    begining.
//
// 4) If there are still characters remain, starts a new line by moving line
//    start offset down.
//    start_offset.x = 0;
//    start_offset.y += line_height;
//    repeat step 2).
//
// This is an "Hello world" example.
//
//              .---------------------> Application composition rect(caret_rect)
//              |                       with width equals to 0.
//              |
//    --------------------------
//    | x  x  x |H  e  l  l  o |  ----> First line starts from caret_rect.
//    |          ------------- |        five characters fits.
//    |    W  o |r  l  d.      |  ----> Second line starts from line begining.
//    |------------------      |        all others fits.
//    |         |              |
//    |          ----------->  |  ----> Our caret position, calcuated by adding
//    |                        |        three characters' {' ', 'W', 'o'}
//    |                        |        extents to the begining of the line.
//    --------------------------  -----> Client rect.
//
bool CompositionWindowLayouter::Layout(
    const RECT& client_rect,
    const RECT& caret_rect,
    const LOGFONT& log_font,
    const std::wstring& text,
    int caret,
    std::vector<CompositionWindowList::CompositionWindowLayout>* layouts) {
  POINT current_pos = {0};
  current_pos.x = caret_rect.left;
  current_pos.y = caret_rect.top;

  CFont new_font(CLogFont(log_font).CreateFontIndirectW());
  CDC dc;
  // Create a memory DC compatible with desktop DC.
  dc.CreateCompatibleDC(CDC(::GetDC(NULL)));
  CFontHandle old_font = dc.SelectFont(new_font);

  POINT offset;
  offset.x = current_pos.x - client_rect.left;
  offset.y = current_pos.y - client_rect.top;
  int new_index = 0;
  while (new_index != text.size()) {
    CompositionWindowList::CompositionWindowLayout new_line;
    if (!GetOneLine(client_rect, log_font, text, dc,
                    &new_index, caret, &offset, &new_line)) {
      break;
    }
    layouts->push_back(new_line);
  }
  dc.SelectFont(old_font);
  return !layouts->empty();
}

bool CompositionWindowLayouter::GetOneLine(
    const RECT& client_rect,
    const LOGFONT& log_font,
    const std::wstring& text,
    HDC hdc,
    int* index,
    int caret,
    POINT* offset,
    CompositionWindowList::CompositionWindowLayout* new_line) {
  if (*index >= text.size()) {
    LOG(ERROR) << "Invalid index of text";
    return false;
  }
  const int remaining_chars = text.size() - *index;
  const int remaining_extent =
      client_rect.right - client_rect.left - offset->x;
  CSize dummy;
  int allowable_chars = 0;
  CDC dc;
  dc.Attach(hdc);
  BOOL result = dc.GetTextExtentExPoint(
      text.c_str() + *index,
      remaining_chars,
      &dummy,
      remaining_extent,
      &allowable_chars,
      NULL);
  dc.Detach();
  if (allowable_chars == 0 && offset->x == 0) {
    // In this case, the region does not have enough space to display
    // the next character.
    return false;
  }
  // Just in case GetTextExtentExPoint returns true but the returned value
  // is invalid.  We have not seen any problem around this API though.
  if (allowable_chars < 0 || remaining_chars < allowable_chars) {
    // Something wrong.
    LOG(ERROR) << "(allowable_chars, remaining_chars) = ("
               << allowable_chars << ", " <<
               remaining_chars << ")";
    return false;
  }

  // Calculates the extent of line window.
  int line_window_height = 0;
  int line_window_width = 0;
  CompositionWindowList::CompositionWindowLayout window_layout;
  if (allowable_chars == 0) {
    // This case occurs only when this line does not have enough space to
    // render the next character from |current_offset|.  We will try to
    // render text in the next line.  Note that an infinite loop should
    // never occur because we have already checked the case above.  This is
    // why |current_offset| should be positive here.
    DCHECK_GT(offset->x, 0);

    TEXTMETRIC metrics = {0};
    if (!::GetTextMetrics(hdc, &metrics)) {
      const int error = ::GetLastError();
      LOG(ERROR) << "GetTextMetrics failed. error = " << error;
      return false;
    }
    line_window_height = metrics.tmHeight;
  } else {
    SIZE line_size = {0};
    int allowable_chars_for_confirmation = 0;
    scoped_array<int> size_buffer(new int[allowable_chars]);
    dc.Attach(hdc);
    result = dc.GetTextExtentExPoint(
        text.c_str() + *index,
        allowable_chars,
        &line_size,
        remaining_extent,
        &allowable_chars_for_confirmation,
        size_buffer.get());
    dc.Detach();
    if (result == FALSE) {
      const int error = ::GetLastError();
      LOG(ERROR) << "GetTextExtentExPoint failed. error = " << error;
      return false;
    }
    line_window_width = line_size.cx;
    line_window_height = line_size.cy;
    if (allowable_chars != allowable_chars_for_confirmation) {
      LOG(ERROR)
          << "(allowable_chars, allowable_chars_for_confirmation) = ("
          << allowable_chars << ", "
          << allowable_chars_for_confirmation << ")";
      return false;
    }
    window_layout.text = text.substr(*index, allowable_chars);
    window_layout.log_font = log_font;

    AddWindowPositionToLayout(client_rect, *offset, line_window_width,
                              line_window_height, &window_layout);
    AddMarkerToLayout(line_window_width, line_window_height, &window_layout);
    AddTextAreaToLayout(line_window_width, line_window_height, &window_layout);

    // base position is omitted.
    POINT base_point = {0, 0};
    window_layout.base_position = base_point;

    AddCaretToLayout(*index, allowable_chars, caret, line_window_height,
                     size_buffer.get(), &window_layout);
  }

  // Shift offset to the next line position.
  offset->x = 0;
  offset->y += (line_window_height + 1);

  // Increase index to indicate text not displayed yet.
  *index += allowable_chars;

  *new_line = window_layout;
  return true;
}

void CompositionWindowLayouter::AddWindowPositionToLayout(
    const RECT& client_rect,
    const POINT& offset,
    int line_window_width,
    int line_window_height,
    CompositionWindowList::CompositionWindowLayout* window_layout) {
  RECT window_rect = {0};
  window_rect.left = client_rect.left + offset.x;
  window_rect.right = window_rect.left + line_window_width;
  window_rect.top = client_rect.top + offset.y;
  window_rect.bottom = window_rect.top + line_window_height;
  window_layout->window_position_in_screen_coordinate = window_rect;
}

void CompositionWindowLayouter::AddMarkerToLayout(
    int line_window_width, int line_window_height,
    CompositionWindowList::CompositionWindowLayout* window_layout) {
  CompositionWindowList::SegmentMarkerLayout marker;
  marker.from.x = 0;
  marker.from.y = line_window_height - 1;
  marker.to.x = line_window_width;
  marker.to.y = line_window_height - 1;
  marker.highlighted = false;
  window_layout->marker_layouts.push_back(marker);
}

void CompositionWindowLayouter::AddTextAreaToLayout(
    int line_window_width,
    int line_window_height,
    CompositionWindowList::CompositionWindowLayout* window_layout) {
  RECT text_rect = {0};
  text_rect.right = line_window_width;
  text_rect.bottom = line_window_height;
  window_layout->text_area = text_rect;
}

void CompositionWindowLayouter::AddCaretToLayout(
    int index, int allowable_chars, int caret,
    int line_window_height,
    int* width_array,
    CompositionWindowList::CompositionWindowLayout* window_layout) {
  if (index < caret && (index + allowable_chars) >= caret) {
    // The caret belongs to current line window.
    window_layout->caret_rect.left = width_array[caret - index - 1];
    window_layout->caret_rect.right = window_layout->caret_rect.left + 1;
    window_layout->caret_rect.top = 0;
    window_layout->caret_rect.bottom = line_window_height;
  } else {
    window_layout->caret_rect.left = 0;
    window_layout->caret_rect.right = 0;
    window_layout->caret_rect.top = 0;
    window_layout->caret_rect.bottom = 0;
  }
}

}  // namespace win_frontend
}  // namespace components
}  // namespace ime_goopy
