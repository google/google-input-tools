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
#include "common/clipboard.h"

#include "base/logging.h"

namespace {

// A compromised renderer could send us bad data, so validate it.
bool IsBitmapSafe(const Clipboard::ObjectMapParams& params) {
  const Clipboard::BitmapSize* size =
      reinterpret_cast<const Clipboard::BitmapSize*>(&(params[1].front()));
  return params[0].size() ==
      static_cast<size_t>(size->cx * size->cy * 4);
}

}

void Clipboard::DispatchObject(ObjectType type, const ObjectMapParams& params) {
  switch (type) {
    case CBF_TEXT:
      WriteText(&(params[0].front()), params[0].size());
      break;

#if defined(_WINDOWS)
    case CBF_BITMAP:
      if (!IsBitmapSafe(params))
        return;
      WriteBitmap(&(params[0].front()), &(params[1].front()));
      break;
#endif  // defined(OS_WIN) || defined(OS_LINUX)

    default:
      NOTREACHED();
  }
}
