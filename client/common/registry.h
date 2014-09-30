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
// A class to read / write data from Windows registry.
// Sample:
//   RegisterKey key;
//    key.Open(HKEY_CURRENT_USER, L"Software\\Google", true, false);
//    key.SetString(L"Key", L"Value");
//    wstring value;
//    key.GetString(L"Key", &value);

#ifndef GOOPY_COMMON_REGISTRY_H__
#define GOOPY_COMMON_REGISTRY_H__

#include <atlbase.h>
#include <windows.h>
#include <string>
#include <vector>

#include "base/basictypes.h"
#include "base/scoped_ptr.h"

namespace ime_goopy {
// A class to handle registry operations.
// We are using ATL's CRegKey as base class, but the string type is wstring,
// instead of ATL's CString, to satisfy old code.
class RegistryKey : public CRegKey {
 public:
  RegistryKey();
  ~RegistryKey();

  LONG QueryStringValue(const wstring& name, wstring* value);
  LONG QueryMultiStringValue(const wstring& name, vector<wstring>* values);
  LONG SetMultiStringValue(const wstring& name, const vector<wstring>& values);
  LONG QueryBinaryValue(const wstring& name,
                        scoped_array<uint8>* value,
                        ULONG* length);
  // Encrypted string is stored as binary data in registry, using windows's
  // encryption feature.
  LONG QueryEncryptedValue(const wstring& name, wstring* value);
  LONG SetEncryptedValue(const wstring& name, const wstring& value);
  bool IsValueExisted(const wstring& name);
  LONG EnumKey(uint32 index, wstring* name);
  // If name does not exist, sets it to value. Otherwise, outputs the previous
  // value via previous_value. existed is used to output the fact whether the
  // value already exists. existed or previous_value can be NULL to ignore the
  // outputs.
  LONG SetStringValueIfNotExisted(const wstring& name,
                                  const wstring& value,
                                  bool* existed,
                                  wstring* previous_value);

  void set_ecrypt_description(const wstring& value) {
    encrypt_description_ = value;
  }

  // These are two factory functions, the caller should delete returned objects.
  static RegistryKey* OpenKey(HKEY root, const wstring& key, REGSAM flags);
  static RegistryKey* OpenKey(HKEY root,
                              const wstring& key,
                              REGSAM flags,
                              bool create_when_missing);

  static void RecurseDeleteKey(HKEY root, const wstring& key, REGSAM flags);
  // A simple wrapper of Create() then SetStringValueIfNotExisted().
  static bool CreateAndSetStringValueIfNotExisted(HKEY root,
                                                  const wstring& key,
                                                  const wstring& name,
                                                  const wstring& value,
                                                  bool* existed,
                                                  wstring* previous_value);
 private:
  // Encrypt description will be displayed to user in some rare situation.
  wstring encrypt_description_;
  DISALLOW_EVIL_CONSTRUCTORS(RegistryKey);
};
}  // namespace ime_goopy
#endif  // GOOPY_COMMON_REGISTRY_H__
