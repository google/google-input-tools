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

#include "base/commandlineflags.h"
#include "base/string_utils_win.h"
#include "common/shellutils.h"
#include "imm/registrar.h"
#include "tsf/registrar.h"

// Action can be register_ime, unregister_ime.
DEFINE_string(action, "", "Register or unregister IME for language");
// Language that needs to register or unregister an IME.
DEFINE_int32(languageid, 0x0804, "Language id to register or unregister");
DEFINE_string(profile_guid, "{82EAA633-3007-4D7E-A04D-906B05E200DB}", "the guid of the test service");
DEFINE_string(name, "Google Input Tools", "The name of the input method");
DEFINE_string(filename, "GoogleInputTools.ime", "The file name of the ime file");

namespace {
// {3C575191-98EC-4FB2-BE2C-54633AC54329}
static const GUID kTextServiceClsid =
      {0x3c575191, 0x98ec, 0x4fb2, {0xbe, 0x2c, 0x54, 0x63, 0x3a, 0xc5, 0x43, 0x29}};

static bool RegisterCOM(bool wow64,
                        const wchar_t* text_service_registry_key,
                        const wchar_t* ime_display_name,
                        const wchar_t* ime_file_name) {
  DWORD flags = KEY_READ | KEY_WRITE;
  if (wow64)
    flags |= KEY_WOW64_32KEY;
  else
    flags |= KEY_WOW64_64KEY;
  CRegKey registry;
  if (registry.Create(HKEY_CLASSES_ROOT,
                      text_service_registry_key,
                      REG_NONE,
                      REG_OPTION_NON_VOLATILE,
                      flags) != ERROR_SUCCESS)
    return false;

  CRegKey inproc;
  if (inproc.Create(registry,
                    L"InprocServer32",
                    REG_NONE,
                    REG_OPTION_NON_VOLATILE,
                    flags) != ERROR_SUCCESS) {
    return false;
  }
  registry.SetStringValue(NULL, ime_display_name);
  wchar_t path[MAX_PATH] = {0};
  ::SHGetFolderPath(NULL, wow64 ? CSIDL_SYSTEMX86 : CSIDL_SYSTEM, NULL,
      SHGFP_TYPE_CURRENT, path); 
  ::PathAppend(path, ime_file_name);
  inproc.SetStringValue(NULL, path);
  inproc.SetStringValue(L"ThreadingModel", L"Apartment");
  return true;
}
}

using ime_goopy::ShellUtils;
using ime_goopy::WideStringPrintf;
using ime_goopy::Utf8ToWide;

int WINAPI _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR lpstrCmdLine,
                     int nCmdShow) {
  ParseCommandLineFlags(GetCommandLineW());
  ::CoInitialize(NULL);
  if (!ime_goopy::ShellUtils::IsCurrentUserAdmin()) {
    return -1;
  }
  int ret = -1;

  std::wstring display_name = Utf8ToWide(FLAGS_name);
  std::wstring file_name = Utf8ToWide(FLAGS_filename);
  wchar_t guid_str[MAX_PATH] = {0};
  ::StringFromGUID2(kTextServiceClsid, guid_str, MAX_PATH);
  std::wstring text_service_registry_key =
      WideStringPrintf(L"CLSID\\%s", guid_str);
  ime_goopy::imm::Registrar imm_registrar(file_name);
  ime_goopy::tsf::Registrar tsf_registrar(kTextServiceClsid);
  CLSID profile = {0};
  ::CLSIDFromString(Utf8ToWide(FLAGS_profile_guid).c_str(), &profile);
  
  if (FLAGS_action == "register_ime") {
    RegisterCOM(false,
                text_service_registry_key.c_str(),
                display_name.c_str(),
                file_name.c_str());
    if (ShellUtils::Is64BitOS()) {
      RegisterCOM(true,
                  text_service_registry_key.c_str(),
                  display_name.c_str(),
                  file_name.c_str());
    }
    HKL hkl = imm_registrar.Register(FLAGS_languageid, display_name);
    tsf_registrar.Register(file_name, FLAGS_languageid, profile, display_name, hkl);
    ret = 0;
  } else if (FLAGS_action == "unregister_ime") {
    imm_registrar.Unregister(FLAGS_languageid, display_name);
    tsf_registrar.Unregister(FLAGS_languageid, profile);
  } else {
    ret = -1;
  }
  return ret;
}
