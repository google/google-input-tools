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

#ifndef GGADGET_AUDIOCLIP_INTERFACE_H__
#define GGADGET_AUDIOCLIP_INTERFACE_H__

#include <string>
#include <ggadget/small_object.h>

namespace ggadget {

class Connection;
template <typename R, typename P1> class Slot1;

namespace framework {

/**
 * @ingroup FrameworkInterfaces
 * Class for playing back audio files.
 */
class AudioclipInterface : public SmallObject<> {
 protected:
  virtual ~AudioclipInterface() {}

 public:
  enum State {
    SOUND_STATE_ERROR = -1,
    SOUND_STATE_STOPPED = 0,
    SOUND_STATE_PLAYING = 1,
    SOUND_STATE_PAUSED = 2,
  };

  enum ErrorCode {
    SOUND_ERROR_NO_ERROR = 0,
    SOUND_ERROR_UNKNOWN = 1,
    SOUND_ERROR_BAD_CLIP_SRC = 2,
    SOUND_ERROR_FORMAT_NOT_SUPPORTED = 3,
  };

  static const int  kMinBalance = -10000;
  static const int  kMaxBalance = 10000;

  static const int  kMinVolume = -10000;
  static const int  kMaxVolume = 0;

 public:
  virtual void Destroy() = 0;

 public:
  /**
   * Get the audio signal balance.
   * A number between -10000 ~ 10000.
   * -10000 means that only the left audio channel can be heard;
   * 10000 means that only the right audio channel can be heard.
   */
  virtual int GetBalance() const = 0;
  /**
   * Set the audio signal balance.
   * @see GetBalance().
   */
  virtual void SetBalance(int balance) = 0;
  /**
   * Get the current position within the audio clip.
   * Where 0 represents the beginning of the clip and @c duration is the
   * end + 1.
   */
  virtual int GetCurrentPosition() const = 0;
  /**
   * Set the current position within the audio clip.
   * @see GetCurrentPosition().
   */
  virtual void SetCurrentPosition(int position) = 0;
  /**
   * The length, in seconds, of the sound.
   */
  virtual int GetDuration() const = 0;
  virtual ErrorCode GetError() const = 0;
  virtual std::string GetSrc() const = 0;
  virtual void SetSrc(const char *src) = 0;
  virtual State GetState() const = 0;
  virtual int GetVolume() const = 0;
  virtual void SetVolume(int volume) = 0;

 public:
  virtual void Play() = 0;
  virtual void Pause() = 0;
  virtual void Stop() = 0;

 public:
  typedef Slot1<void, State> OnStateChangeHandler;
  virtual Connection *ConnectOnStateChange(OnStateChangeHandler *handler) = 0;
};

} // namespace framework

} // namespace ggadget

#endif // GGADGET_AUDIOCLIP_INTERFACE_H__
