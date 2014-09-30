/*
  Copyright 2013 Google Inc.

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
#ifndef GGADGET_MAC_UTILS_H_
#define GGADGET_MAC_UTILS_H_

#import <Cocoa/Cocoa.h>

namespace ggadget {
namespace mac {

int GetModifiersFromEvent(NSEvent *event);

}  // mac
}  // ggadget

#endif  // GGADGET_MAC_UTILS_H_
