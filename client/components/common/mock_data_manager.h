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

#ifndef GOOPY_COMPONENTS_COMMON_MOCK_DATA_MANAGER_H_
#define GOOPY_COMPONENTS_COMMON_MOCK_DATA_MANAGER_H_

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "i18n/input/engine/t13n/public/data_manager_interface.h"

namespace i18n_input {
namespace engine {
namespace t13n {
class CustomTokenDictionaryInterface;
class UserCacheInterface;
class UserDictionaryInterface;
class LanguageAlphabetInterface;
struct CanonicalSchemeLoadResult;
}  // namespace t13n
}  // namespace engine
}  // namespace i18n_input

namespace ime_goopy {
namespace components {

class MockDataManager : public i18n_input::engine::t13n::DataManagerInterface {
 public:
  MockDataManager();
  virtual ~MockDataManager();

  virtual bool EnrollData(const string& decoder_id,
                          const string& model_id,
                          const string& model_location) OVERRIDE;

  virtual bool EnrollDataByFileDescriptor(const string& decoder_id,
                                          const string& model_id,
                                          int file_descriptor,
                                          int offset,
                                          int length) OVERRIDE;

  virtual i18n_input::engine::t13n::CustomTokenDictionaryInterface*
    GetCustomTokenDictionary(const string& decoder_id) OVERRIDE;

  virtual i18n_input::engine::t13n::UserCacheInterface*
    GetUserCache(const string& decoder_id) OVERRIDE;

  virtual i18n_input::engine::t13n::UserDictionaryInterface*
    GetUserDictionary(const string& decoder_id) OVERRIDE;

  virtual const i18n_input::engine::t13n::LanguageAlphabetInterface*
    GetLanguageAlphabetForTargetLanguage(const string& decoder_id) OVERRIDE;

  virtual bool LoadCanonicalScheme(
      const string& decoder_id,
      const string& model_location,
      i18n_input::engine::t13n::CanonicalSchemeLoadResult* result) OVERRIDE;
};

}  // namespace components
}  // namespace ime_goopy

#endif  // GOOPY_COMPONENTS_COMMON_MOCK_DATA_MANAGER_H_
