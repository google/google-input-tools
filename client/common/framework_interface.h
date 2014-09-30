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

#ifndef GOOPY_COMMON_FRAMEWORK_INTERFACE_H__
#define GOOPY_COMMON_FRAMEWORK_INTERFACE_H__

#include <string>
#include "base/basictypes.h"
#include "ipc/protos/ipc.pb.h"

namespace ime_goopy {
class CandidateInterface;
class CandidateItem;
class CandidatePage;
class CommandInterface;
class CompositionInterface;
class EngineInterface;
class KeyStroke;

enum UpdateComponent {
  COMPONENT_COMPOSITION_STRING = 0x01,
  COMPONENT_COMPOSITION_CARET = 0x02,
  COMPONENT_COMPOSITION = 0x03,

  COMPONENT_CANDIDATES_STRING = 0x04,
  COMPONENT_CANDIDATES_CURRENT_INDEX = 0x08,
  COMPONENT_CANDIDATES = 0x0C,

  // Status bar showing states and menu.
  COMPONENT_STATUS = 0x10,

  COMPONENT_SOFT_KEYBOARD = 0x20,

  // Info window to show the detail related to candidates.
  COMPONENT_INFO = 0x40,

  COMPONENT_USER_START = 0x100,
};

enum TextState {
  TEXTSTATE_COMPOSING,
  TEXTSTATE_HOVER,
  TEXTSTATE_COUNT,
};

// Each TextState can be assigned a different TextStyle.
struct TextStyle {
  enum LineStyle {
    LINESTYLE_NONE,
    LINESTYLE_SOLID,
    LINESTYLE_DOT,
    LINESTYLE_DASH,
    LINESTYLE_SQUIGGLE,
    LINESTYLE_COUNT
  };
  enum ColorFieldMask {
    FIELD_NONE = 0,
    FIELD_TEXT_COLOR = 1,
    FIELD_BACKGROUND_COLOR = 2,
    FIELD_LINE_COLOR = 4
  };
  GUID guid;
  int color_field_mask;
  COLORREF text_color;
  COLORREF background_color;
  COLORREF line_color;
  LineStyle line_style;
  bool bold_line;
};

// Text range objects passed between the engine and the context.
class TextRangeInterface {
 public:
  virtual ~TextRangeInterface() {}
  // Gets the text in the range.
  virtual wstring GetText() = 0;
  // Shifts the start of the range. actual_offset can be NULL.
  virtual void ShiftStart(int offset, int *actual_offset) = 0;
  // Shifts the end of the range. actual_offset can be NULL.
  virtual void ShiftEnd(int offset, int *actual_offset) = 0;
  // Collapse the end to the start.
  virtual void CollapseToStart() = 0;
  // Collapse the start to the end.
  virtual void CollapseToEnd() = 0;
  // Is the range empty?
  virtual bool IsEmpty() = 0;
  // Contains another range?
  virtual bool IsContaining(TextRangeInterface *inner_range) = 0;
  // Reconverts the range into the composition.
  // TODO(zengjian): This is a read-write request. In some
  // non-keyboard-event-processing cycles, this could only be processed
  // asynchronously.
  virtual void Reconvert() = 0;
  // Makes a copy of the range.
  virtual TextRangeInterface *Clone() = 0;
};

// Implemented by the framework, like IMM or TSF.
class ContextInterface {
 public:
  enum Platform {
    PLATFORM_WINDOWS_IMM,
    PLATFORM_WINDOWS_TSF,
  };

  enum UIComponent {
    UI_COMPONENT_STATUS,
    UI_COMPONENT_COMPOSITION,
    UI_COMPONENT_CANDIDATES,
    UI_COMPONENT_COUNT,
  };

#if defined(OS_WIN)
  typedef HWND ContextId;
  static bool IsValidContextID(ContextId id) {
    return id && ::IsWindow(id);
  }
#else
#error don't support other platform than windows.
#endif  // OS_WIN

  virtual ~ContextInterface() {}

  // Actions.
  //
  // Called when composition string changed.
  // |composition| : new composition text.
  // |caret| : new caret position in characters.
  virtual void UpdateComposition(const std::wstring& composition,
                                 int caret) {}

  // Called when IME commits the composition.
  // |result| : text to be commited to application.
  virtual void CommitResult(const std::wstring& result) {}

  // Called when candidate list changed.
  // |is_compositing| : true if in a composition process.
  // |candidate_list| : new candidate list.
  virtual void UpdateCandidates(
      bool is_compositing, const ipc::proto::CandidateList& candiate_list) {}

  // Called when IME changed the conversion status.
  // |native| : native language(not alpha mode).
  // |full_shape| : true if full shape character.
  // |full_punct| : punctuation symbols.
  virtual void UpdateStatus(bool native, bool full_shape, bool full_punct) {}

  // Information.
  virtual Platform GetPlatform() = 0;
  virtual EngineInterface* GetEngine() = 0;

  // Returns true if client rect of input context window is set in rect.
  virtual bool GetClientRect(RECT *client_rect) { return false; }
  // Returns caret rect of current input caret, used as a hint to place
  // candiadte window.
  virtual bool GetCaretRectForCandidate(RECT* rect) { return false; }
  // Returns caret rect where composition starts, used as a hint to show
  // composition window.
  virtual bool GetCaretRectForComposition(RECT* rect) { return false; }

  virtual bool GetCandidatePos(POINT *point) { return false; }
  virtual bool GetCompositionPos(POINT *point) { return false; }
  virtual bool GetCompositionBoundary(RECT* rect) { return false; }
  virtual bool GetCompositionFont(LOGFONT* font) { return false; }

  // TODO(zengjian): Make it pure virtual after integration from english to
  // main.
  // The invoker should own this pointer by scoped_ptr.
  virtual TextRangeInterface *GetSelectionRange() { return NULL; }
  virtual TextRangeInterface *GetCompositionRange() { return NULL; }
  // Should show certain UI components? In some games there are external UI
  // for candidates, and in that case the candidate window should be hidden.
  virtual bool ShouldShow(UIComponent ui_type) = 0;
  // Returns a context unique id.
  virtual ContextId GetId() { return NULL; }
  // Detach working input method, used when input method is switching.
  virtual void DetachEngine() {}
};

// Implemented by input method.
class EngineInterface {
 public:
  enum Status {
    // Status like full shape, traditional Chinese mode, etc.
    STATUS_CONVERSION,
    // Unused. Status like backgram.
    STATUS_SENTENCE,
    // Status if the info window is opened.
    STATUS_SHOULD_SHOW_INFO,
  };

  enum ConversionMode {
    CONVERSION_MODE_CHINESE = 0x1,
    CONVERSION_MODE_FULL_SHAPE = 0x2,
    CONVERSION_MODE_FULL_PUNCT = 0x4,
  };

  enum ChangeFlags {
    CHANGE_CONTENTS,
    CHANGE_SELECTION
  };

  virtual ~EngineInterface() {}

  // Actions.

  // Inquires if a key would be accepted by the engine.
  virtual bool ShouldProcessKey(const ipc::proto::KeyEvent& key) = 0;
  // Tells the engine to process a key.
  virtual void ProcessKey(const ipc::proto::KeyEvent& key) = 0;
  // Tells the that a mouse event occurs over a range of text.
  virtual void ProcessMouseEvent(DWORD button_status, int character_offset) {}
  // This would either be invoked by Goopy UI or external candidate UI.
  virtual void SelectCandidate(int index, bool commit) = 0;
  // Informs the ime that the composition is terminated by frontend or
  // text service.
  virtual void EndComposition(bool commit) = 0;
  // Informs ime current input context got focus.
  virtual void FocusInputContext() = 0;
  // Informs ime current input context lost focus.
  virtual void BlurInputContext() = 0;
  // Enable external composition window if |enable| is true.
  virtual void EnableCompositionWindow(bool enable) = 0;
  // Enable external candidates window if |enable| is true.
  virtual void EnableCandidateWindow(bool enable) = 0;
  virtual void EnableToolbarWindow(bool enable) = 0;
  virtual void EnableSoftkeyboard(bool enable) {}
  virtual void UpdateInputCaret() {}
  // Called by ContextInterface to notify ime of converison mode.
  virtual void NotifyConversionModeChange(uint32 conversion_mode) = 0;
  virtual uint32 GetConversionMode() { return 0; }
  // Selection or contents in the document have changed.
  virtual void DocumentChanged(int change_flags) {}
  // Tells the engine that the number of candidates on each page has changed.
  virtual void ResizeCandidatePage(int new_size) = 0;
  // Requests the engine to show detail info. It is up to the implement to
  // choose a proper way to display these info.
  virtual void ShowInfo() {}

  // For a specified text_state, returns the style index for it. The TSF module
  // would invoke this method whenever the display needs update.
  virtual int GetTextStyleIndex(TextState text_state) { return 0; }

  // Imports user dictionary from |file name|.
  // This function will first verify the certification info of the calling exe
  // module. If the certifications do not exist or the certifications are
  // invalid, it will provide a pop-up message box for the user to approve or
  // deny the import action. If and only if the certifications of the exe file
  // are trusted or the user approve the action, the importing procedure will
  // proceed.
  // Returns false if user cancel the action or importing from |dict_file|
  // failed.
  virtual bool ImportDictionary(const wchar_t* file_name) = 0;
  virtual void SetContext(ContextInterface* context) = 0;
};

// Implemented by input method.
class UIManagerInterface {
 public:
  virtual ~UIManagerInterface() {}

  virtual void SetContext(ContextInterface* engine) = 0;
  virtual void SetToolbarStatus(bool is_open) = 0;
  virtual void Update(uint32 component) = 0;
  virtual void LayoutChanged() = 0;
};

// InputMethod class is used to define a specific input method. All members are
// static and are implemented in client code of this framework. It's not
// necessary to implement all members if you are not using dual-interface.
class InputMethod {
 public:
  static UIManagerInterface* CreateUIManager(HWND parent);
  static EngineInterface* CreateEngine(ContextInterface* context);
  static void DestroyEngineOfContext(ContextInterface* context);
  static bool ShowConfigureWindow(HWND parent);

  // All the text styles. This would be called by DisplayAttributeManager
  // for its initialization.
  static int GetTextStyleCount();
  static void GetTextStyle(int index, TextStyle *style);

  static const int kMaxDisplayNameLength = 32;
  static const wchar_t kDisplayName[kMaxDisplayNameLength];

  static const DWORD kConversionModeMask;
  static const DWORD kSentenceModeMask;
  static const DWORD kImmProperty;

  // For IMM.
  static const int kMaxUIClassNameLength = 16;
  static const wchar_t kUIClassName[kMaxUIClassNameLength];

  // For TSF.
  static const CLSID kTextServiceClsid;
  static const GUID kInputAttributeGUID;
  static const int kRegistrarScriptId;
};
}  // namespace ime_goopy

#endif  // GOOPY_COMMON_FRAMEWORK_INTERFACE_H__
