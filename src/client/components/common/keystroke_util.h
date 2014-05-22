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

#include "components/common/keystroke.h"
#include "ipc/protos/ipc.pb.h"

#ifndef GOOPY_COMPONENTS_COMMON_KEYSTROKE_UTIL_H_
#define GOOPY_COMPONENTS_COMMON_KEYSTROKE_UTIL_H_

namespace ime_goopy {
namespace components {

class KeyStrokeUtil {
 public:
  static KeyStroke ConstructKeyStroke(const ipc::proto::KeyEvent& src);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(KeyStrokeUtil);
};

}  // namespace components
}  // namespace ime_goopy

#endif  // GOOPY_COMPONENTS_COMMON_KEYSTROKE_UTIL_H_
