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

#include <shlwapi.h>
#include <wincrypt.h>

#include "common/registry.h"

namespace ime_goopy {

RegistryKey::RegistryKey() {
}

RegistryKey::~RegistryKey() {
}

LONG RegistryKey::QueryStringValue(const wstring& name, wstring* value) {
  // Get the length of string.
  ULONG length = 0;
  LONG result = 0;
  result = CRegKey::QueryStringValue(name.c_str(), NULL, &length);
  if (result != ERROR_SUCCESS) return result;
  length++;

  // Get the string.
  scoped_array<wchar_t> buffer(new wchar_t[length]);
  result = CRegKey::QueryStringValue(name.c_str(), buffer.get(), &length);
  if (result != ERROR_SUCCESS) return result;

  *value = buffer.get();
  return ERROR_SUCCESS;
}

LONG RegistryKey::QueryMultiStringValue(const wstring& name,
                                        vector<wstring>* values) {
  assert(values);
  // Get the length of string.
  ULONG num_chars = 0;
  LONG result = 0;
  result = CRegKey::QueryMultiStringValue(name.c_str(), NULL, &num_chars);
  if (result != ERROR_SUCCESS) return result;

  // Get the string.
  // Allocate one character more to make sure the string read does terminate.
  scoped_array<wchar_t> buffer(new wchar_t[num_chars + 1]);
  buffer[num_chars] = L'\0';
  DWORD num_bytes = num_chars * sizeof(wchar_t);
  DWORD type = 0;
  result = CRegKey::QueryValue(name.c_str(), &type, buffer.get(), &num_bytes);
  if (result != ERROR_SUCCESS) return result;

  // Extracts strings out.
  // The returned strings are each terminated with '\0'. All strings with their
  // terminators are joined into one very long string and ends with another
  // NULL-terminator. There might be empty strings in list. So an example of the
  // string would like:
  // aaa\0\0bbb\0ccc\0\0
  // which represents ["aaa", "", "bbb", "ccc"]
  wchar_t* cur_pos = buffer.get();
  values->clear();
  wchar_t* end = buffer.get() + num_chars;
  while (cur_pos < end) {
    wstring value = cur_pos;
    values->push_back(value);
    cur_pos += value.length() + 1;
  }
  return ERROR_SUCCESS;
}

LONG RegistryKey::SetMultiStringValue(const wstring& name,
                                      const vector<wstring>& values) {
  uint32 length = 0;
  // Computes the total length including separator '\0' and trailing '\0'.
  for (vector<wstring>::const_iterator i = values.begin();
       i != values.end(); ++i)
    length += static_cast<uint32>(i->length()) + 1;
  length++;
  // Merges strings into a buffer.
  scoped_array<wchar_t> buffer(new wchar_t[length]);
  memset(buffer.get(), 0, length * sizeof(wchar_t));
  wchar_t* cur_pos = buffer.get();
  for (vector<wstring>::const_iterator i = values.begin();
       i != values.end(); ++i) {
    memcpy(cur_pos, i->c_str(), i->length() * sizeof(wchar_t));
    cur_pos += i->length() + 1;
  }

  // Sets the string into registry.
  // CRegKey::SetMultiStringValue doesn't work because it doesn't support a
  // parameter of buffer size so it takes L'\0' as the terminate mark for
  // the buffer which causes wrong result since the buffer here may have L'\0'
  // in-between.
  return CRegKey::SetValue(name.c_str(), REG_MULTI_SZ, buffer.get(),
                           (length - 1) * sizeof(wchar_t));
}

LONG RegistryKey::QueryBinaryValue(const wstring& name,
                                   scoped_array<uint8>* value,
                                   ULONG* length) {
  assert(name.size() && length);
  *length = 0;

  // Get the length.
  LONG result = 0;
  result = CRegKey::QueryBinaryValue(name.c_str(), NULL, length);
  if (result != ERROR_SUCCESS) return result;
  if (*length == 0) return ERROR_NO_DATA;

  // Get the binary.
  value->reset(new uint8[*length]);
  return CRegKey::QueryBinaryValue(name.c_str(), value->get(), length);
}

LONG RegistryKey::EnumKey(uint32 index, wstring* name) {
  assert(name);
  WCHAR buf[MAX_PATH] = {0};
  LONG result = RegEnumKey(m_hKey, index, buf, MAX_PATH);
  if (result != ERROR_SUCCESS) return result;
  *name = buf;
  return ERROR_SUCCESS;
}

LONG RegistryKey::QueryEncryptedValue(const wstring& name, wstring* value) {
  // Get the binary encrypted data from registry.
  ULONG length = 0;
  scoped_array<uint8> buffer;
  LONG result = 0;
  result = QueryBinaryValue(name, &buffer, &length);
  if (result != ERROR_SUCCESS) return result;

  // Decrypt.
  DATA_BLOB in_data, out_data;
  in_data.pbData = reinterpret_cast<LPBYTE>(buffer.get());
  in_data.cbData = length;
  if (!CryptUnprotectData(&in_data, NULL, NULL, NULL, NULL,
      CRYPTPROTECT_UI_FORBIDDEN, &out_data)) return E_FAIL;
  if (!out_data.cbData || !out_data.pbData) return E_FAIL;

  // The string may not null-terminated.
  wchar_t* decrypted = reinterpret_cast<wchar_t*>(out_data.pbData);
  length = out_data.cbData / sizeof(TCHAR);
  decrypted[length - 1] = 0;
  *value = decrypted;

  SecureZeroMemory(out_data.pbData, out_data.cbData);
  SecureZeroMemory(decrypted, out_data.cbData);
  ::LocalFree(out_data.pbData);
  return ERROR_SUCCESS;
}

LONG RegistryKey::SetEncryptedValue(const wstring& name, const wstring& value) {
  DATA_BLOB in_data, out_data;
  const wchar_t* tc = value.c_str();
  in_data.pbData = reinterpret_cast<BYTE*>(const_cast<TCHAR*>(tc));
  in_data.cbData = (static_cast<DWORD>(value.size()) + 1) * sizeof(wchar_t);

  // Encrypt, then save it as binary.
  if (CryptProtectData(&in_data, encrypt_description_.c_str(), NULL, NULL, NULL,
      0, &out_data) == TRUE) {
    LONG result = SetBinaryValue(name.c_str(),
                                 out_data.pbData,
                                 out_data.cbData);
    ::LocalFree(out_data.pbData);
    return result;
  }
  return E_FAIL;
}

bool RegistryKey::IsValueExisted(const wstring& name) {
  DWORD type = 0;
  ULONG length = 0;
  return QueryValue(name.c_str(), &type, NULL, &length) == ERROR_SUCCESS;
}

RegistryKey* RegistryKey::OpenKey(HKEY root,
                                  const wstring& key,
                                  REGSAM flags) {
  return OpenKey(root, key, flags, false);
}

RegistryKey* RegistryKey::OpenKey(HKEY root,
                                  const wstring& key,
                                  REGSAM flags,
                                  bool create_when_missing) {
  RegistryKey* registry = new RegistryKey();
  // Use 64bit registry by default.
  if (!(flags & KEY_WOW64_32KEY)) flags |= KEY_WOW64_64KEY;
  if (registry->Open(root, key.c_str(), flags) != ERROR_SUCCESS) {
    if (!create_when_missing ||
        registry->Create(root, key.c_str(),
                         REG_NONE,
                         REG_OPTION_NON_VOLATILE,
                         flags)
        != ERROR_SUCCESS) {
      delete registry;
      return NULL;
    }
  }
  return registry;
}

void RegistryKey::RecurseDeleteKey(
    HKEY root, const wstring& key, REGSAM flags) {
  CRegKey registry;
  // Use 64bit registry by default.
  if (!(flags & KEY_WOW64_32KEY)) flags |= KEY_WOW64_64KEY;
  if (registry.Open(root, NULL, KEY_READ | KEY_WRITE | flags) != ERROR_SUCCESS)
    return;

  registry.RecurseDeleteKey(key.c_str());
}

LONG RegistryKey::SetStringValueIfNotExisted(const wstring& name,
                                             const wstring& value,
                                             bool* existed,
                                             wstring* previous_value) {
  if (IsValueExisted(name)) {
    if (existed)
      *existed = true;
    if (previous_value) {
      return QueryStringValue(name, previous_value);
    }
  } else {
    SetStringValue(name.c_str(), value.c_str());
    if (existed)
      *existed = false;
  }
  return ERROR_SUCCESS;
}

bool RegistryKey::CreateAndSetStringValueIfNotExisted(
    HKEY root,
    const wstring& key,
    const wstring& name,
    const wstring& value,
    bool* existed,
    wstring* previous_value) {
  RegistryKey registry;
  LONG ret = ERROR_SUCCESS;
  ret = registry.Create(root, key.c_str(),
                        REG_NONE,
                        REG_OPTION_NON_VOLATILE,
                        KEY_READ | KEY_WRITE | KEY_WOW64_64KEY);
  if (ret != ERROR_SUCCESS)
    return false;
  ret = registry.SetStringValueIfNotExisted(name, value,
                                            existed, previous_value);
  if (ret != ERROR_SUCCESS)
    return false;
  return true;
}

}  // namespace ime_goopy
