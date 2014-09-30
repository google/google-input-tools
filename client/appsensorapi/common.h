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
//
// This file contains some predefined constant variables which are used at
// client side.
#ifndef GOOPY_APPSENSORAPI_COMMON_H__
#define GOOPY_APPSENSORAPI_COMMON_H__

#include <windows.h>

namespace ime_goopy {

// Declares all predefined version information Unicode strings.
// These strings are also keys to the file_info std::map in VersionInfo.
class FileInfoKey {
 public:
  static const TCHAR* const kCompanyName;
  static const TCHAR* const kComments;
  static const TCHAR* const kFileDescription;
  static const TCHAR* const kFileVersion;
  static const TCHAR* const kInternalName;
  static const TCHAR* const kLegalCopyright;
  static const TCHAR* const kLegalTrademarks;
  static const TCHAR* const kOriginalFilename;
  static const TCHAR* const kProductName;
  static const TCHAR* const kProductVersion;
  static const TCHAR* const kPrivateBuild;
  static const TCHAR* const kSpecialBuild;
  // Declares an array to contain all above strings for easy access.
  static const int kNumInfoStrings = 12;
  static const TCHAR* const kAllInfoStrings[kNumInfoStrings];
};

// Declares the names of export functions in the appsensorapi library.
class FunctionName {
 public:
  static const char* const kInitFuncName;
  static const char* const kHandleMessageFuncName;
  static const char* const kHandleCommandFuncName;
  static const char* const kRegisterHandlerFuncName;
};

// Represents the command indicators to pass in HandleCommand method.
// The sensitive framework supports more commands to be added, by extending
// this enumration with individual command ids.
enum HandlerCommand {
  CMD_SHOULD_ASSEMBLE_COMPOSITION = 1,   // Returns true if the composition
                                         // should be assembled from selected
                                         // candidate strings, or false for a
                                         // simple copy of the input string.
                                         // The return status will be passed in
                                         // the second argument of the function
                                         // HandleCommand.
  CMD_SHOULD_USE_FALLBACK_UI,  // Returns true if IME should use default UI
                               // Manager, or false if IME should use skin UI.
                               // The second argument of HandleCommand should be
                               // a pointer of UI_Selection_Data and the return
                               // status will be passed in
                               // UI_Selection_Data::should_use_default_ui.
  CMD_SHOULD_DISABLE_IME,  // Returns true if ime should be disabled for
                           // specific application.
  CMD_SHOULD_DESTROY_FRONTEND, // Returns true if we should destroy the frontend
                               // rather than shelfing it.
};

// The input-output structure of the command CMD_SHOULD_USE_FALLBACK_UI.
// Should be used as the second argument of HandleCommand when the command is
// CMD_SHOULD_USE_FALLBACK_UI.
struct UI_Selection_Data {
  // The handler of the UI_Window which is a subwindow of "Default IME" and
  // "Default IME" is the subwindow of the application window.
  HWND hwnd;
  // True if we should use fallbackui instead of skin UI.
  bool should_use_default_ui;
};

}
#endif  // GOOPY_APPSENSORAPI_COMMON_H__
