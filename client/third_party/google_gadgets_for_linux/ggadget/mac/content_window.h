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
#ifndef GGADGET_MAC_CONTENT_WINDOW_H_
#define GGADGET_MAC_CONTENT_WINDOW_H_

#import <Cocoa/Cocoa.h>

// Extend NSWindow to provide the functionality to set whether the window is
// focusable and top-most
@interface ContentWindow : NSWindow {
 @private
  bool _isFocusable;
}

// Sets whether this window can become main window and key window.
// Note: this won't prevent the application from stealing focus from other
// applications. To prevent the app from stealing focus, set |LSBackgroundOnly|
// to 1 in the plist file.
@property bool focusable;
@property bool keepAbove;

@end

#endif // GGADGET_MAC_CONTENT_WINDOW_H_
