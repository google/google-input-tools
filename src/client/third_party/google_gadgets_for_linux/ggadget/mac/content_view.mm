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

#import "ggadget/mac/content_view.h"

#include <Quartz/Quartz.h>
#include "ggadget/event.h"
#include "ggadget/graphics_interface.h"
#include "ggadget/mac/single_view_host.h"
#include "ggadget/mac/utils.h"

@implementation ContentView

- (void)drawRect:(NSRect)dirtyFrame {
  _onDrawSignal();
}

- (void)setFrame:(NSRect)frameRect {
  [super setFrame:frameRect];
  // Invalidate mouse event tracking area
  [self resetTrackingArea];
}

- (void)setCursor:(NSCursor*)cursor {
  if (_cursor == cursor) {
    return;
  }

  if (_isMouseEntered) {
    [_cursor pop];
    [cursor push];
  }
  _cursor = cursor;
}

- (void)resetTrackingArea {
  if (_trackingArea) {
    [self removeTrackingArea:_trackingArea];
  }
  NSUInteger options = NSTrackingMouseEnteredAndExited | NSTrackingMouseMoved |
    NSTrackingCursorUpdate | NSTrackingActiveAlways;
  _trackingArea = [[NSTrackingArea alloc] initWithRect:[self frame]
                                                options:options
                                                  owner:self
                                               userInfo:nil];
  [self addTrackingArea:_trackingArea];
}

- (void)mouseEntered:(NSEvent *)theEvent {
  _isMouseEntered = true;
  [_cursor push];
  _content->OnMouseEvent([self getMouseEvent:theEvent]);
}

- (void)mouseExited:(NSEvent *)theEvent {
  _isMouseEntered = false;
  [_cursor pop];
  _content->OnMouseEvent([self getMouseEvent:theEvent]);
}

- (void)mouseDown:(NSEvent *)theEvent {
  _mouseButtons |= ggadget::MouseEvent::BUTTON_LEFT;
  _content->OnMouseEvent([self getMouseEvent:theEvent]);

  NSPoint mouseLocation = [NSEvent mouseLocation];
  NSPoint windowLocation = [[self window] frame].origin;

  _mouseOffset.x = windowLocation.x - mouseLocation.x;
  _mouseOffset.y = windowLocation.y - mouseLocation.y;
}

- (void)mouseUp:(NSEvent *)theEvent {
  _mouseButtons &= ~ggadget::MouseEvent::BUTTON_LEFT;
  _content->OnMouseEvent([self getMouseEvent:theEvent]);
  _isMoving = false;
}

- (void)mouseMoved:(NSEvent *)theEvent {
  _content->OnMouseEvent([self getMouseEvent:theEvent]);
}

- (void)mouseDragged:(NSEvent *)theEvent {
  // TODO: dragging to resize
  if (!_isMoving) {
    ggadget::EventResult result =
        _content->OnMouseEvent([self getMouseEvent:theEvent]);
    if (result == ggadget::EVENT_RESULT_UNHANDLED) {
      [self beginMoveDrag];
    }
  }

  if (_isMoving) {
    NSPoint mousePos = [NSEvent mouseLocation];
    ggadget::mac::SingleViewHost *viewHost
        = ggadget::down_cast<ggadget::mac::SingleViewHost*>(
            _content->GetViewHost());
    viewHost->SetWindowPosition(mousePos.x + _mouseOffset.x,
                                mousePos.y + _mouseOffset.y);
  }
}

- (id)initWithFrame:(NSRect)frameRect
            content:(ggadget::ViewInterface*)content {
  self = [super initWithFrame:frameRect];
  if (self) {
    [self resetTrackingArea];

    _content = content;
    _mouseButtons = ggadget::MouseEvent::BUTTON_NONE;
  }
  return self;
}

- (id)initWithFrame:(NSRect)frameRect {
  @throw [NSException exceptionWithName:NSInternalInconsistencyException
                                 reason:@"-initWithFrame: is not allowed, use "
                                         "-initWithFrame:content:"
                               userInfo:nil];
  return nil;
}

- (ggadget::Connection*) connectOnEndMoveDrag:(ggadget::Slot2<void, int, int>*)slot {
  return _onEndMoveDragSignal.Connect(slot);
}

- (ggadget::Connection*) connectOnDraw:(ggadget::Slot0<void>*)slot {
  return _onDrawSignal.Connect(slot);
}

- (ggadget::MouseEvent)getMouseEvent:(NSEvent*)theEvent {
  int modifiers = ggadget::mac::GetModifiersFromEvent(theEvent);
  ggadget::Event::Type eventType;
  switch ([theEvent type]) {
    case NSLeftMouseDown:
    case NSRightMouseDown:
    case NSOtherMouseDown:
      eventType = ggadget::Event::EVENT_MOUSE_DOWN;
      break;
    case NSLeftMouseUp:
    case NSRightMouseUp:
    case NSOtherMouseUp:
      eventType = ggadget::Event::EVENT_MOUSE_UP;
      break;
    case NSMouseMoved:
    case NSLeftMouseDragged:
    case NSRightMouseDragged:
      eventType = ggadget::Event::EVENT_MOUSE_MOVE;
      break;
    case NSMouseEntered:
      eventType = ggadget::Event::EVENT_MOUSE_OVER;
      break;
    case NSMouseExited:
      eventType = ggadget::Event::EVENT_MOUSE_OUT;
      break;
    default:
      ASSERT_M(0, ("Unsupported event type"));
  }

  NSPoint pos = [self getMousePos:theEvent];
  return ggadget::MouseEvent(eventType,
                             pos.x, pos.y,
                             0, 0,
                             modifiers, _mouseButtons);
}

- (NSPoint)getMousePos:(NSEvent*)theEvent {
  NSPoint pos = [self convertPoint:[theEvent locationInWindow] fromView:nil];

  // The returned point has its origin at lower-left corner and y axis increases
  // upwards. We need to flip it vertically.
  pos.y = [self frame].size.height - pos.y;

  double zoom = _content->GetGraphics()->GetZoom();
  pos.x /= zoom;
  pos.y /= zoom;
  return pos;
}

- (void)beginMoveDrag {
  _isMoving = true;
}

@end
