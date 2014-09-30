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

#include "components/win_frontend/ipc_ui_manager.h"
#include "base/logging.h"

namespace ime_goopy {
namespace components {

// Implementation of class IPCUIManager
class IPCUIManager::Impl {
 public:
  explicit Impl(IPCUIManager* owner);
  ~Impl();

  void UpdateCompositionWindow();
  void UpdateCandidateWindow();
  void UpdateStatusWindow();
  void UpdateInputCaret();
  // Last show state of both compositon/status window.
  bool composition_show_;
  bool candidates_show_;
  bool status_show_;
  RECT caret_rect_;
  ContextInterface* context_;
  IPCUIManager* owner_;
};

IPCUIManager::Impl::Impl(IPCUIManager* owner)
    : composition_show_(false),
      candidates_show_(false),
      status_show_(false),
      context_(NULL),
      owner_(owner) {
}

IPCUIManager::Impl::~Impl() {
}

void IPCUIManager::Impl::UpdateCompositionWindow() {
  if (!context_)
    return;
  composition_show_ =
      context_->ShouldShow(ContextInterface::UI_COMPONENT_COMPOSITION);
  EngineInterface* frontend = context_->GetEngine();
  if (frontend)
    frontend->EnableCompositionWindow(composition_show_);
}

void IPCUIManager::Impl::UpdateCandidateWindow() {
  if (!context_)
    return;

  EngineInterface* frontend = context_->GetEngine();
  if (!frontend)
    return;

  // Always show candidate window if showing composition window is permitted by
  // application.
  candidates_show_ =
      context_->ShouldShow(ContextInterface::UI_COMPONENT_CANDIDATES);
  composition_show_ =
      context_->ShouldShow(ContextInterface::UI_COMPONENT_COMPOSITION);
  if (candidates_show_ || composition_show_)
    UpdateInputCaret();
  // Always call this interface to inform UI component even the value is not
  // changed because UI may forget it after input context lost focus.
  frontend->EnableCandidateWindow(candidates_show_ || composition_show_);
}

void IPCUIManager::Impl::UpdateStatusWindow() {
  if (!context_)
    return;

  EngineInterface* frontend = context_->GetEngine();
  if (!frontend)
    return;

  frontend->EnableToolbarWindow(status_show_);
}

void IPCUIManager::Impl::UpdateInputCaret() {
  if (!context_)
    return;
  EngineInterface* frontend = context_->GetEngine();
  if (!frontend)
    return;
  frontend->UpdateInputCaret();
}

IPCUIManager::IPCUIManager()
    : ALLOW_THIS_IN_INITIALIZER_LIST(impl_(new Impl(this))) {
}

IPCUIManager::~IPCUIManager() {
}

void IPCUIManager::SetContext(ContextInterface* context) {
  if (impl_->context_ == context)
    return;
  if (impl_->context_ != NULL) {
    EngineInterface* frontend = impl_->context_->GetEngine();
    if (!frontend)
      return;
    frontend->BlurInputContext();
  }

  impl_->context_ = context;
  if (impl_->context_!= NULL) {
    EngineInterface* frontend = impl_->context_->GetEngine();
    if (!frontend)
      return;
    frontend->FocusInputContext();
    // Update UI component display status.
    impl_->UpdateCandidateWindow();
    impl_->UpdateStatusWindow();
  }
}

void IPCUIManager::SetToolbarStatus(bool is_open) {
  impl_->status_show_ = is_open;
  impl_->UpdateStatusWindow();
}

void IPCUIManager::Update(uint32 component) {
  if (component & COMPONENT_COMPOSITION) {
    impl_->UpdateCompositionWindow();
  } else if (component & COMPONENT_CANDIDATES) {
    impl_->UpdateCandidateWindow();
  } else if (component & COMPONENT_STATUS) {
    impl_->UpdateStatusWindow();
  }
}

void IPCUIManager::LayoutChanged() {
  impl_->UpdateInputCaret();
}

}  // namespace components
}  // namespace ime_goopy
