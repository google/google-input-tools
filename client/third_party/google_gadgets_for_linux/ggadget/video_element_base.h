/*
  Copyright 2008 Google Inc.

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

#ifndef GGADGET_VIDEO_ELEMENT_BASE_H__
#define GGADGET_VIDEO_ELEMENT_BASE_H__

#include <string>
#include <ggadget/basic_element.h>

namespace ggadget {

template <typename R> class Slot0;
class View;

/**
 * @ingroup Elements
 * Base class for real video element implementation.
 * This class is platform-independent. Real video element should inherit
 * from this class, and implement all the pure virtual functions.
 */
class VideoElementBase : public BasicElement {
 public:
  DEFINE_CLASS_ID(0x7C5D2E793806427F, BasicElement);

  enum State {
    STATE_ERROR = -1,
    STATE_UNDEFINED = 0,
    STATE_STOPPED = 1,
    STATE_READY = 2,
    STATE_PAUSED = 3,
    STATE_PLAYING = 4,
    STATE_ENDED = 5
  };

  enum ErrorCode {
    ERROR_NO_ERROR = 0,
    ERROR_UNKNOWN = 1,
    ERROR_BAD_SRC = 2,
    ERROR_FORMAT_NOT_SUPPORTED = 3
  };

  enum TagType {
    TAG_AUTHOR,
    TAG_TITLE,
    TAG_ALBUM,
    TAG_DATE
  };

  //@{
  /** Ranges of balance and volume. */
  static const int kMinBalance = -10000;
  static const int kMaxBalance = 10000;
  static const int kMinVolume = -10000;
  static const int kMaxVolume = 0;
  //@}

 public:
  VideoElementBase(View *view, const char *tag_name, const char *name);
  virtual ~VideoElementBase();

  // Standard gadget APIs.

  /**
   * Checks whether the control is currently available, Possible controls
   * includes "play", "pause", "stop", "seek", "volume", "currentPosition",
   * "balance" and "mute".
   * Play backend in a state that doesn't allow the control, lacking support
   * because of the play backend or video property, are possible causes that
   * make some controls unavailable.
   * Returns true if the control avaiable.
   */
  virtual bool IsAvailable(const std::string &name) const;

  /** Starts playing the current media from the current position. */
  virtual void Play() = 0;

  /** Stops playing the current media, maintaining the current position. */
  virtual void Pause() = 0;

  /** Stops playing the current media, resetting the current position to 0. */
  virtual void Stop() = 0;

  /**
   * Gets the currentPosition property.
   * The property shows the current position within the video stream, in
   * seconds.
   */
  virtual double GetCurrentPosition() const = 0;
  /** Sets the currentPosition property. */
  virtual void SetCurrentPosition(double position) = 0;

  /**
   * Gets the length of the video. If no video data is available,
   * returns 0.
   */
  virtual double GetDuration() const = 0;

  /** Gets the error code that most recently reported. */
  virtual ErrorCode GetErrorCode() const = 0;

  /** Gets the play state of the video stream. */
  virtual State GetState() const = 0;

  /** Indicates whether the video is seekable. */
  virtual bool IsSeekable() const = 0;

  /** Gets the address of the video resource to playback. */
  virtual std::string GetSrc() const = 0;
  /**
   * Sets the address of the video resource to playback.
   * This attribute, if present, must contain a valid URL.
   */
  virtual void SetSrc(const std::string &src) = 0;

  /** Gets the volume. */
  virtual int GetVolume() const = 0;
  /** Sets the volume. */
  virtual void SetVolume(int volume) = 0;

  //@{
  /** Not standard APIs, but needed when hosted by object element. */
  virtual std::string GetTagInfo(TagType tag) const = 0;
  virtual int GetBalance() const = 0;
  virtual void SetBalance(int balance) = 0;
  virtual bool IsMute() const = 0;
  virtual void SetMute(bool mute) = 0;
  //@}

  Connection *ConnectOnStateChangeEvent(Slot0<void> *handler);
  Connection *ConnectOnMediaChangeEvent(Slot0<void> *handler);

  virtual void Layout();

 protected:
  /**
   * Register properties, methods, and signals. The real video element
   * doesn't need to do any registation, and should never call this function.
   */
  virtual void DoClassRegister();

  /**
   * Draw a video frame on the canvas @canvas.
   * The real video element should call PutImage to pass in the
   * metadata of an image frame that is ready to be shown. PutImage will do
   * a queue draw, and finally this function will be scheduled to actually
   * show the frame.
   * @see PutImage.
   */
  virtual void DoDraw(CanvasInterface *canvas);

  /**
   * The real video element should implement this so that it can change
   * the size of output video according to its parent element's new size.
   */
  virtual void SetGeometry(double width, double height) = 0;

  /**
   * The real video element should call this method to pass in the next
   * video frame. This function will also do a queuedraw.
   * @param data image buffer with format RGB24.
   * @param x and y start position to draw the image.
   * @param w width of the image.
   * @param h height of the image.
   * @param stride bytes per line for the image buffer (with pads).
   * @see DoDraw.
   * */
  bool PutImage(const char *data, int x, int y, int w, int h, int stride);

  /**
   * The real video element should call this method to clear the last image
   * frame so that it won't be shown any more.
   * Always be called when a playback is stopped.
   */
  void ClearImage();

  //@{
  /**
   * The real video element should fire these calls when corresponding
   * events occurs.
   */
  void FireOnStateChangeEvent();
  void FireOnMediaChangeEvent();
  //@}

 private:
  friend class ObjectVideoPlayer;
  DISALLOW_EVIL_CONSTRUCTORS(VideoElementBase);

  class Impl;
  Impl *impl_;
};

} // namespace ggadget

#endif // GGADGET_VIDEO_ELEMENT_BASE_H__
