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

#include "ggadget/mac/cocoa_main_loop.h"

#include <sys/time.h>
#include <ctime>

#import <Cocoa/Cocoa.h>

namespace {

static const int kDefaultTimerCapacity = 20;

} // namespace

// Wraps a WatchCallbackInterface into an NSObject so that it can be called by
// NSTimer
@interface TimerCallaback : NSObject {
 @private
  ggadget::WatchCallbackInterface *_callback;
  ggadget::MainLoopInterface *_mainLoop;
  int _watchId;
}

-(id)initWithCallBack:(ggadget::WatchCallbackInterface*)callback
             mainLoop:(ggadget::MainLoopInterface*)mainLoop
              watchId:(int)watchId;
-(void)call:(NSTimer*)theTimer;
@end

@implementation TimerCallaback

-(id)initWithCallBack:(ggadget::WatchCallbackInterface *)callback
             mainLoop:(ggadget::MainLoopInterface*)mainLoop
              watchId:(int)watchId {
  self = [super init];
  _callback = callback;
  _mainLoop = mainLoop;
  _watchId = watchId;
  return self;
}

-(void)call:(NSTimer*)theTimer {
  bool ret = _callback->Call(_mainLoop, _watchId);
  if (!ret) {
    _mainLoop->RemoveWatch(_watchId);
  }
}

@end

namespace ggadget {
namespace mac {

class CocoaMainLoop::Impl {
 public:
  Impl() : timers_([NSMutableDictionary
                    dictionaryWithCapacity:kDefaultTimerCapacity]),
           max_callback_id_(0) {
  }

  NSMutableDictionary *timers_;
  int max_callback_id_;
};

CocoaMainLoop::CocoaMainLoop() : impl_(new Impl) {
  // Get NSApplication instance to guarantee the connection to display server.
  [NSApplication sharedApplication];
}

CocoaMainLoop::~CocoaMainLoop() {}

int CocoaMainLoop::AddIOReadWatch(int fd, WatchCallbackInterface *callback) {
  // Unsupported.
  return 0;
}

int CocoaMainLoop::AddIOWriteWatch(int fd, WatchCallbackInterface *callback) {
  // Unsupported.
  return 0;
}

int CocoaMainLoop::AddTimeoutWatch(int interval, WatchCallbackInterface *callback) {
  impl_->max_callback_id_++;
  NSRunLoop *main_loop = [NSRunLoop mainRunLoop];
  TimerCallaback *target =
      [[TimerCallaback alloc] initWithCallBack:callback
                                      mainLoop:this
                                       watchId:impl_->max_callback_id_];
  NSTimeInterval time_interval = static_cast<NSTimeInterval>(interval / 1000.0);
  NSTimer *timer = [NSTimer timerWithTimeInterval:time_interval
                                           target:target
                                         selector:@selector(call:)
                                         userInfo:nil
                                          repeats:YES];
  [impl_->timers_ setObject:timer
                     forKey:[NSNumber numberWithInt:impl_->max_callback_id_]];
  [main_loop addTimer:timer
              forMode:NSDefaultRunLoopMode];
  return impl_->max_callback_id_;
}

MainLoopInterface::WatchType CocoaMainLoop::GetWatchType(int watch_id) {
  return MainLoopInterface::INVALID_WATCH;
}

int CocoaMainLoop::GetWatchData(int watch_id) {
  NSNumber *key = [NSNumber numberWithInt:watch_id];
  NSTimer *timer = [impl_->timers_ objectForKey:key];
  if (!timer) {
    return -1;
  } else {
    return static_cast<int>([timer timeInterval] * 1000.0);
  }
}

void CocoaMainLoop::RemoveWatch(int watch_id) {
  NSNumber *key = [NSNumber numberWithInt:watch_id];
  NSTimer *timer = [impl_->timers_ objectForKey:key];
  if (!timer) {
    NSLog(@"watch %d doesn't exist", watch_id);
  } else {
    [timer invalidate];
    [impl_->timers_ removeObjectForKey:key];
  }
}

void CocoaMainLoop::Run() {
  NSApplication *app = [NSApplication sharedApplication];
  [app run];
}

bool CocoaMainLoop::DoIteration(bool may_block) {
  NSApplication *app = [NSApplication sharedApplication];
  NSRunLoop *main_loop = [NSRunLoop mainRunLoop];

  bool timer_fired = false;
  bool source_handled = false;

  do {
    CFRunLoopRef run_loop = [main_loop getCFRunLoop];
    CFAbsoluteTime t1 = CFRunLoopGetNextTimerFireDate(run_loop,
                                                      kCFRunLoopDefaultMode);
    SInt32 runResult = CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0, true);
    CFAbsoluteTime t2 = CFRunLoopGetNextTimerFireDate(run_loop,
                                                      kCFRunLoopDefaultMode);
    source_handled = runResult == kCFRunLoopRunHandledSource;
    timer_fired = fabs(t2 - t1) > 1e-6;
  } while (may_block && !source_handled && !timer_fired);
  return timer_fired;    // Only support timer watch
}

void CocoaMainLoop::Quit() {
  NSApplication *app = [NSApplication sharedApplication];
  [app stop:nil];
}

bool CocoaMainLoop::IsRunning() const {
  NSApplication *app = [NSApplication sharedApplication];
  return [app isRunning];
}

uint64_t CocoaMainLoop::GetCurrentTime() const {
  return static_cast<uint64_t>([[NSDate date] timeIntervalSince1970] * 1000);
}

bool CocoaMainLoop::IsMainThread() const {
  return [NSThread isMainThread];
}

void CocoaMainLoop::WakeUp() {
  // Not used.
}

} // namespace mac
} // namespace ggadget
