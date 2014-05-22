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

#include "components/common/mock_decoder_factory.h"

#include "i18n/input/engine/t13n/public/decoder_interface.h"
#include "components/common/mock_data_manager.h"
#include "components/common/mock_decoder.h"

namespace {
using i18n_input::engine::t13n::DataManagerInterface;
using i18n_input::engine::t13n::DecoderInterface;
using i18n_input::engine::t13n::DecoderOptions;
}  // namespace

namespace ime_goopy {
namespace components {

MockDecoderFactory::MockDecoderFactory() {
  data_manager_.reset(new MockDataManager);
}

MockDecoderFactory::~MockDecoderFactory() {
}

DecoderInterface* MockDecoderFactory::CreateDecoder(
    const std::string& config, const DecoderOptions& option) {
  DecoderInterface* new_instance = new MockDecoder();
  return new_instance;
}

DataManagerInterface* MockDecoderFactory::GetDataManager() {
  return data_manager_.get();
}

bool MockDecoderFactory::GetModelIdList(const std::string& decoder_id,
                                        const DecoderOptions& option,
                                        std::vector<std::string>* model_ids) {
  return true;
}

}  // namespace components
}  // namespace ime_goopy
