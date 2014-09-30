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

#ifndef GOOPY_COMPONENTS_WIN_FRONTEND_FRONTEND_COMPONENT_H_
#define GOOPY_COMPONENTS_WIN_FRONTEND_FRONTEND_COMPONENT_H_

#include <map>
#include <queue>
#include <string>
#include <vector>

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "base/logging.h"
#include "base/scoped_ptr.h"
#include "common/framework_interface.h"
#include "ipc/component_base.h"
#include "ipc/protos/ipc.pb.h"
#include "ipc/settings_client.h"

namespace ime_goopy {
namespace components {
class LoggingClient;
namespace win_frontend {
class CompositionWindowList;
}  // namespace win_frontend

// FrontendComponent provides EngineInterface for IMM/TSF module and
// convert EngineInterface calls into IPC messages, then translate IPC messages
// received into IMM/TSF calls.
class FrontendComponent : public EngineInterface,
                          public ipc::ComponentBase,
                          public ipc::SettingsClient::Delegate {
 public:
  class Delegate {
   public:
    virtual ~Delegate() {}
    // Notifies that an input context |icid| is created in current process.
    virtual void InputContextCreated(int icid) = 0;
  };
  explicit FrontendComponent(Delegate* delegate);
  virtual ~FrontendComponent();

  // Overridden from ipc::SettingsClient::Delegate:
  virtual void OnValueChanged(const std::string& key,
                              const ipc::proto::VariableArray& array) OVERRIDE;
  // Overridden from ipc::Component:
  virtual void GetInfo(ipc::proto::ComponentInfo* info) OVERRIDE;
  virtual void OnRegistered() OVERRIDE;
  virtual void OnDeregistered() OVERRIDE;
  virtual void Handle(ipc::proto::Message* message) OVERRIDE;

  // Overridden from EngineInterface:
  virtual bool ShouldProcessKey(const ipc::proto::KeyEvent& key) OVERRIDE;
  virtual void ProcessKey(const ipc::proto::KeyEvent& key) OVERRIDE;
  virtual void SelectCandidate(int index, bool commit) OVERRIDE;
  virtual void EndComposition(bool commit) OVERRIDE;
  virtual void FocusInputContext() OVERRIDE;
  virtual void BlurInputContext() OVERRIDE;
  virtual void EnableCompositionWindow(bool enable) OVERRIDE;
  virtual void EnableCandidateWindow(bool enable) OVERRIDE;
  virtual void EnableToolbarWindow(bool enable) OVERRIDE;
  virtual void UpdateInputCaret() OVERRIDE;
  virtual void NotifyConversionModeChange(uint32 conversion_mode) OVERRIDE;
  virtual uint32 GetConversionMode() OVERRIDE;
  virtual void ResizeCandidatePage(int new_size) OVERRIDE;
  virtual int GetTextStyleIndex(TextState text_state) OVERRIDE;
  // TODO(haicsun): integrate import dictionary when integrating goopy.
  virtual bool ImportDictionary(const wchar_t* file_name) OVERRIDE {
    return true;
  }
  virtual void SetContext(ContextInterface* context) OVERRIDE;

 private:
  // Creates input context in IPC layer.
  void IPCCreateInputContext();
  bool IPCSwitchToInputMethod();
  // Requests message consumer in IPC input context.
  void IPCRequestConsumer();

  // Dispatches message to the proper methods to handle.
  void DoHandle(ipc::proto::Message* message);
  // IPC message handlers.
  void OnMsgActiveConsumerChanged(ipc::proto::Message* message);
  void OnMsgInputContextLostFocus(ipc::proto::Message* message);
  void OnMsgInputContextGotFocus(ipc::proto::Message* message);
  void OnMsgInputContextDeleted(ipc::proto::Message* message);
  void OnMsgInputMethodActivated(ipc::proto::Message* message);
  void OnMsgCompositionChanged(ipc::proto::Message* message);
  void OnMsgCandidateListChanged(ipc::proto::Message* message);
  void OnMsgInsertText(ipc::proto::Message* message);
  void OnMsgConversionModeChanged(ipc::proto::Message* message);
  void OnMsgSynthesizeKeyEvent(ipc::proto::Message* message);
  void OnMsgEnableFakeInlineComposition(ipc::proto::Message* message);

  // Gets value from settings store, if failed, set int_value to default value.
  void GetIntegerValue(std::string key, int32* int_value);

  void AssembleComposition();
  bool ShouldAssembleComposition() const;
  void OnIPCDisconnected();
  void CacheMessage(ipc::proto::Message* message);
  void SwitchInputMethod();

  // Returns false if fails.
  // Updates the layout of composition window and fills in the coordination for
  // the bottom-most window.
  bool UpdateInlineCompositionWindow(RECT* last_composition_line_rect);

  void UpdateComposition();

  // Variables.
  ContextInterface* context_;
  const std::wstring kEmptyString;

  ipc::proto::CandidateList candidates_;
  ipc::proto::Composition raw_composition_;
  std::wstring composition_;
  // Caret position in characters.
  int caret_;
  std::wstring composition_in_window_;
  int caret_in_window_;
  bool was_backspace_;

  // Legacy variable from engine.cc, used to prevent EndComposition from
  // being re-entrant.
  // TODO(haicsun): Do some survey, if EndComposition never be called from
  // multiple threads, remove it.
  bool composition_terminating_;
  std::vector<std::wstring> converters_;
  std::wstring help_tips_;

  // IPC input context id.
  uint32 icid_;
  // If the language of input method in current icid is a rtl language.
  bool is_rtl_language_;
  // True if ime has been loaded to input context.
  bool ime_loaded_;
  // True if ui component has been loaded to input context.
  bool ui_loaded_;
  // Used to combine |ShouldProcessKey| and |ProcessKey|. at first
  // |ShouldProcessKey| is called, and IPC message will be sent and received by
  // ime component, then an reply indicating if the key should be processed will
  // be sent back. if it's true, ime also process the key and send the result
  // back. from fronend's perspect, after receiving true reply of
  // |ShouldProcessKey|, it begins to listen to ime components and caches all
  // messages received until IMM call |ProcessKey|, then it handle all the
  // cached message.
  //
  // True if |ShouldProcessKey| return true and component is waiting for
  // |ProcessKey| call.
  bool waiting_process_key_;
  std::queue<ipc::proto::Message*> cached_messages_;
  // Cached messages sent before ime is loaded.
  std::queue<ipc::proto::Message*> cached_produced_messages_;

  // CJK only information
  // Those values are orginally suggested by IME, and then send to system(IMM)
  // to determine its final value. if system disagree, new value will be set,
  // and IME should follow the new values.
  DWORD conversion_mode_;

  ipc::SettingsClient* settings_client_;
  // Mapping from string key to interger value cached from settings store.
  std::map<std::string, int32> settings_integers_;

  // The process id of ipc console.
  DWORD console_pid_;

  Delegate* delegate_;
  // Current input method's information.
  ipc::proto::ComponentInfo current_input_method_;

  // True if the user is switching input method using the toolbar.
  bool switching_input_method_by_toolbar_;

  // A list of composition line window used to show inline composition text.
  scoped_ptr<win_frontend::CompositionWindowList> composition_window_list_;

  bool enable_fake_inline_composition_;
  DISALLOW_COPY_AND_ASSIGN(FrontendComponent);
};

}  // namespace components
}  // namespace ime_goopy

#endif  // GOOPY_COMPONENTS_WIN_FRONTEND_FRONTEND_COMPONENT_H_
