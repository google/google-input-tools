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

#ifndef GOOPY_COMPONENTS_COMMON_MOCK_DECODER_H_
#define GOOPY_COMPONENTS_COMMON_MOCK_DECODER_H_

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "i18n/input/engine/t13n/public/decoder_interface.h"

namespace ime_goopy {
namespace components {

class MockDecoder : public i18n_input::engine::t13n::DecoderInterface {
 public:
  MockDecoder();
  virtual ~MockDecoder();

  virtual void Decode(
      const i18n_input::engine::t13n::DecodeRequest& request,
      i18n_input::engine::t13n::DecodeResponse* response) const OVERRIDE;
 private:
  DISALLOW_COPY_AND_ASSIGN(MockDecoder);
};

}  // namespace components
}  // namespace ime_goopy

#endif  // GOOPY_COMPONENTS_COMMON_MOCK_DECODER_H_
