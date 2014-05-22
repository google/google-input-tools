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
#ifndef GOOPY_COMPONENTS_WIN_FRONTEND_TEXT_STYLES_H_
#define GOOPY_COMPONENTS_WIN_FRONTEND_TEXT_STYLES_H_

#include <windows.h>
#include "common/framework_interface.h"

namespace ime_goopy {
namespace components {
enum StockTextStyle {
  STOCKSTYLE_NORMAL,
  STOCKSTYLE_UNDERLINED,
  STOCKSTYLE_COUNT
};

extern const TextStyle kStockTextStyles[];
}  // namespace components
}  // namespace ime_goopy

#endif  // GOOPY_COMPONENTS_WIN_FRONTEND_TEXT_STYLES_H_
