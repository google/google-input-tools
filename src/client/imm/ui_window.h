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

#ifndef GOOPY_IMM_UI_WINDOW_H__
#define GOOPY_IMM_UI_WINDOW_H__

#include "appsensorapi/appsensor.h"
#include "appsensorapi/appsensor_helper.h"
#include "base/basictypes.h"
#include "base/logging.h"
#include "base/scoped_ptr.h"
#include "common/debug.h"
#include "imm/context_manager.h"
#include "imm/debug.h"
#include "imm/immdev.h"

extern bool _goopy_exiting;

namespace ime_goopy {
class UIManagerInterface;
namespace imm {
// UIWindow communicates between IMM framework and user interface module. The
// user interface module is implemented by client via UIManagerInterface.
// When the UI need to update, IMM will send messages to UIWindow, and UIWindow
// translate those messages and call UIManagerInterface.
template <class ContextType>
class UIWindowT {
 public:
  // UIWindow will delete ui_manager in destructor.
  UIWindowT(HWND hwnd, UIManagerInterface* ui_manager)
      : hwnd_(hwnd),
        ui_manager_(ui_manager),
        context_(NULL) {
    assert(hwnd_);
  }

  ~UIWindowT() {
    ContextManagerT<ContextType>& context_manager =
        ContextManagerT<ContextType>::Instance();
    if (context_)
      context_manager.DisassociateUIManager(context_);
  }

  BEGIN_MSG_MAP(UIWindowT)
    MESSAGE_HANDLER(WM_IME_STARTCOMPOSITION, OnStartComposition)
    MESSAGE_HANDLER(WM_IME_COMPOSITION, OnComposition)
    MESSAGE_HANDLER(WM_IME_ENDCOMPOSITION, OnEndComposition)
    MESSAGE_HANDLER(WM_IME_NOTIFY, OnNotify)
    MESSAGE_HANDLER(WM_IME_SETCONTEXT, OnSetContext)
    MESSAGE_HANDLER(WM_IME_CONTROL, OnControl)
    MESSAGE_HANDLER(WM_IME_COMPOSITIONFULL, OnCompositionFull)
    MESSAGE_HANDLER(WM_IME_SELECT, OnSelect)
    MESSAGE_HANDLER(WM_IME_KEYDOWN, OnDummyHandler)
    MESSAGE_HANDLER(WM_IME_KEYUP, OnDummyHandler)
    MESSAGE_HANDLER(WM_IME_CHAR, OnDummyHandler)
  END_MSG_MAP()

  // The WM_IME_STARTCOMPOSITION message is sent immediately before an IME
  // generates a composition string as a result of a user's keystroke.
  // The UI window opens its composition window when it receives this message.
  LRESULT OnStartComposition(
      UINT message, WPARAM wparam, LPARAM lparam, BOOL& handled) {
    DVLOG(3) << __SHORT_FUNCTION__;

    if (context_) context_->SetShouldShow(
        ContextInterface::UI_COMPONENT_COMPOSITION, true);
    return 0;
  }

  // Sent two bytes of composition characters to the application. The
  // WM_IME_COMPOSITION message is sent when an IME changes composition status
  // as a result of a user's keystroke. The IME user interface window changes
  // its appearance when it processes this message.
  LRESULT OnComposition(
      UINT message, WPARAM wparam, LPARAM lparam, BOOL& handled) {
    DVLOG(3) << __SHORT_FUNCTION__
             << L" hwnd: " << hex << hwnd_
             << L" flag: " << ime_goopy::imm::Debug::GCS_String(lparam);
    if (context_ && context_->ShouldShow(
        ContextInterface::UI_COMPONENT_COMPOSITION)) {
      ui_manager_->Update(ContextInterface::UI_COMPONENT_COMPOSITION);
    }
    return 0;
  }

  // The WM_IME_ENDCOMPOSITION message is sent to an application when the IME
  // ends composition. An application that needs to display composition
  // characters by itself should not pass this message to either the application
  // IME user interface window or DefWindowProc, which passes it to the default
  // IME window.
  LRESULT OnEndComposition(
      UINT message, WPARAM wparam, LPARAM lparam, BOOL& handled) {
    DVLOG(1) << __SHORT_FUNCTION__
             << L" hwnd: " << hex << hwnd_;
    if (context_) {
      context_->SetShouldShow(ContextInterface::UI_COMPONENT_COMPOSITION,
                              false);
      ui_manager_->Update(ContextInterface::UI_COMPONENT_COMPOSITION);
    }
    return 0;
  }

  // The WM_IME_NOTIFY message notifies an application or the UI window of the
  // IME status. The wParam parameter is an IMN_ notification that indicates the
  // purpose of the message.
  LRESULT OnNotify(
      UINT message, WPARAM wparam, LPARAM lparam, BOOL& handled) {
    ContextManagerT<ContextType>& context_manager =
        ContextManagerT<ContextType>::Instance();
    ContextType* context = context_manager.GetFromWindow(hwnd_);
    DVLOG(1) << __SHORT_FUNCTION__
             << L" hwnd: " << hwnd_
             << L" context: " << context
             << L" flag: " << ime_goopy::imm::Debug::IMN_String(wparam)
             << L" lparam: " << lparam;
    if (!context) return 0;

    // See bug 4312191. Application "World of Warcraft" has its own candidate
    // window, so it should process this message and not send it to IME.
    // But it does not process this message. Here we use Appsensor to detect
    // "World of Warcraft" and neglect the IMN_OPENCANDIDATE message.
    if (AppSensorHelper::Instance()->HandleMessage(hwnd_, message, wparam,
                                                   lparam)) {
      return 0;
    }
    switch (wparam) {
      case IMN_CLOSESTATUSWINDOW:
        if (context_) {
          if (context_->GetOpenStatus()) {
            ui_manager_->SetToolbarStatus(false);
            ui_manager_->Update(COMPONENT_STATUS);
          }
        }
        break;
      case IMN_OPENSTATUSWINDOW:
        if (context_) {
          if (context_->GetOpenStatus() && context_->IsVisible()) {
            ui_manager_->SetToolbarStatus(true);
            ui_manager_->Update(COMPONENT_STATUS);
          }
        }
        break;
      case IMN_CHANGECANDIDATE:
        break;
      case IMN_CLOSECANDIDATE:
        if (context_) {
          context_->SetShouldShow(
            ContextInterface::UI_COMPONENT_CANDIDATES, false);
        }
        break;
      case IMN_OPENCANDIDATE:
        // The IMN_OPENCANDIDATE may also come from imm::ContextT::Update() as
        // a trick to fix Dreamweaver, even if we have IMN_SETCOMPOSITIONWINDOW
        // instead of IMN_SETCANDIDATEPOS. We do not allow it override the bit
        // if UI_COMPONENT_COMPOSITION is actually selected to show.
        //
        // Simply checking context_->updating() doesn't work here, because some
        // applications like firefox3 doesn't send IMN_SETCANDIDATEPOS before
        // update, so we will not be told which component to show when we are
        // updating the window, the composition window will lost in such case.
        if (context_ &&
            !context_->ShouldShow(ContextInterface::UI_COMPONENT_COMPOSITION)) {
          context_->SetShouldShow(
              ContextInterface::UI_COMPONENT_CANDIDATES, true);
        }
        break;
      case IMN_SETCONVERSIONMODE:
        context->OnSystemStatusChange();
        if (context->GetOpenStatus()) ui_manager_->Update(COMPONENT_STATUS);
        break;
      case IMN_SETSENTENCEMODE:
        context->OnSystemStatusChange();
        break;
      case IMN_SETOPENSTATUS:
        // Some applications (Opera for example), set open status to false when
        // the focus is changed to an empty context.
        if (context_) {
          SwitchContext(context_, context_);
        }
        break;
      case IMN_SETCANDIDATEPOS:
        // The bits in lparam represent the identity of candidate forms. The
        // first candidate form is used in many applications to store the
        // position info of the candidate window.
        if (lparam & 0x1) {
          if (context_ && context_->GetOpenStatus() && !context_->updating()) {
            // It is not likely that both will be true, but just in case.
            context_->SetShouldShow(Context::UI_COMPONENT_CANDIDATES, true);
            context_->SetShouldShow(Context::UI_COMPONENT_COMPOSITION, false);
            ui_manager_->LayoutChanged();
          }
        }
        break;
      case IMN_SETCOMPOSITIONFONT:
        break;
      case IMN_SETCOMPOSITIONWINDOW:
        if (context_ && context_->GetOpenStatus() && !context_->updating()) {
          // It is not likely that both will be true, but just in case.
          context_->SetShouldShow(Context::UI_COMPONENT_COMPOSITION, true);
          context_->SetShouldShow(Context::UI_COMPONENT_CANDIDATES, false);
          ui_manager_->LayoutChanged();
        }
        break;
      case IMN_SETSTATUSWINDOWPOS:
        break;
      case IMN_GUIDELINE:
        break;
      case IMN_PRIVATE:
        if (context_ && context_->GetOpenStatus()) ui_manager_->Update(lparam);
        if (context_) context_->FinishUpdate();
        break;
    }
    return 0;
  }

  // The WM_IME_SETCONTEXT message is sent to an application when a window of
  // the application is being activated. The UI window receives this message
  // after an application calls DefWindowProc or ImmIsUIMessage with
  // WM_IME_SETCONTEXT. The UI window should show the composition, guide, or
  // candidate window as indicated by lParam.
  LRESULT OnSetContext(
      UINT message, WPARAM wparam, LPARAM lparam, BOOL& handled) {
    ContextManagerT<ContextType>& context_manager =
        ContextManagerT<ContextType>::Instance();
    ContextType* context = context_manager.GetFromWindow(hwnd_);
    DVLOG(1) << __SHORT_FUNCTION__
             << L" hwnd: " << hwnd_
             << L" context: " << context
             << L" flag: " << ime_goopy::imm::Debug::ISC_String(lparam)
             << L" wparam: " << wparam;

    context_ = (wparam && context) ? context : NULL;
    if (context_) {
      if ((lparam & ISC_SHOWUICOMPOSITIONWINDOW) == 0) {
        context_->SetShouldShow(ContextInterface::UI_COMPONENT_COMPOSITION,
                                false);
      }
      if ((lparam & ISC_SHOWUIALLCANDIDATEWINDOW) == 0) {
        context_->SetShouldShow(ContextInterface::UI_COMPONENT_CANDIDATES,
                                false);
      }
    }
    SwitchContext(context, context_);
    return 0;
  }

  // The WM_IME_CONTROL message is a group of sub messages to control the IME
  // user interface. An application uses this message to interact with the IME
  // window created by the application. The IMC_ messages list the submessages
  // classified by the value of wParam.
  LRESULT OnControl(
      UINT message, WPARAM wparam, LPARAM lparam, BOOL& handled) {
    DVLOG(1) << __SHORT_FUNCTION__
             << L" hwnd: " << hex << hwnd_
             << L" flag: " << ime_goopy::imm::Debug::ISC_String(wparam);
    return 0;
  }

  // The WM_IME_COMPOSITIONFULL message is sent to an application when the IME
  // user interface window cannot increase the size of the composition window.
  // An application should specify how to display the IME UI window when it
  // receives this message. This message is a notification, which is sent to an
  // application by the IME user interface window and not by the IME itself.
  // The IME uses SendMEssage to send this notification.
  LRESULT OnCompositionFull(
      UINT message, WPARAM wparam, LPARAM lparam, BOOL& handled) {
    DVLOG(1) << __SHORT_FUNCTION__
             << L" hwnd: " << hex << hwnd_;
    return 0;
  }

  // The WM_IME_SELECT message is sent to the UI window when the system is about
  // to change the current IME. The system IME class uses this message to create
  // a new UI window and to destroy an old UI window for the application or the
  // system. DefWindowProc responds to this message by passing information to
  // the default IME window, which sends the message to its UI window.
  LRESULT OnSelect(
      UINT message, WPARAM wparam, LPARAM lparam, BOOL& handled) {
    ContextManagerT<ContextType>& context_manager =
      ContextManagerT<ContextType>::Instance();
    ContextType* context = context_manager.GetFromWindow(hwnd_);
    DVLOG(1) << __SHORT_FUNCTION__
             << L" hwnd: " << hwnd_
             << L" context: " << context
             << L" wparam: " << wparam;
    context_ = (wparam && context) ? context : NULL;
    SwitchContext(context, context_);
    return 0;
  }

  // This function is used to catch WM_IME_KEYDOWN, WM_IME_KEYUP and
  // WM_IME_CHAR. We don't process them but they must be eaten.
  LRESULT OnDummyHandler(
      UINT message, WPARAM wparam, LPARAM lparam, BOOL& handled) {
    DVLOG(1) << __SHORT_FUNCTION__
             << L" hwnd: " << hex << hwnd_;
    return 0;
  }

  static ATOM RegisterClass(const wchar_t class_name[]) {
    WNDCLASSEX wc;
    ZeroMemory(&wc, sizeof(wc));

    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_IME;
    wc.lpfnWndProc = WindowProc;
    wc.cbWndExtra = 2 * sizeof(LONG_PTR);
    wc.hInstance = _AtlBaseModule.m_hInst;
    wc.lpszClassName  = class_name;

    return RegisterClassEx(&wc);
  }

  // Windows message handler for UI window. This function handle the creation
  // and destroying UIWindow object.
  static LRESULT WINAPI WindowProc(HWND hwnd,
                                   UINT message,
                                   WPARAM wparam,
                                   LPARAM lparam) {
    // Create UI window object and associate it to the window.
    if (message == WM_CREATE) {
      UIManagerInterface* ui_manager = InputMethod::CreateUIManager(hwnd);
      // Return -1 to destroy the window.
      if (!ui_manager) return -1;
      // The ownership of ui_manager will be transferred to ui_window.
      UIWindow* ui_window = new UIWindow(hwnd, ui_manager);

      SetWindowLongPtr(hwnd,
                       IMMGWLP_PRIVATE,
                       reinterpret_cast<LONG_PTR>(ui_window));
    }

    // Call UI window object's message map.
    UIWindow* ui_window =
      reinterpret_cast<UIWindow*>(GetWindowLongPtr(hwnd, IMMGWLP_PRIVATE));
    // DefWindowProc will be called here only when our UIWindow instance is
    // not created for this window, we create the window as soon as we get a
    // WM_CREATE. The window proc in UIWindow will eat all IME messages.
    if (!ui_window) {
      if (!ImmIsUIMessage(hwnd, message, wparam, lparam))
        return DefWindowProc(hwnd, message, wparam, lparam);
      return 0;
    }

    LRESULT result = 0;
    if (!ui_window->ProcessWindowMessage(hwnd,
                                         message,
                                         wparam,
                                         lparam,
                                         result)) {
      result = DefWindowProc(hwnd, message, wparam, lparam);
    }

    // Delete UI window object if the window is destroyed.
    if (message == WM_NCDESTROY) {
      SetWindowLongPtr(hwnd, IMMGWLP_PRIVATE, NULL);
      // See bug 1698546, sometimes, IE6 will call this after
      // DllMain(DLL_PROCESS_DETACH). Many dlls are already unloaded at that
      // time, so "delete ui_window" will crash. Here we use a guard to make
      // sure that won't happen.
      if (!_goopy_exiting) delete ui_window;
    }
    return result;
  }

  friend class UIWindowTest;

 private:
  void SwitchContext(ContextType* previous_context,
                     ContextType* current_context) {
    ContextManagerT<ContextType>& context_manager =
        ContextManagerT<ContextType>::Instance();
    if (current_context && current_context->GetOpenStatus()) {
      ui_manager_->SetContext(current_context);
      context_manager.AssociateUIManager(current_context, ui_manager_.get());
    } else {
      ui_manager_->SetContext(NULL);
      context_manager.DisassociateUIManager(previous_context);
    }
  }

  HWND hwnd_;
  scoped_ptr<UIManagerInterface> ui_manager_;
  ContextType* context_;
};

typedef UIWindowT<Context> UIWindow;
}  // namespace imm
}  // namespace ime_goopy
#endif  // GOOPY_IMM_UI_WINDOW_H__
