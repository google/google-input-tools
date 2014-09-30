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

#ifndef GOOPY_SKIN_SKIN_PREVIEW_WINDOW_H_
#define GOOPY_SKIN_SKIN_PREVIEW_WINDOW_H_

#include <atlbase.h>
#include <atlwin.h>

#include <vector>

#include "base/basictypes.h"
#include "base/scoped_ptr.h"

namespace ggadget {
namespace win32 {
class GadgetWindow;
}  // namespace win32
}  // namespace ggadget

namespace ime_goopy {
namespace skin {

class Skin;
class SkinHostWin;

// Defines a window class that can load a skin and display the views in skin as
// its child windows.
class SkinPreviewWindow
    : public CWindowImpl<SkinPreviewWindow, CWindow, CNullTraits> {
 public:
  SkinPreviewWindow();
  ~SkinPreviewWindow();
  // Set the skin to preview, the skin will be shown if this window is visible.
  // If this function is called multiple times in a short period, only the last
  // skin will be loaded.
  void PreviewSkin(const wchar_t* skin_path,
                   bool horizontal_candidate_list,
                   int font_size,
                   bool mini_status_bar);
  // Create this window.
  bool CreatePreviewWindow();
  // Reposition the preview window according to the position of its owner.
  void Reposition();
  void SetOwner(HWND owner);
  void Hide();
  void Show();
  // Sets the bounding rectangle of the window. Notice that the origin of the
  // param |window_rect|'s coordinates is on the top-right corner of the owner
  // window.
  void SetWindowRect(RECT window_rect);
  // Returns true if there's no skin shown in this window.
  bool IsEmpty();
  void SetErrorMessage(int message_id);
  LRESULT OnActivate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnMouseActivate(UINT uMsg,
                          WPARAM wParam,
                          LPARAM lParam,
                          BOOL& bHandled);
  LRESULT OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnParentNotify(UINT uMsg,
                         WPARAM wParam,
                         LPARAM lParam,
                         BOOL& bHandled);
  LRESULT OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnWindowPosChanging(UINT uMsg,
                              WPARAM wParam,
                              LPARAM lParam,
                              BOOL& bHandled);
  LRESULT OnDestory(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  BEGIN_MSG_MAP(SkinPreviewWindow)
    MESSAGE_HANDLER(WM_ACTIVATE, OnActivate);
    MESSAGE_HANDLER(WM_MOUSEACTIVATE, OnMouseActivate);
    MESSAGE_HANDLER(WM_PAINT, OnPaint);
    MESSAGE_HANDLER(WM_PARENTNOTIFY, OnParentNotify);
    MESSAGE_HANDLER(WM_TIMER, OnTimer);
    MESSAGE_HANDLER(WM_WINDOWPOSCHANGING, OnWindowPosChanging);
    MESSAGE_HANDLER(WM_DESTROY, OnDestory);
  END_MSG_MAP();

 private:
  enum Action {
    kActionAnimateShow,  // Performs show animation.
    kActionAnimateHide,  // Performs hide animation.
    kActionNone  // No animation.
  };
  // Adds a child window;
  void AddChild(HWND child);
  // Performs window animation. Set the param |is_show| to true to show the
  // window, false to hide the window.
  void Animate(bool is_show);
  void LoadSkin();
  HWND owner_;
  scoped_ptr<skin::Skin> skin_;
  scoped_ptr<skin::SkinHostWin> skin_host_;
  bool is_dragging_;
  bool is_animating_;
  Action next_action_;
  RECT window_rect_;
  std::wstring current_skin_path_;
  std::wstring next_skin_path_;
  bool current_candidate_direction_;
  bool next_candidate_direction_;
  int error_message_id_;
  int current_font_size_;
  int next_font_size_;
  bool current_mini_status_bar_;
  bool next_mini_status_bar_;
  std::wstring error_message_;
  DISALLOW_COPY_AND_ASSIGN(SkinPreviewWindow);
};

}  // namespace skin
}  // namespace ime_goopy

#endif  // GOOPY_SKIN_SKIN_PREVIEW_WINDOW_H_
