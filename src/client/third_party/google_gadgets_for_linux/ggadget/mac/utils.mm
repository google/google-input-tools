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

#include "ggadget/mac/utils.h"

#include "ggadget/event.h"

namespace ggadget {
namespace mac{

int GetModifiersFromEvent(NSEvent *event) {
  int modifier = ggadget::Event::MODIFIER_NONE;
  NSUInteger event_flags = [event modifierFlags];
  if (event_flags & NSCommandKeyMask) {
    modifier |= ggadget::Event::MODIFIER_CONTROL;
  }
  if (event_flags & NSAlternateKeyMask) {
    modifier |= ggadget::Event::MODIFIER_ALT;
  }
  if (event_flags & NSShiftKeyMask) {
    modifier |= ggadget::Event::MODIFIER_SHIFT;
  }

  return modifier;
}

} // mac
} // ggadget
