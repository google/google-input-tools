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
// This file is integrated from ime/goopy/imetool/imetool.cc

#include "imm/order.h"

#include <stdio.h>
#include <windows.h>
#include <shlwapi.h>


namespace ime_goopy {
namespace imm {
static const WCHAR* kImeFileName = L"GooglePinyin.ime";
static const WCHAR* kImeDescription =
    L"\x4E2D\x6587 (\x7B80\x4F53) - \x8C37\x6B4C\x62FC\x97F3\x8F93\x5165\x6CD5";
static const WCHAR* kImeRegLayout =
    L"SYSTEM\\CurrentControlSet\\Control\\Keyboard Layouts";
static const WCHAR* kRegLayoutPattern =
    L"SYSTEM\\CurrentControlSet\\Control\\Keyboard Layouts\\%s";
static const WCHAR* kImeRegPreload =
    L"Keyboard Layout\\Preload";
static const WCHAR* kEnglishLayout = L"00000409";
static const int kMaxKeyLength = 256;
static const int kMaxValueName = 16383;
static const int kMaxImeNumber = 256;

// Constants used for sort order settings under Vista. The UUID here is the
// actual value of GUID_TFCAT_TIP_KEYBOARD, which is defined by TSF.
static const WCHAR* kImeRegPreloadSortOrder =
    L"Software\\Microsoft\\CTF\\SortOrder\\AssemblyItem\\0x00000804\\"
    L"{34745C63-B2F0-4784-8B67-5E12C8701A31}";
static const WCHAR* kClsidValueName = L"CLSID";
static const WCHAR* kLayoutValueName = L"KeyboardLayout";
static const WCHAR* kProfileValueName = L"Profile";
static const WCHAR* kFirstSubkey = L"00000000";
static const WCHAR* kEmptyGuid = L"{00000000-0000-0000-0000-000000000000}";
static const DWORD kChineseUsLayout = 0x08040804;

void SwapImePreload(HKEY hkey, WCHAR* ime,
                    WCHAR* first_index, WCHAR* second_index) {
  WCHAR value[kMaxValueName];
  DWORD len = kMaxValueName;
  DWORD type = 0;
  RegQueryValueEx(hkey, second_index, NULL, &type,
                  reinterpret_cast<BYTE*>(value), &len);
  RegSetValueEx(hkey, second_index, NULL, REG_SZ,
                reinterpret_cast<BYTE*>(ime),
                (wcslen(ime) + 1) * sizeof(ime[0]));
  RegSetValueEx(hkey, first_index, NULL, REG_SZ,
                reinterpret_cast<BYTE*>(value),
                (wcslen(value) + 1) * sizeof(ime[0]));
}

BOOL MoveTopPreload(WCHAR* ime_name) {
  HKEY hkey = NULL;
  HKEY other_hkey = NULL;
  if (RegOpenKeyEx(HKEY_CURRENT_USER, kImeRegPreload, 0,
                   KEY_ALL_ACCESS, &hkey) == ERROR_SUCCESS) {
    // Get the class name and the value count.
    DWORD value_count = 0;
    DWORD longest_value = 0;
    DWORD longest_name = 0;
    if (RegQueryInfoKey(hkey, NULL, NULL, NULL, NULL, NULL, NULL, &value_count,
        &longest_name, &longest_value, NULL, NULL) == ERROR_SUCCESS) {
      // Find the index of our IME
      WCHAR current_item[kMaxValueName] = { 0 };

      for (DWORD i = 0; i < value_count; i++) {
        DWORD type = 0;
        DWORD value_len = longest_value + 1;  // include the terminal chars.
        DWORD name_len = longest_name + 1;  // include the terminal chars.
        WCHAR value[kMaxValueName] = L"";
        WCHAR name[kMaxValueName] = L"";
        if (longest_value < kMaxValueName &&
            RegEnumValue(
                hkey, i, name, &name_len, NULL, &type,
                reinterpret_cast<BYTE*>(value), &value_len) == ERROR_SUCCESS) {
          if (_wcsicmp(value, ime_name) == 0) {
            wcscpy(current_item, name);
            break;
          }
        }
      }
      if (current_item[0]) {
        // swap our IME with the first IME in preload list
        SwapImePreload(hkey, ime_name, current_item, L"1");
      }
    }
  }
  RegCloseKey(hkey);
  return TRUE;
}

void ChangePreload(WCHAR* ime_name, BOOL is_insert) {
  HKEY hkey = NULL;
  HKL other_hkl = NULL;
  if (RegOpenKeyEx(HKEY_CURRENT_USER, kImeRegPreload, 0,
                   KEY_ALL_ACCESS, &hkey) == ERROR_SUCCESS) {
    // Get the class name and the value count.
    DWORD value_count = 0;
    DWORD longest_value = 0;
    DWORD longest_name = 0;
    if (RegQueryInfoKey(hkey, NULL, NULL, NULL, NULL, NULL, NULL,
                        &value_count, &longest_name, &longest_value,
                        NULL, NULL) == ERROR_SUCCESS) {
      // find goppy ime name and value in preload list.
      WCHAR gpy_ime_name[kMaxValueName] = { 0 };
      WCHAR gpy_ime_value[kMaxValueName] = { 0 };
      for (DWORD i = 0; i < value_count; i++) {
        DWORD type = 0;
        DWORD value_len = longest_value + 1;
        DWORD name_len = longest_name + 1;
        WCHAR value[kMaxValueName] = L"";
        WCHAR name[kMaxValueName] = L"";
        if (longest_value < kMaxValueName &&
            RegEnumValue(
                hkey, i, name, &name_len, NULL, &type,
                reinterpret_cast<BYTE*>(value), &value_len) == ERROR_SUCCESS) {
          if (_wcsicmp(value, ime_name) == 0) {
            wcscpy(gpy_ime_name, name);
            wcscpy(gpy_ime_value, value);
            break;
          }
        }
      }
      if (!is_insert) {
        if (*gpy_ime_name) {
          WCHAR last_name[kMaxValueName];
          wsprintf(last_name, L"%d", value_count);
          // swap goopy with another ime in preload list.
          SwapImePreload(hkey, gpy_ime_value, gpy_ime_name, last_name);
          // delete the key of goopy
          RegDeleteValue(hkey, last_name);
        }
      } else {
        // if goopy already existed in preload list.
        if (_wcsicmp(gpy_ime_value, ime_name) == 0) {
          // if goopy is not the first layout, change it to the second position.
          if (_wcsicmp(gpy_ime_name, L"1"))
            SwapImePreload(hkey, gpy_ime_value, gpy_ime_name, L"2");
        } else {
          // add Google Pinyin at the last of the list
          WCHAR num_str[kMaxValueName];
          wsprintf(num_str, L"%d", value_count + 1);
          RegSetValueEx(hkey, num_str, NULL, REG_SZ,
                        reinterpret_cast<const BYTE*>(ime_name),
                        (wcslen(ime_name) + 1) * sizeof(ime_name[0]));
          // then swap Google Pinyin to the seconde position.
          SwapImePreload(hkey, ime_name, num_str, L"2");
        }
      }
    }
    RegCloseKey(hkey);
  }
}

void SwapStringValueBetweenTwoSubkeys(HKEY hsubkey1,
                                      HKEY hsubkey2,
                                      const WCHAR* value_name) {
  WCHAR value1[MAX_PATH] = {0};
  WCHAR value2[MAX_PATH] = {0};
  DWORD len = MAX_PATH;
  DWORD type = 0;
  RegQueryValueEx(hsubkey1, value_name, NULL, &type,
                  reinterpret_cast<BYTE*>(&value1), &len);
  RegQueryValueEx(hsubkey2, value_name, NULL, &type,
                  reinterpret_cast<BYTE*>(&value2), &len);
  RegSetValueEx(hsubkey1, value_name, 0, REG_SZ,
                reinterpret_cast<BYTE*>(value2),
                (wcslen(value2) + 1) * sizeof(value2[0]));
  RegSetValueEx(hsubkey2, value_name, 0, REG_SZ,
                reinterpret_cast<BYTE*>(value1),
                (wcslen(value1) + 1) * sizeof(value1[0]));
}

void SwapDwordValueBetweenTwoSubkeys(HKEY hsubkey1,
                                     HKEY hsubkey2,
                                     const WCHAR* value_name) {
  DWORD value1 = 0;
  DWORD value2 = 0;
  DWORD len = sizeof(DWORD);
  DWORD type = 0;
  RegQueryValueEx(hsubkey1, value_name, NULL, &type,
                  reinterpret_cast<BYTE*>(&value1), &len);
  RegQueryValueEx(hsubkey2, value_name, NULL, &type,
                  reinterpret_cast<BYTE*>(&value2), &len);
  RegSetValueEx(hsubkey1, value_name, 0, REG_DWORD,
                reinterpret_cast<BYTE*>(&value2), len);
  RegSetValueEx(hsubkey2, value_name, 0, REG_DWORD,
                reinterpret_cast<BYTE*>(&value1), len);
}

void SwapImePreloadSortOrder(HKEY hkey,
                             const WCHAR* subkey1,
                             const WCHAR* subkey2) {
  HKEY hsubkey1 = NULL;
  HKEY hsubkey2 = NULL;
  if (RegOpenKeyEx(hkey, subkey1, 0,
                   KEY_ALL_ACCESS, &hsubkey1) == ERROR_SUCCESS &&
      RegOpenKeyEx(hkey, subkey2, 0,
                   KEY_ALL_ACCESS, &hsubkey2) == ERROR_SUCCESS) {
    SwapStringValueBetweenTwoSubkeys(hsubkey1, hsubkey2, kClsidValueName);
    SwapDwordValueBetweenTwoSubkeys(hsubkey1, hsubkey2, kLayoutValueName);
    SwapStringValueBetweenTwoSubkeys(hsubkey1, hsubkey2, kProfileValueName);
  }
  if (hsubkey1)
    RegCloseKey(hsubkey1);
  if (hsubkey2)
    RegCloseKey(hsubkey2);
}

void ChangePreloadSortOrder(const WCHAR* goopy_layout_id, BOOL is_insert) {
  HKEY hkey = NULL;
  // Currently only Vista has SortOrder key. On the system which has no such
  // key, this function will simply return. Thus we don't need to detect OS
  // first.
  if (RegOpenKeyEx(HKEY_CURRENT_USER, kImeRegPreloadSortOrder, 0,
                   KEY_ALL_ACCESS, &hkey) == ERROR_SUCCESS) {
    DWORD goopy_layout_value = 0;
    swscanf(goopy_layout_id, L"%x", &goopy_layout_value);
    // The Ctrl-Space rule in Vista is: when Chinese US is the default keyboard,
    // and no IME has been switched out before in an application, the OS will
    // select the layout right after the Chinese US from the SortOrder
    // list. Thus we need to locate the layout next to the Chinese US keyboard
    // then swap it with Goopy.
    DWORD subkey_index = 0;
    DWORD chinese_us_subkey_index = -1;
    WCHAR subkey[kMaxKeyLength] = { 0 };
    WCHAR gpy_subkey[kMaxKeyLength] = { 0 };
    WCHAR subkey_next_to_chinese_us[kMaxKeyLength] = { 0 };
    BOOL last_entry_is_chinese_us = FALSE;
    // Finds the goopy sub key and the subkey next to the Chinese US Keyboard
    // sub key from the sort order list.
    for (subkey_index = 0; subkey_index < kMaxImeNumber; subkey_index++) {
      wsprintf(subkey, L"%08d", subkey_index);
      HKEY hsubkey = NULL;
      if (RegOpenKeyEx(hkey, subkey, 0, KEY_ALL_ACCESS, &hsubkey)
          != ERROR_SUCCESS)
        break;
      DWORD value = 0;
      DWORD len = sizeof(value);
      DWORD type = 0;
      if (RegQueryValueEx(hsubkey, kLayoutValueName, NULL, &type,
                          reinterpret_cast<BYTE*>(&value), &len)
          == ERROR_SUCCESS) {
        if (subkey_index == 0 && value != kChineseUsLayout) {
          // If there is no Chinese US layout found, or it's found at the end of
          // the list, the value of subkey_next_to_chinese_us will be the first
          // sub key.
          wcscpy(subkey_next_to_chinese_us, subkey);
        } else if (last_entry_is_chinese_us) {
          // Stores the sub key when Chinese US is found last time.
          wcscpy(subkey_next_to_chinese_us, subkey);
          last_entry_is_chinese_us = FALSE;
        }
        if (value == kChineseUsLayout) {
          last_entry_is_chinese_us = TRUE;
          chinese_us_subkey_index = subkey_index;
        } else if (value == goopy_layout_value) {
          // Stores goopy sub key when it is found.
          wcscpy(gpy_subkey, subkey);
        }
      }
      RegCloseKey(hsubkey);
    }
    // Records the last sub key.
    WCHAR last_subkey[kMaxKeyLength] = { 0 };
    if (subkey_index > 0)
      wsprintf(last_subkey, L"%08d", subkey_index - 1);
    // If there is no goopy sub key, try to insert it.
    if (!(*gpy_subkey) && is_insert) {
      HKEY hsubkey = NULL;
      wsprintf(subkey, L"%08d", subkey_index);
      if (RegCreateKey(hkey, subkey, &hsubkey) == ERROR_SUCCESS) {
        RegSetValueEx(hsubkey, kClsidValueName, 0, REG_SZ,
                      reinterpret_cast<const BYTE*>(kEmptyGuid),
                      (wcslen(kEmptyGuid) + 1) * sizeof(kEmptyGuid[0]));
        RegSetValueEx(hsubkey, kLayoutValueName, 0, REG_DWORD,
                      reinterpret_cast<const BYTE*>(&goopy_layout_value),
                      sizeof(goopy_layout_value));
        RegSetValueEx(hsubkey, kProfileValueName, 0, REG_SZ,
                      reinterpret_cast<const BYTE*>(kEmptyGuid),
                      (wcslen(kEmptyGuid) + 1) * sizeof(kEmptyGuid[0]));
        RegCloseKey(hsubkey);
        // Saves the new goopy sub key.
        wcscpy(gpy_subkey, subkey);
        // If the last sub key before adding goopy is Chinese US, update the
        // value of subkey_next_to_chinese_us.
        if (chinese_us_subkey_index == subkey_index - 1)
          wcscpy(subkey_next_to_chinese_us, subkey);
      }
    }
    if (*gpy_subkey) {
      if (!is_insert) {
        // Swaps out then deletes the goopy sub key.
        if (_wcsicmp(gpy_subkey, last_subkey))
          SwapImePreloadSortOrder(hkey, gpy_subkey, last_subkey);
        RegDeleteKey(hkey, last_subkey);
      } else if (*subkey_next_to_chinese_us &&
                 _wcsicmp(gpy_subkey, subkey_next_to_chinese_us)) {
        // Swaps goopy right after the Chinese US layout.
        SwapImePreloadSortOrder(hkey, gpy_subkey, subkey_next_to_chinese_us);
      }
    }
    RegCloseKey(hkey);
  }  
}

void Order::SetFirstChineseIME(HKL hkl) {
  wchar_t ime_key_name[kMaxValueName];
  wsprintf(ime_key_name, L"%08x", hkl);
  ChangePreload(ime_key_name, TRUE);
  ChangePreloadSortOrder(ime_key_name, TRUE);
}
}  // namespace imm
}  // namespace ime_goopy
