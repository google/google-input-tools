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

#include "imm/registrar.h"

#include <atlbase.h>
#include <shlwapi.h>
#include <strsafe.h>

#include <algorithm>
#include <string>
#include <vector>

#include "base/logging.h"
#include "base/stringprintf.h"
#include "base/string_utils_win.h"
#include "imm/immdev.h"

namespace ime_goopy {
namespace imm {
namespace {
const wchar_t kKeyboardLayoutRegistry[] =
    L"SYSTEM\\CurrentControlSet\\Control\\Keyboard Layouts";
const wchar_t kPreloadRegistry[] = L"Keyboard Layout\\Preload";
const wchar_t kImeFile[] = L"Ime File";
const wchar_t kLayoutFile[] = L"Layout File";
const wchar_t kLayoutText[] = L"Layout Text";
const int kHKLLength = 8;
const int kMaxKeyLength = 255;
const int kDeviceIDMin = 0xE020;
const int kDeviceIDMax = 0xE0FF;
struct ImeEntry {
  DWORD hkl;
  std::wstring ime_file;
  std::wstring layout_text;
};

// Gets a list of IMM based input methods currently installed in the system.
// It will enumerates the keys under
// "HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Control\\Keyboard Layouts"
// and find all the keys representing a IMM based input method.
// A registry key is considered as an IMM based input method if all the
// following conditions are true:
//   1. The key name is in the form of HKL which is composed of a device id and
//      a language id, each of them are of the length of a WORD(16bit).
//   2. The device id is in the range of [kDeviceIDMin, kDeviceIDMax];
//   3. There are string values of name "Ime File" and "Layout Text" under this
//      key.
void GetIMEEntries(std::vector<ImeEntry>* entries) {
  entries->clear();
  // TODO(synch): Use RegistryKey.
  CRegKey key;
  key.Open(HKEY_LOCAL_MACHINE, kKeyboardLayoutRegistry,
           KEY_READ | KEY_WOW64_64KEY);
  DCHECK(key.m_hKey);
  if (!key.m_hKey)
    return;
  int index = 0;
  while (1) {
    wchar_t hkl_name[kMaxKeyLength] = {0};
    DWORD len = kMaxKeyLength;
    if (key.EnumKey(index++, hkl_name, &len) != ERROR_SUCCESS)
      break;
    wchar_t* parse_end = NULL;
    DWORD hkl_value = wcstoul(hkl_name, &parse_end, 16);
    DCHECK_EQ(hkl_name + 8, parse_end);
    if (hkl_name + kHKLLength != parse_end)
      continue;
    if (HIWORD(hkl_value) < kDeviceIDMin || HIWORD(hkl_value) > kDeviceIDMax)
      continue;
    CRegKey subkey;
    if (subkey.Open(key.m_hKey, hkl_name, KEY_READ | KEY_WOW64_64KEY) !=
        ERROR_SUCCESS) {
      continue;
    }
    wchar_t buffer[MAX_PATH] = {0};
    ULONG size = MAX_PATH;
    if (subkey.QueryStringValue(kImeFile, buffer, &size) != ERROR_SUCCESS)
      continue;
    ImeEntry entry;
    entry.hkl = hkl_value;
    entry.ime_file = buffer;
    size = MAX_PATH;
    if (subkey.QueryStringValue(kLayoutText, buffer, &size) != ERROR_SUCCESS)
      continue;
    entry.layout_text = buffer;
    entries->push_back(entry);
  }
}

// Get a unused device ID with respect to all the current input methods.
int GetNewDeviceID(const std::vector<ImeEntry>& ime_entries) {
  std::vector<int> device_ids;
  if (ime_entries.empty())
    return kDeviceIDMin;
  for (size_t i = 0; i < ime_entries.size(); ++i)
    device_ids.push_back(HIWORD(ime_entries[i].hkl));
  std::sort(device_ids.begin(), device_ids.end());
  // Prefered unused device ids than recycled ones.
  if (device_ids.back() + 1 <= kDeviceIDMax)
    return device_ids.back() + 1;
  if (device_ids.front() - 1 >= kDeviceIDMin)
    return device_ids.front() - 1;
  for (int i = 0; i < device_ids.size() - 1; ++i) {
    if (device_ids[i + 1] - device_ids[i] > 1)
      return device_ids[i] + 1;
  }
  NOTREACHED() << "All device IDs are taken";
  return 0;
}

// Write the ime information to
// "HKLM\\SYSTEM\\CurrentControlSet\\Control\\Keyboard Layouts\\<hkl>"
bool WriteImeEntryToRegistry(DWORD hkl,
                             const std::wstring& ime_file,
                             const std::wstring& layout_text) {
  CRegKey key;
  key.Open(HKEY_LOCAL_MACHINE, kKeyboardLayoutRegistry,
           KEY_ALL_ACCESS | KEY_WOW64_64KEY);
  DCHECK(key.m_hKey);
  if (!key.m_hKey)
    return false;
  CRegKey subkey;
  std::wstring key_name = WideStringPrintf(L"%08X", hkl);
  subkey.Create(key.m_hKey, key_name.c_str(), 0, 0,
                KEY_ALL_ACCESS | KEY_WOW64_64KEY);
  DCHECK(subkey.m_hKey);
  if (!subkey.m_hKey)
    return false;
  if (subkey.SetStringValue(kImeFile, ime_file.c_str()) != ERROR_SUCCESS) {
    subkey.Close();
    key.DeleteSubKey(key_name.c_str());
    return false;
  }
  if (subkey.SetStringValue(kLayoutText, layout_text.c_str()) !=
      ERROR_SUCCESS) {
    subkey.Close();
    key.DeleteSubKey(key_name.c_str());
    return false;
  }
  std::wstring layout_file;
  // Janpanese and Korean have their owned keyboard layout, others uses default
  // US keyboard layout.
  switch (LOBYTE(hkl)) {
    case LANG_JAPANESE:
      layout_file = L"kbdjpn.dll";
      break;
    case LANG_KOREAN:
      layout_file = L"kbdkor.dll";
      break;
    default:
      layout_file = L"kbdus.dll";
      break;
  }
  if (subkey.SetStringValue(kLayoutFile, layout_file.c_str()) !=
      ERROR_SUCCESS) {
    subkey.Close();
    key.DeleteSubKey(key_name.c_str());
    return false;
  }
  return true;
}

// Write the registered HKL to the preload list under
// "HKEY_CURRENT_USER\\Keyboard Layout\\Preload" so that this input method will
// appears in the language bar.
bool SetPreload(const std::wstring& hkl_name) {
  CRegKey key;
  key.Open(HKEY_CURRENT_USER, kPreloadRegistry,
           KEY_ALL_ACCESS | KEY_WOW64_64KEY);
  DCHECK(key.m_hKey);
  if (!key.m_hKey)
    return false;
  for (int i = 1; i < 0xff; ++i) {
    wchar_t buffer[MAX_PATH] = {0};
    ULONG size = MAX_PATH;
    std::wstring value_key = WideStringPrintf(L"%d", i);
    if (key.QueryStringValue(value_key.c_str(), buffer, &size) !=
        ERROR_SUCCESS) {
      key.SetStringValue(value_key.c_str(), hkl_name.c_str());
      return true;
    }
    if (hkl_name == buffer)
      return true;
  }
  return false;
}
}  // namespace

static const wchar_t* kRegLayoutPattern =
    L"SYSTEM\\CurrentControlSet\\Control\\Keyboard Layouts\\%X";

Registrar::Registrar(const wstring& base_filename)
    : base_filename_(base_filename) {
  assert(base_filename_.size());
}

HRESULT Registrar::Register(const wstring& display_name, HKL* hkl) {
  // If the IME is already registered, return directly. If we install 32-bit
  // and 64-bit IME side-by-side in a 64-bit Windows, the ImmInstallIME
  // function should be called only once, either for the 32-bit or 64-bit
  // version of the IME DLL.
  HKL check_hkl = GetHKL();
  if (check_hkl) {
    if (hkl) *hkl = check_hkl;
    return S_OK;
  }

  // ImmInstallIME has a bug in 64-bit Windows. It can't recoganize SysWOW64
  // folder as a system folder, so it will refuse to install our IME. The
  // solution here is to combine the 64-bit System32 folder and our filename
  // to make ImmInstallIME happy.
  wchar_t fake_path[MAX_PATH] = {0}, system_path[MAX_PATH] = {0};
  GetSystemDirectory(system_path, MAX_PATH);
  PathCombine(fake_path, system_path, base_filename_.c_str());

  HKL installed_hkl = ImmInstallIME(fake_path, display_name.c_str());
  if (hkl) *hkl = installed_hkl;
  return installed_hkl ? S_OK : E_FAIL;
}

HRESULT Registrar::Unregister(HKL hkl) {
  if (!hkl) return E_FAIL;

  UnloadKeyboardLayout(hkl);

  // Remove IME registry key.
  wchar_t key_name[MAX_PATH];
  StringCchPrintf(key_name, MAX_PATH, kRegLayoutPattern, hkl);
  SHDeleteKey(HKEY_LOCAL_MACHINE, key_name);

  return S_OK;
}

HRESULT Registrar::Unregister() {
  return Unregister(GetHKL());
}

HRESULT Registrar::Unregister(DWORD language_id,
                              const std::wstring& display_name) {
  return Unregister(GetHKL(language_id, display_name));
}

HKL Registrar::GetHKL() {
  // Get the list of activated keyboard layouts.
  UINT size = GetKeyboardLayoutList(0, NULL);
  if (!size) return NULL;
  scoped_array<HKL> list(new HKL[size]);
  if (!GetKeyboardLayoutList(size, list.get())) return NULL;

  // Find the layout which match the filename.
  wchar_t filename[MAX_PATH];
  for (int i = 0; i < size; i++) {
    if (!ImmGetIMEFileName(list[i], filename, MAX_PATH)) continue;
    if (!lstrcmpi(base_filename_.c_str(), filename)) return list[i];
  }
  return NULL;
}

HKL Registrar::Register(DWORD language_id, const std::wstring& display_name) {
  std::vector<ImeEntry> ime_entries;
  GetIMEEntries(&ime_entries);
  DWORD hkl_value = 0;
  for (int i = 0; i < ime_entries.size(); ++i) {
    if (LOWORD(ime_entries[i].hkl) == language_id &&
        ime_entries[i].ime_file == base_filename_.c_str() &&
        ime_entries[i].layout_text == display_name) {
      // Return the HKL if already exist.
      hkl_value = ime_entries[i].hkl;
      break;
    }
  }
  if (!hkl_value) {
    DWORD device_id = GetNewDeviceID(ime_entries);
    if (!device_id)
      return NULL;
    hkl_value = (device_id << 16) + language_id;
    if (!WriteImeEntryToRegistry(hkl_value, base_filename_, display_name))
      return NULL;
  }
  std::wstring hkl_name = WideStringPrintf(L"%08X", hkl_value);
  SetPreload(hkl_name);
  // LoadKeyboardLayout may fail if system doens't enable eastern asian language
  // support, |Register| should returns valid HKL in this case to make sure the
  // IME works after user enable the support.
  HKL hkl = ::LoadKeyboardLayout(hkl_name.c_str(),
                                 KLF_ACTIVATE | KLF_SUBSTITUTE_OK);
  if (!hkl)
    hkl = reinterpret_cast<HKL>(hkl_value);
  return hkl;
}

HKL Registrar::GetHKL(DWORD language_id,
                      const std::wstring& display_name) {
  std::vector<ImeEntry> ime_entries;
  GetIMEEntries(&ime_entries);
  DWORD hkl_value = 0;
  for (int i = 0; i < ime_entries.size(); ++i) {
    if (LOWORD(ime_entries[i].hkl) == language_id &&
        ime_entries[i].ime_file == base_filename_.c_str() &&
        ime_entries[i].layout_text == display_name) {
      // Return the HKL if already exist.
      hkl_value = ime_entries[i].hkl;
      return ::LoadKeyboardLayout(WideStringPrintf(L"%08X", hkl_value).c_str(),
                                  KLF_NOTELLSHELL);
    }
  }
  return NULL;
}

}  // namespace imm
}  // namespace ime_goopy
