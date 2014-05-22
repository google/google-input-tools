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

#include "ggadget/mac/content_window.h"

@implementation ContentWindow

-(void)setFocusable:(bool)focusable {
  _isFocusable = focusable;
}

-(bool)focusable {
  return _isFocusable;
}

-(void)setKeepAbove:(bool)keepAbove {
  if (keepAbove) {
    self.level = NSMainMenuWindowLevel;
  } else {
    self.level = NSNormalWindowLevel;
  }
}

-(bool)keepAbove {
  return self.level == NSMainMenuWindowLevel;
}

-(BOOL)canBecomeKeyWindow {
  return _isFocusable;
}

-(BOOL)canBecomeMainWindow {
  return _isFocusable;
}

-(id)init {
  self = [super init];
  self->_isFocusable = false;
  return self;
}

@end
