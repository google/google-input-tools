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

#ifndef EXTENSIONS_GST_VIDEO_ELEMENT_H__
#define EXTENSIONS_GST_VIDEO_ELEMENT_H__

#include <gst/gst.h>
#include <string>

#include <ggadget/view.h>
#include <ggadget/basic_element.h>
#include <ggadget/video_element_base.h>
#include "gadget_videosink.h"

namespace ggadget {
namespace gst {

/**
 * Implements video element based on gstreamer.
 * Any thread using this implementation must run in the default
 * g_main_loop context.
 */
class GstVideoElement: public VideoElementBase {
 public:
  DEFINE_CLASS_ID(0xc67e3d7bbc7283a9, VideoElementBase);

  GstVideoElement(View *view, const char *name);
  virtual ~GstVideoElement();

 public:
  static BasicElement *CreateInstance(View *view, const char *name);

  virtual bool IsAvailable(const std::string& name);

  virtual void Play();
  virtual void Pause();
  virtual void Stop();

  virtual double GetCurrentPosition() const;
  virtual void SetCurrentPosition(double position);

  virtual double GetDuration() const;
  virtual ErrorCode GetErrorCode() const;
  virtual State GetState() const;
  virtual bool IsSeekable() const;

  virtual std::string GetSrc() const;
  virtual void SetSrc(const std::string &src);

  virtual int GetVolume() const;
  virtual void SetVolume(int volume);

  virtual std::string GetTagInfo(TagType tag) const;
  virtual int GetBalance() const;
  virtual void SetBalance(int balance);
  virtual bool IsMute() const;
  virtual void SetMute(bool mute);

 protected:
  virtual void SetGeometry(double width, double height);

 private:
  static State GstStateToLocalState(GstState state);
  static gboolean OnNewMessage(GstBus *bus, GstMessage *msg, gpointer data);

  void OnError(GstMessage *msg);
  void OnMediaEnded();
  void OnStateChange(GstMessage *msg);
  void OnElementMessage(GstMessage *msg);
  void OnTagInfo(GstMessage *msg);

 private:
  DISALLOW_EVIL_CONSTRUCTORS(GstVideoElement);

  void SetCurrentPositionInternal(double position);
  void StopInternal(bool fire_state_change);
  GstStateChangeReturn SetPlayState(GstState state);

  std::string src_;  // Media source to play.

  bool geometry_initialized_;

  GstElement *playbin_;
  GstElement *videosink_;
  GstElement *volume_;   // Mute control.
  GstElement *panorama_; // Balance control.

  GadgetVideoSink::Image* (*receive_image_handler_)(GstElement*);

  GstTagList *tag_list_;

  bool media_changed_;
  State local_state_;
  ErrorCode local_error_;
};

} // namespace gst
} // namespace ggadget

#endif // EXTENSIONS_GST_VIDEO_ELEMENT_H__
