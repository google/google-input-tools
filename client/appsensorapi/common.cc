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
#include "appsensorapi/common.h"

namespace ime_goopy {
const TCHAR* const FileInfoKey::kCompanyName = TEXT("CompanyName");
const TCHAR* const FileInfoKey::kComments = TEXT("Comments");
const TCHAR* const FileInfoKey::kFileDescription = TEXT("FileDescription");
const TCHAR* const FileInfoKey::kFileVersion = TEXT("FileVersion");
const TCHAR* const FileInfoKey::kInternalName = TEXT("InternalName");
const TCHAR* const FileInfoKey::kLegalCopyright = TEXT("LegalCopyright");
const TCHAR* const FileInfoKey::kLegalTrademarks = TEXT("LegalTrademarks");
const TCHAR* const FileInfoKey::kOriginalFilename = TEXT("OriginalFilename");
const TCHAR* const FileInfoKey::kProductName = TEXT("ProductName");
const TCHAR* const FileInfoKey::kProductVersion = TEXT("ProductVersion");
const TCHAR* const FileInfoKey::kPrivateBuild = TEXT("PrivateBuild");
const TCHAR* const FileInfoKey::kSpecialBuild = TEXT("SpecialBuild");

const char* const FunctionName::kInitFuncName = "AppSensorInit";
const char* const FunctionName::kHandleMessageFuncName =
    "AppSensorHandleMessage";
const char* const FunctionName::kHandleCommandFuncName =
    "AppSensorHandleCommand";
const char* const FunctionName::kRegisterHandlerFuncName =
    "AppSensorRegisterHandler";

const TCHAR* const FileInfoKey::kAllInfoStrings[] = {
  kCompanyName,
  kComments,
  kFileDescription,
  kFileVersion,
  kInternalName,
  kLegalCopyright,
  kLegalTrademarks,
  kOriginalFilename,
  kProductName,
  kProductVersion,
  kPrivateBuild,
  kSpecialBuild
};
}
