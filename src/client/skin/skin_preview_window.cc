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

#include "skin/skin_preview_window.h"

#include <atlstr.h>
#include <gdiplus.h>

#include "base/logging.h"
#include "common/app_const.h"
#include "common/shellutils.h"
#include "common/string_utils.h"
#pragma push_macro("LOG")
#include "skin/skin.h"
#include "skin/skin_consts.h"
#include "skin/skin_host_win.h"
#include "skin/candidate_list_element.h"
#include "skin/composition_element.h"
#include "third_party/google_gadgets_for_linux/ggadget/gadget.h"
#include "third_party/google_gadgets_for_linux/ggadget/gadget_interface.h"
#include "third_party/google_gadgets_for_linux/ggadget/permissions.h"
#include "third_party/google_gadgets_for_linux/ggadget/view.h"
#include "third_party/google_gadgets_for_linux/ggadget/view_host_interface.h"
#include "third_party/google_gadgets_for_linux/ggadget/win32/gadget_window.h"
#pragma pop_macro("LOG")

using ggadget::ViewInterface;
using ggadget::ViewHostInterface;

namespace ime_goopy {
namespace skin {

#ifndef CS_DROPSHADOW
#define CS_DROPSHADOW 0x00020000
#endif

static const COLORREF kBorderColor = RGB(222, 222, 222);
static const int kDefaultWidth = 420;
static const int kDefaultHeight = 340;
static const int kDefaultXOffset = 3;
static const int kDefaultYOffset = 38;
static const int kViewPadding = 20;
static const RECT kDefaultRect = {kDefaultXOffset, kDefaultYOffset,
                                  kDefaultXOffset + kDefaultWidth,
                                  kDefaultYOffset + kDefaultHeight
};
static const wchar_t kCandidateTexts[][5] = {
    L"1.\u8c37\u6b4c",
    L"2.\u9aa8\u9abc",
    L"3.\u53e4\u683c",
    L"4.\u9aa8\u80f3",
    L"5.\u53e4\u6b4c"
};
static const wchar_t kCompositionText[] = L"guge";
static const int kAnimateTime = 200;  // The length of window animations.
static const int kAnimateInterval = 25;
static const int kLoadSkinDelay = 40;
static const double kFontScales[] = {0.75, 1, 1.25, 1.5};
static const double kMiniScale = 0.75;

enum TimerID {
  kReleaseCursorTimerID = 0x1001,  // A timer to release cursor clip.
  kAnimateShowTimerID,  // A timer to perform show animations.
  kAnimateHideTimerID,  // A timer to perform hide animations.
  kActionTimerID,  // A timer to start animations.
  kLoadSkinTimerID  // A timer that load the skin and show.
};

SkinPreviewWindow::SkinPreviewWindow()
    : owner_(NULL),
      skin_(NULL),
      skin_host_(new skin::SkinHostWin),
      window_rect_(kDefaultRect),
      is_dragging_(false),
      is_animating_(false),
      error_message_id_(0),
      current_candidate_direction_(false),
      next_candidate_direction_(false),
      current_font_size_(1),
      next_font_size_(1),
      current_mini_status_bar_(false),
      next_mini_status_bar_(false) {
}

SkinPreviewWindow::~SkinPreviewWindow() {
}

void SkinPreviewWindow::Show() {
  if (!IsWindowVisible()) {
    Reposition();
    Animate(true);
  } else {
    next_action_ = kActionNone;
    Invalidate(TRUE);
  }
}

void SkinPreviewWindow::Hide() {
  Animate(false);
}

bool SkinPreviewWindow::CreatePreviewWindow() {
  Create(NULL, CWindow::rcDefault, L"",
         0, WS_EX_TOOLWINDOW);
  ModifyStyle(WS_CAPTION, WS_CLIPCHILDREN | WS_POPUP, 0);
  if (ShellUtils::CheckWindowsXPOrLater()) {
    // Set window shadow, this style is only available in xp and later.
    DWORD style = GetClassLong(m_hWnd, GCL_STYLE);
    SetClassLong(m_hWnd, GCL_STYLE, style | CS_DROPSHADOW);
  }
  return IsWindow();
}

LRESULT SkinPreviewWindow::OnParentNotify(UINT uMsg,
                                          WPARAM wParam,
                                          LPARAM lParam,
                                          BOOL& bHandled) {
  WORD msg = LOWORD(wParam);
  if (msg == WM_LBUTTONDOWN) {
    RECT client_rect = {0};
    GetClientRect(&client_rect);
    ClientToScreen(&client_rect);
    ::ClipCursor(&client_rect);
    SetTimer(kReleaseCursorTimerID, 10, NULL);
    is_dragging_ = true;
  }
  return 0;
}

LRESULT SkinPreviewWindow::OnTimer(UINT uMsg,
                                   WPARAM wParam,
                                   LPARAM lParam,
                                   BOOL& bHandled) {
  RECT current_rect = {0};
  RECT owner_rect = {0};
  int increasement = (window_rect_.right - window_rect_.left) /
                     (kAnimateTime / kAnimateInterval);
  switch (wParam) {
    case kReleaseCursorTimerID:
      if (GetKeyState(VK_LBUTTON) >= 0) {
        ::ClipCursor(NULL);
        KillTimer(kReleaseCursorTimerID);
        is_dragging_ = false;
      }
      break;
    case kAnimateShowTimerID:
      GetWindowRect(&current_rect);
      ::GetWindowRect(owner_, &owner_rect);
      ::OffsetRect(&current_rect, increasement, 0);
      if (current_rect.left >= owner_rect.right + window_rect_.left) {
        KillTimer(kAnimateShowTimerID);
        is_animating_ = false;
        Reposition();
        Invalidate();
      } else {
        this->SetWindowPos(
            owner_, &current_rect,
            SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOSENDCHANGING);
      }
      break;
    case kAnimateHideTimerID:
      GetWindowRect(&current_rect);
      ::GetWindowRect(owner_, &owner_rect);
      ::OffsetRect(&current_rect, -increasement, 0);
      if (current_rect.right <= owner_rect.right + window_rect_.left) {
        KillTimer(kAnimateHideTimerID);
        is_animating_ = false;
        ShowWindow(SW_HIDE);
      } else {
        this->SetWindowPos(
            owner_, &current_rect,
            SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOSENDCHANGING);
      }
      break;
    case kActionTimerID:
      switch (next_action_) {
        case kActionAnimateShow:
          if (!is_animating_) {
            // Set the origin state of the window and start the show animation
            // timer.
            ::GetWindowRect(owner_, &owner_rect);
            current_rect = window_rect_;
            ::OffsetRect(&current_rect, owner_rect.right, owner_rect.top);
            ::OffsetRect(&current_rect, current_rect.left - current_rect.right,
                         0);
            is_animating_ = true;
            this->SetWindowPos(
                owner_, &current_rect,
                SWP_NOACTIVATE | SWP_NOSIZE | SWP_SHOWWINDOW |
                SWP_NOSENDCHANGING);
            SetTimer(kAnimateShowTimerID, 10, NULL);
          }
          break;
        case kActionAnimateHide:
          if (!is_animating_) {
            SetTimer(kAnimateHideTimerID, 10, NULL);
            is_animating_ = true;
          }
          break;
        case kActionNone:
          break;
      }
      KillTimer(kActionTimerID);
      break;
    case kLoadSkinTimerID:
      KillTimer(kLoadSkinTimerID);
      LoadSkin();
      break;
    default:
      DCHECK(false);
  }
  return 0;
}

LRESULT SkinPreviewWindow::OnWindowPosChanging(UINT uMsg,
                                               WPARAM wParam,
                                               LPARAM lParam,
                                               BOOL& bHandled) {
  WINDOWPOS* window_pos = reinterpret_cast<WINDOWPOS*>(lParam);
  window_pos->flags |= (SWP_NOMOVE | SWP_NOSIZE);
  return 0;
}

void SkinPreviewWindow::AddChild(HWND child) {
  ::SetWindowLong(
      child, GWL_STYLE,
      ::GetWindowLong(child, GWL_STYLE) & (~WS_POPUP) | WS_CHILD);
  ::SetParent(child, m_hWnd);
}

void SkinPreviewWindow::Reposition() {
  if (!is_animating_ && IsWindow() && owner_) {
    RECT owner_rect = {0};
    ::GetWindowRect(owner_, &owner_rect);
    RECT preview_rect = window_rect_;
    ::OffsetRect(&preview_rect, owner_rect.right, owner_rect.top);
    // Always put this window under its owner.
    SetWindowPos(owner_, &preview_rect, SWP_NOACTIVATE | SWP_NOSENDCHANGING);
  }
}

void SkinPreviewWindow::SetOwner(HWND owner) {
  owner_ = owner;
}

// Active the owner window when this window is activated.
LRESULT SkinPreviewWindow::OnActivate(UINT uMsg,
                                      WPARAM wParam,
                                      LPARAM lParam,
                                      BOOL& bHandled) {
  if (wParam != WA_INACTIVE) {
    ::SetWindowPos(owner_, m_hWnd, 0, 0, 0, 0,
                   SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE);
    ::SetActiveWindow(owner_);
  }
  return 0;
}

LRESULT SkinPreviewWindow::OnPaint(UINT uMsg,
                                   WPARAM wParam,
                                   LPARAM lParam,
                                   BOOL& bHandled) {
  PAINTSTRUCT ps = {0};
  HDC dc = BeginPaint(&ps);
  // Draw a border is the window has no caption.
  if ((GetWindowLong(GWL_STYLE) & WS_CAPTION) == 0) {
    HPEN pen = ::CreatePen(PS_SOLID, 1, kBorderColor);
    HGDIOBJ origin_pen = ::SelectObject(dc, pen);
    RECT client_rect;
    GetClientRect(&client_rect);
    ::MoveToEx(dc, client_rect.left, client_rect.top, NULL);
    ::LineTo(dc, client_rect.right - 1, client_rect.top);
    ::LineTo(dc, client_rect.right - 1, client_rect.bottom - 1);
    ::LineTo(dc, client_rect.left, client_rect.bottom - 1);
    ::LineTo(dc, client_rect.left, client_rect.top);
    ::SelectObject(dc, origin_pen);
    ::DeleteObject(pen);
  }
  if (!error_message_.empty()) {
    Gdiplus::Graphics graphics(dc);
    RECT client_rect;
    GetClientRect(&client_rect);
    Gdiplus::Font font(L"Arial", 12);
    Gdiplus::RectF layout_rect(client_rect.left, client_rect.top,
                               client_rect.right - client_rect.left,
                               client_rect.bottom - client_rect.top);
    Gdiplus::StringFormat string_format;
    string_format.SetAlignment(Gdiplus::StringAlignmentCenter);
    string_format.SetLineAlignment(Gdiplus::StringAlignmentCenter);
    graphics.DrawString(error_message_.c_str(), error_message_.length(), &font,
                        layout_rect, &string_format,
                        &Gdiplus::SolidBrush(Gdiplus::Color::Black));
  }
  EndPaint(&ps);
  return 1;
}

LRESULT SkinPreviewWindow::OnMouseActivate(UINT uMsg,
                                           WPARAM wParam,
                                           LPARAM lParam,
                                           BOOL& bHandled) {
  return MA_NOACTIVATE;
}

void SkinPreviewWindow::Animate(bool is_show) {
  next_action_ = is_show ? kActionAnimateShow : kActionAnimateHide;
  SetTimer(kActionTimerID, kAnimateInterval, NULL);
}

void SkinPreviewWindow::LoadSkin() {
  if (current_skin_path_ == next_skin_path_ &&
      current_candidate_direction_ == next_candidate_direction_ &&
      current_mini_status_bar_ == next_mini_status_bar_ &&
      current_font_size_ == next_font_size_) {
    return;
  }
  current_skin_path_ = next_skin_path_;
  current_candidate_direction_ = next_candidate_direction_;
  current_font_size_ = next_font_size_;
  current_mini_status_bar_ = next_mini_status_bar_;
  next_skin_path_.clear();
  if (!skin::Skin::ValidateSkinPackage(WideToUtf8(current_skin_path_).c_str(),
                                       kSkinLocaleName)) {
    CString error_string;
    error_string.LoadString(_AtlBaseModule.GetResourceInstance(),
                            error_message_id_);
    error_message_ = error_string + current_skin_path_.c_str();
    skin_.reset(NULL);
    Invalidate(TRUE);
    return;
  }
  error_message_.clear();
  skin_.reset(skin_host_->LoadSkin(current_skin_path_.c_str(), L"Option",
                                   kSkinLocaleName, 0, true));
  if (!skin_.get() && !skin_->IsValid()) {
    DCHECK(skin_->IsValid());
    return;
  }
  Skin::ViewType composing_view_type = current_candidate_direction_ ?
      Skin::HORIZONTAL_COMPOSING_VIEW : Skin::VERTICAL_COMPOSING_VIEW;
  ViewHostInterface* status_view_host = skin_->GetMainView()->GetViewHost();
  ViewHostInterface* composing_view_host = 
      skin_->GetView(composing_view_type)->GetViewHost();
  AddChild(reinterpret_cast<HWND>(
      status_view_host->GetNativeWidget()));
  AddChild(reinterpret_cast<HWND>(
      composing_view_host->GetNativeWidget()));

  // Set Font Size and status bar size.
  status_view_host->SetZoom(current_mini_status_bar_ ? kMiniScale : 1);
  DCHECK_GE(current_font_size_, 0);
  DCHECK_LT(current_font_size_, arraysize(kFontScales));
  if (current_font_size_ < 0 || current_font_size_ >= arraysize(kFontScales))
    current_font_size_ = 1;
  composing_view_host->SetFontScale(kFontScales[current_font_size_]);
  // Add candidates and composition text.
  CandidateListElement* candidate_list =
      skin_->GetElementByNameAndType<CandidateListElement>(
          composing_view_type, kCandidateListElement);
  for (int i = 0; i < arraysize(kCandidateTexts); ++i)
    candidate_list->AppendCandidate(i, WideToUtf8(kCandidateTexts[i]));
  candidate_list->SetSelectedCandidate(0);

  CompositionElement* composition =
      skin_->GetElementByNameAndType<CompositionElement>(
          composing_view_type, kCompositionElement);
  composition->AppendClause(WideToUtf8(kCompositionText),
                            CompositionElement::ACTIVE);
  composition->SetCaretPosition(1);
  composition->UpdateUI();

  // Calculate view positions.
  int status_width = 0;
  int status_height = 0;
  int composing_width = 0;
  int composing_height = 0;
  status_view_host->GetWindowSize(&status_width, &status_height);
  composing_view_host->GetWindowSize(&composing_width, &composing_height);
  int preview_width = window_rect_.right - window_rect_.left;
  int preview_height = window_rect_.bottom - window_rect_.top;
  int composing_x = (preview_width - composing_width) / 2;
  int composing_y =
      (preview_height - status_height - composing_height - kViewPadding) / 2;
  int status_x = (preview_width - status_width) / 2;
  int status_y = composing_y + composing_height + kViewPadding;
  status_view_host->SetWindowPosition(status_x, status_y);
  composing_view_host->SetWindowPosition(composing_x, composing_y);
  status_view_host->ShowView(false, 0, NULL);
  composing_view_host->ShowView(false, 0, NULL);
  Invalidate(TRUE);
}

void SkinPreviewWindow::PreviewSkin(const wchar_t* skin_path,
                                    bool horizontal_candidate_list,
                                    int font_size,
                                    bool mini_status_bar) {
  next_skin_path_ = skin_path;
  next_candidate_direction_ = horizontal_candidate_list;
  next_font_size_ = font_size;
  next_mini_status_bar_ = mini_status_bar;
  SetTimer(kLoadSkinTimerID, kLoadSkinDelay);
}

void SkinPreviewWindow::SetWindowRect(RECT window_rect) {
  window_rect_ = window_rect;
}

bool SkinPreviewWindow::IsEmpty() {
  return skin_.get() == NULL;
}

LRESULT SkinPreviewWindow::OnDestory(UINT uMsg,
                                     WPARAM wParam,
                                     LPARAM lParam,
                                     BOOL& bHandled) {
  skin_.reset(NULL);
  return 0;
}

void SkinPreviewWindow::SetErrorMessage(int message_id) {
  error_message_id_ = message_id;
}

}  // namespace skin
}  // namespace ime_goopy
