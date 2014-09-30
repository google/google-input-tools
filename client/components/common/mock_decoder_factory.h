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

#ifndef GOOPY_COMPONENTS_COMMON_MOCK_DECODER_FACTORY_H_
#define GOOPY_COMPONENTS_COMMON_MOCK_DECODER_FACTORY_H_

#include "i18n/input/engine/t13n/public/decoder_factory.h"
#include "i18n/input/engine/t13n/public/data_manager_interface.h"

namespace i18n_input {
namespace engine {
namespace t13n {
class DecoderInterface;
}  // namespace t13n
}  // namespace engine
}  // namespace i18n_input

namespace ime_goopy {
namespace components {

class MockDecoderFactory : public i18n_input::engine::t13n::DecoderFactory {
 public:
  MockDecoderFactory();
  virtual ~MockDecoderFactory();

  virtual i18n_input::engine::t13n::DecoderInterface* CreateDecoder(
      const string& decoder_id,
      const i18n_input::engine::t13n::DecoderOptions& options);

  virtual bool GetModelIdList(
      const string& decoder_id,
      const i18n_input::engine::t13n::DecoderOptions& options,
      std::vector<string>* model_ids);

  virtual i18n_input::engine::t13n::DataManagerInterface* GetDataManager();

 private:
  scoped_ptr<i18n_input::engine::t13n::DataManagerInterface> data_manager_;
  DISALLOW_COPY_AND_ASSIGN(MockDecoderFactory);
};
}  // namespace components
}  // namespace ime_goopy

#endif  // GOOPY_COMPONENTS_COMMON_MOCK_DECODER_FACTORY_H_
