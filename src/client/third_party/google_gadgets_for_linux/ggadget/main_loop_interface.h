/*
  Copyright 2011 Google Inc.

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

#ifndef GGADGET_MAIN_LOOP_INTERFACE_H__
#define GGADGET_MAIN_LOOP_INTERFACE_H__
#include <ggadget/common.h>
#include <ggadget/slot.h>

namespace ggadget {

class MainLoopInterface;

/**
 * @defgroup MainLoopInterface Main loop interfaces
 * @ingroup Interfaces
 * @{
 */

/**
 * This object is used by main loop to callback application
 * when there is any incoming event.
 *
 * This object can't inherit SmallObject<>, because it might be created in
 * multiple threads (curl_xml_http_request).
 */
class WatchCallbackInterface {
 public:
  virtual ~WatchCallbackInterface() { }

  /**
   * This function will be called by main loop when there is an interested
   * event available for processing.
   * The event could be an I/O read, I/O write or a timeout event.
   *
   * @param main_loop The main loop object which holds this watch.
   * @param watch_id ID of the watch.
   * @return If this function returns false, then the associated watch
   *         will be removed from the main loop.
   */
  virtual bool Call(MainLoopInterface *main_loop, int watch_id) = 0;

  /**
   * This function will be called by main loop when the watch is being removed
   * from the main loop.
   *
   * Application resources associated to the watch can be released in it.
   * The WatchCallback object itself will not be deleted by main loop, so if
   * necessary, it can be deleted here by calling "delete this;"
   *
   * Note: The watch will be removed from the main loop after calling this
   * method, so don't need to call main_loop->RemoveWatch() in it.
   * main loop methods: Run() and DoIteration() shouldn't be called in it.
   * Other main loop methods, such as RemoveWatch(), AddIOReadWatch(),
   * AddTimeoutWatch() etc. are ok.
   *
   * @param main_loop The main loop object which holds this watch.
   * @param watch_id ID of the watch to be removed.
   */
  virtual void OnRemove(MainLoopInterface *main_loop, int watch_id) = 0;
};

/**
 * A special WatchCallbackInterface implementation that calls a specified slot
 * when it got called.
 *
 * Then if you want to add a slot into main loop, you can just call:
 * main_loop->AddTimeoutWatch(interval, new WatchCallbackSlot(
 *                            NewSlot(this, &Foo::Bar)));
 *
 * The slot shall have one parameter which will be the unique id of the watch.
 * If the slot returns false then the watch will be removed from main loop.
 */
class WatchCallbackSlot : public WatchCallbackInterface {
 public:
  WatchCallbackSlot(Slot1<bool, int> *slot) : slot_(slot) { }
  virtual bool Call(MainLoopInterface *main_loop, int watch_id) {
    GGL_UNUSED(main_loop);
    if (slot_) return (*slot_)(watch_id);
    return false;
  }
  virtual void OnRemove(MainLoopInterface *main_loop, int watch_id) {
    GGL_UNUSED(main_loop);
    GGL_UNUSED(watch_id);
    delete slot_;
    delete this;
  }
 private:
  Slot1<bool, int> *slot_;
};

/**
 * Interface to the real main loop implementation.
 * For current implementation, no multiple threading support is required.
 */
class MainLoopInterface {
 protected:
  /**
   * Prevents from directly deleting the main loop instance.
   */
  virtual ~MainLoopInterface() { }

 public:
  /**
   * Possible types of main loop watch.
   * INVALID_WATCH means the watch is invalid.
   */
  enum WatchType {
    INVALID_WATCH = 0,
    IO_READ_WATCH,
    IO_WRITE_WATCH,
    TIMEOUT_WATCH
  };

  /**
   * Adds an I/O watch to the main loop, which watches over a file descriptor
   * for read event.
   *
   * callback is a WatchCallback object created by caller. Its Call method
   * will be called when fd becomes readable. Its Remove method will be
   * called when the watch is removed by calling RemoveWatch().
   * This callback object is owned by caller, main loop will never delete it.
   *
   * @param fd The fd to be watched.
   * @param callback The callback object associated to the newly created watch.
   * @return A watch id (greater than zero) which can be used to remove the
   *         watch later. Returns -1 upon failure.
   */
   virtual int AddIOReadWatch(int fd, WatchCallbackInterface *callback) = 0;

  /**
   * Adds an I/O watch to the main loop, which watches over a file descriptor
   * for write event.
   *
   * callback is a WatchCallback object created by caller. Its Call() method
   * will be called when fd becomes writable. Its OnRemove() method will be
   * called when the watch is removed by calling RemoveWatch().
   * This callback object is owned by caller, main loop will never delete it.
   *
   * @param fd The fd to be watched.
   * @param callback The callback object associated to the newly created watch.
   * @return A watch id (greater than zero) which can be used to remove the
   *         watch later. Returns -1 upon failure.
   */
  virtual int AddIOWriteWatch(int fd, WatchCallbackInterface *callback) = 0;

  /**
   * Adds a timeout watch to the main loop, which will be called at regular
   * intervals. The callback will be called repeatedly until it returns false,
   * then the watch will be removed from the main loop.
   *
   * @param interval Specifies the intervals to call the callback,
   *        in milliseconds.
   * @param callback The callback object associated to the newly created watch.
   *        It's owned by caller, main loop will never delete it.
   * @return A watch id (greater than zero) which can be used to remove the
   *         watch later. Returns -1 when fail.
   */
  virtual int AddTimeoutWatch(int interval, WatchCallbackInterface *callback)=0;

  /**
   * Returns the type of a watch. If the specified watch_id is invalid,
   * INVALID_WATCH will be returned.
   *
   * @param watch_id ID of the watch, must be greater than zero.
   * @return Type of the watch.
   */
  virtual WatchType GetWatchType(int watch_id) = 0;

  /**
   * Returns corresponding data of a watch.
   * For I/O read and write watch, it returns the file descriptor.
   * For timeout watch, it returns the interval.
   * If the watch_id is invalid, -1 will be returned.
   *
   * @param watch_id ID of the watch, must be greater than zero.
   * @return Interval value for timeout watch, or fd for IO watch,
   *         or -1 if the watch is invalid.
   */
  virtual int GetWatchData(int watch_id) = 0;

  /**
   * Removes a watch by its watch id.
   * The OnRemove() method of corresponding callback object will be called.
   * Nothing will happen if the watch_id is invalid.
   *
   * @param watch_id ID of the watch to be removed.
   */
  virtual void RemoveWatch(int watch_id) = 0;

  /**
   * Runs the main loop. It won't return until the loop is quitted by calling
   * Quit().
   * This method can be called multiple times recursively.
   */
  virtual void Run() = 0;

  /**
   * Runs a single iteration of the main loop.
   * It'll check to see if any event watches are ready to be processed.
   *
   * @param may_block Indicates if this function could block to wait for an
   *        event. If no event watches are ready and may_block is true,
   *        then waits for watches to become ready, then dispatches the
   *        watches that are ready. Note that even when may_block is true,
   *        it is still possible to return false, since the wait may be
   *        interrupted for other reasons than an event watch becoming ready.
   *        if may_block is false, then returns immediately if no event watches
   *        are ready.
   * @return true if one or more watch has been dispatched during this
   *         iteration.
   */
  virtual bool DoIteration(bool may_block) = 0;

  /**
   * Quits the main loop.
   * If the main loop is being run recursively, then only the innermost
   * invocation of Run() will be quitted.
   */
  virtual void Quit() = 0;

  /** Checks if a main loop is running.  */
  virtual bool IsRunning() const = 0;

  /**
   * Returns the current time in millisecond units since the Epoch
   * (00:00:00 UTC, January 1, 1970).
   */
  virtual uint64_t GetCurrentTime() const = 0;

  /** Checks if the calling thread is the main thread. */
  virtual bool IsMainThread() const = 0;

  /** Wakes up the main loop from another thread. */
  virtual void WakeUp() = 0;
};

/** @} */

/**
 * @relates MainLoopInterface
 * Sets a main loop instance as the global main loop, which can be used by any
 * components.
 *
 * This function must be called in main program at very beginning, and can only
 * be called once.
 */
bool SetGlobalMainLoop(MainLoopInterface *main_loop);

/**
 * @relates MainLoopInterface
 * Gets the global main loop instance set by calling SetGlobalMainLoop()
 * previously.
 */
MainLoopInterface *GetGlobalMainLoop();

} // namespace ggadget

#endif  // GGADGET_MAIN_LOOP_INTERFACE_H__
