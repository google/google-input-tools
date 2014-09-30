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

#ifndef GGADGET_MAC_CONTENT_VIEW_H_
#define GGADGET_MAC_CONTENT_VIEW_H_

#import <Cocoa/Cocoa.h>

#include "ggadget/scoped_ptr.h"
#include "ggadget/slot.h"
#include "ggadget/view.h"

// An |NSView| that holds, renders, and interacts with a |ViewInterface|. An
// instance of |ViewInterface| is called |content| in this class to avoid
// ambiguity.
@interface ContentView : NSView {
 @private
  ggadget::Signal0<void> _onDrawSignal;
  ggadget::Signal2<void, int, int> _onEndMoveDragSignal;
  ggadget::ViewInterface *_content;
  NSCursor *_cursor;
  NSTrackingArea *_trackingArea;
  bool _isMouseEntered;
  int _mouseButtons;
  bool _isMoving;
  NSPoint _mouseOffset;
}

// Sets the cursor of the view
- (void)setCursor:(NSCursor*)cursor;

// Initialize the view. Use this instead of |initWithFrame:|
- (id)initWithFrame:(NSRect)frameRect
            content:(ggadget::ViewInterface*)content;

- (ggadget::Connection*) connectOnEndMoveDrag:(ggadget::Slot2<void, int, int>*)slot;
- (ggadget::Connection*) connectOnDraw:(ggadget::Slot0<void>*)slot;

// Enable dragging to move. The left button should be pressed down to move.
- (void)beginMoveDrag;

@end

#endif // GGADGET_MAC_CONTENT_VIEW_H_
