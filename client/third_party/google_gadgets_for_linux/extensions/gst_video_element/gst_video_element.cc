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

#include "gst_video_element.h"

#include <gst/gst.h>
#include <gst/video/gstvideosink.h>
#include <gst/gstinfo.h>
#include <string>

#include <ggadget/basic_element.h>
#include <ggadget/element_factory.h>
#include <ggadget/logger.h>
#include <ggadget/signals.h>
#include <ggadget/slot.h>
#include <ggadget/math_utils.h>
#include <ggadget/scriptable_interface.h>
#include <ggadget/scriptable_framework.h>
#include <ggadget/registerable_interface.h>

#include <extensions/gst_video_element/gadget_videosink.h>

#define Initialize gst_video_element_LTX_Initialize
#define Finalize gst_video_element_LTX_Finalize
#define RegisterElementExtension gst_video_element_LTX_RegisterElementExtension

extern "C" {
  bool Initialize() {
    LOGI("Initialize gst_video_element extension.");
    return true;
  }

  void Finalize() {
    LOGI("Finalize gst_video_element extension.");
  }

  bool RegisterElementExtension(ggadget::ElementFactory *factory) {
    LOGI("Register gst_video_element extension.");
    if (factory) {
      factory->RegisterElementClass(
          "video", &ggadget::gst::GstVideoElement::CreateInstance);
    }
    return true;
  }
}

namespace ggadget {
namespace gst {

#ifndef GGL_DEFAULT_GST_AUDIO_SINK
  #define GGL_DEFAULT_GST_AUDIO_SINK "autoaudiosink"
#endif

static const char *kGstAudioSinks[] = {
  GGL_DEFAULT_GST_AUDIO_SINK,
#if GGL_HOST_LINUX
  "alsasink",
  "osssink",
#endif
#if GGL_HOST_MACOSX
  "osxaudiosink",
#endif
#if GGL_HOST_WINDOWS
  "directsoundsink",
#endif
  NULL,
};

static double kMaxGstVolume = 4.0;

static const char *tag_strings[] = {
  GST_TAG_ARTIST,  // tagAuthor
  GST_TAG_TITLE,   // tagTitle
  GST_TAG_ALBUM,   // tagAlbum
  GST_TAG_DATE,    // tagDate
  NULL             // Others not supported yet.
};

static int g_video_element_count = 0;

GstVideoElement::GstVideoElement(View *view, const char *name)
    : VideoElementBase(view, "video", name),
      geometry_initialized_(false),
      playbin_(NULL),
      receive_image_handler_(NULL),
      tag_list_(NULL),
      media_changed_(false),
      local_state_(STATE_UNDEFINED),
      local_error_(ERROR_NO_ERROR) {
  // Initialize Gstreamer.
  gst_init(NULL, NULL);
  g_video_element_count++;

  // Register our video sink.
  if (!GadgetVideoSink::Register())
    return;

  playbin_ = gst_element_factory_make("playbin", "player");
  // Only do further initialize if playbin is created correctly.
  if (!playbin_) {
    LOG("Failed to create gstreamer playbin element.");
    return;
  }

  videosink_ = gst_element_factory_make(kGadgetVideoSinkElementName,
                                        "videosink");

  if (!videosink_) {
    LOG("Failed to create gadget_videosink element.");
    gst_object_unref(GST_OBJECT(playbin_));
    playbin_ = NULL;
    return;
  } else {
    g_object_get(G_OBJECT(videosink_),
                 "receive-image-handler", &receive_image_handler_, NULL);
    if (!receive_image_handler_) {
      gst_object_unref(GST_OBJECT(playbin_));
      gst_object_unref(GST_OBJECT(videosink_));
      playbin_ = NULL;
      return;
    }
  }

  // Set videosink to receive video output.
  g_object_set(G_OBJECT(playbin_), "video-sink", videosink_, NULL);

  // Create new audio sink with panorama support if possible.
  GstElement *audiosink = NULL;
  for (size_t i = 0; kGstAudioSinks[i]; ++i) {
    audiosink = gst_element_factory_make(kGstAudioSinks[i],
                                         "audiosink");
    if (audiosink) break;
  }

  if (!audiosink) {
    LOG("Failed to find a suitable gstreamer audiosink.");
    if (playbin_) gst_object_unref(GST_OBJECT(playbin_));
    playbin_ = NULL;
    return;
  }

  volume_ = gst_element_factory_make("volume", "mute");
  panorama_ = gst_element_factory_make("audiopanorama", "balance");

  // If volume or panorama is available then construct a new compound audiosink
  // with volume or panorama support.
  if (volume_ || panorama_) {
    GstElement *audiobin = gst_bin_new("audiobin");
    GstPad *sinkpad;
    if (volume_ && panorama_) {
      gst_bin_add_many(GST_BIN(audiobin), volume_, panorama_, audiosink, NULL);
      gst_element_link_many(volume_, panorama_, audiosink, NULL);
      sinkpad = gst_element_get_pad(volume_, "sink");
    } else if (volume_) {
      gst_bin_add_many(GST_BIN(audiobin), volume_, audiosink, NULL);
      gst_element_link(volume_, audiosink);
      sinkpad = gst_element_get_pad(volume_, "sink");
    } else {
      gst_bin_add_many(GST_BIN(audiobin), panorama_, audiosink, NULL);
      gst_element_link(panorama_, audiosink);
      sinkpad = gst_element_get_pad(panorama_, "sink");
    }
    gst_element_add_pad(audiobin, gst_ghost_pad_new("sink", sinkpad));
    gst_object_unref(GST_OBJECT(sinkpad));
    audiosink = audiobin;
  }

  // Set audio-sink to our new audiosink.
  g_object_set(G_OBJECT(playbin_), "audio-sink", audiosink, NULL);

  // Watch the message bus.
  // The host using this class must use a g_main_loop to capture the
  // message in the default context.
  GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(playbin_));
  gst_bus_add_watch(bus, OnNewMessage, this);
  gst_object_unref(bus);

  // We are ready to play.
  local_state_ = STATE_READY;
}

GstVideoElement::~GstVideoElement() {
  if (playbin_) {
    SetPlayState(GST_STATE_NULL);
    gst_object_unref(GST_OBJECT(playbin_));
    playbin_ = NULL;
    videosink_ = NULL;
    panorama_ = NULL;
  }
  if (tag_list_) {
    gst_tag_list_free(tag_list_);
    tag_list_ = NULL;
  }
  if (--g_video_element_count == 0)
    gst_deinit();
}

BasicElement *GstVideoElement::CreateInstance(View *view, const char *name) {
  return new GstVideoElement(view, name);
}

bool GstVideoElement::IsAvailable(const std::string &name) {
  if (VideoElementBase::IsAvailable(name))
    return true;

  if (name == "volume") {
    if (playbin_)
      return true;
  } else if (name == "balance") {
    if (playbin_ && panorama_)
      return true;
  } else if (name == "mute") {
    if (playbin_ && volume_)
      return true;
  }

  return false;
}

void GstVideoElement::Play() {
  if (!geometry_initialized_) {
    // Initialize the geometry. Cannot do this in the constructor, as at that
    // time, the element has not been created and its size has not been set yet.
    SetGeometry(GetPixelWidth(), GetPixelHeight());
    geometry_initialized_ = true;
  }
  if (playbin_ && src_.length()) {
    if (SetPlayState(GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE)
      LOGE("Failed to play the media.");
  } else {
    if (!playbin_)
      DLOG("Playbin was not initialized correctly.");
    else
      LOG("No media source.");
  }
}

void GstVideoElement::Pause() {
  if (playbin_ && local_state_ == STATE_PLAYING &&
      SetPlayState(GST_STATE_PAUSED) == GST_STATE_CHANGE_FAILURE)
    LOGE("Failed to pause the media.");
}

void GstVideoElement::StopInternal(bool fire_state_change) {
  if (playbin_ && (local_state_ == STATE_PLAYING ||
                   local_state_ == STATE_PAUSED ||
                   local_state_ == STATE_ENDED)) {
    if (SetPlayState(GST_STATE_NULL) == GST_STATE_CHANGE_FAILURE) {
      LOGE("Failed to stop the media.");
    } else if (fire_state_change && local_state_ != STATE_ERROR) {
      // Playbin won't post "STATE CHANGED" message when being set to
      // "NULL" state. We make a state-change scene manually.
      // But We don't clear any ERROR sign, let it be there until gstreamer
      // itself changes its state.
      local_state_ = STATE_STOPPED;
      FireOnStateChangeEvent();
    }

    SetCurrentPositionInternal(0);
    // Clear the last image frame.
    ClearImage();
  }
}

void GstVideoElement::Stop() {
  StopInternal(true);
}

double GstVideoElement::GetCurrentPosition() const {
  if (playbin_ && (local_state_ == STATE_PLAYING ||
                   local_state_ == STATE_PAUSED)) {
    gint64 position;
    GstFormat format = GST_FORMAT_TIME;

    if (gst_element_query_position(playbin_, &format, &position)) {
      return static_cast<double>(position) / GST_SECOND;
    }
  }
  return 0;
}

void GstVideoElement::SetCurrentPositionInternal(double position) {
  gst_element_seek(playbin_, 1.0, GST_FORMAT_TIME,
                   static_cast<GstSeekFlags>(GST_SEEK_FLAG_FLUSH |
                                             GST_SEEK_FLAG_KEY_UNIT),
                   GST_SEEK_TYPE_SET,
                   static_cast<gint64>(position) * GST_SECOND,
                   GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE);
}

void GstVideoElement::SetCurrentPosition(double position) {
  // Seek will only be successful under PAUSED or PLAYING state.
  // It's ok to check local state.
  if (playbin_ &&
      (local_state_ == STATE_PLAYING || local_state_ == STATE_PAUSED))
    SetCurrentPositionInternal(position);
}

double GstVideoElement::GetDuration() const {
  if (playbin_) {
    gint64 duration;
    GstFormat format = GST_FORMAT_TIME;
    if (gst_element_query_duration(playbin_, &format, &duration) &&
        format == GST_FORMAT_TIME)
    return static_cast<double>(duration) / GST_SECOND;
  }
  return 0;
}

GstVideoElement::ErrorCode GstVideoElement::GetErrorCode() const {
  return local_error_;
}

GstVideoElement::State GstVideoElement::GetState() const {
  return local_state_;
}

bool GstVideoElement::IsSeekable() const {
  GstQuery *query;
  gboolean res;
  gboolean seekable = FALSE;

  query = gst_query_new_seeking(GST_FORMAT_TIME);
  res = gst_element_query(playbin_, query);
  if (res) {
    gst_query_parse_seeking(query, NULL, &seekable, NULL, NULL);
  }
  gst_query_unref(query);

  return seekable == TRUE;
}

std::string GstVideoElement::GetSrc() const {
  return src_;
}

void GstVideoElement::SetSrc(const std::string &src) {
  if (src_ != src) {
    // Empty the tag cache when loading a new media.
    if (tag_list_) {
      gst_tag_list_free(tag_list_);
      tag_list_ = NULL;
    }

    src_ = src;
    media_changed_ = true;
    g_object_set(G_OBJECT(playbin_), "uri", src_.c_str(), NULL);
  }
}

int GstVideoElement::GetVolume() const {
  if (playbin_) {
    double volume;
    g_object_get(G_OBJECT(playbin_), "volume", &volume, NULL);
    int gg_volume = static_cast<int>((volume / kMaxGstVolume) *
                                     (kMaxVolume - kMinVolume) + kMinVolume);
    return Clamp(gg_volume, kMinVolume, kMaxVolume);
  }
  DLOG("Playbin was not initialized correctly.");
  return kMinVolume;
}

void GstVideoElement::SetVolume(int volume) {
  if (playbin_) {
    if (volume < kMinVolume || volume > kMaxVolume) {
      LOG("Invalid volume value, range: [%d, %d].", kMinVolume, kMaxVolume);
      volume = Clamp(volume, kMinVolume, kMaxVolume);
    }
    gdouble gg_volume = ((gdouble(volume - kMinVolume) /
                          (kMaxVolume - kMinVolume)) * kMaxGstVolume);
    g_object_set(G_OBJECT(playbin_), "volume", gg_volume, NULL);
  } else {
    DLOG("Playbin was not initialized correctly.");
  }
}

std::string GstVideoElement::GetTagInfo(TagType tag) const {
  gchar *info;
  const char *tag_string = tag_strings[tag];
  if (tag_list_ && tag_string &&
      gst_tag_list_get_string(tag_list_, tag_string, &info)) {
    std::string s(info);
    delete info;
    return s;
  } else {
    return "";
  }
}

int GstVideoElement::GetBalance() const {
  if (playbin_ && panorama_) {
    gfloat balance;
    g_object_get(G_OBJECT(panorama_), "panorama", &balance, NULL);
    int gg_balance = static_cast<int>(((balance + 1) / 2) *
                                      (kMaxBalance - kMinBalance) +
                                      kMinBalance);
    return Clamp(gg_balance, kMinBalance, kMaxBalance);
  }

  if (!playbin_)
    DLOG("Playbin was not initialized correctly.");
  else
    DLOG("Balance is not supported.");

  return (kMaxBalance + kMinBalance) / 2;
}

void GstVideoElement::SetBalance(int balance) {
  if (playbin_ && panorama_) {
    if (balance < kMinBalance || balance > kMaxBalance) {
      LOG("Invalid balance value, range: [%d, %d].", kMinBalance, kMaxBalance);
      balance = Clamp(balance, kMinBalance, kMaxBalance);
    }
    gfloat gg_balance = (gfloat(balance - kMinBalance) /
                          (kMaxBalance - kMinBalance)) * 2 - 1;
    g_object_set(G_OBJECT(panorama_), "panorama", gg_balance, NULL);
  } else {
    if (!playbin_)
      DLOG("Playbin was not initialized correctly.");
    else
      DLOG("Balance is not supported.");
  }
}

bool GstVideoElement::IsMute() const {
  if (playbin_ && volume_) {
    gboolean mute;
    g_object_get(G_OBJECT(volume_), "mute", &mute, NULL);
    return static_cast<bool>(mute);
  } else {
    if (!playbin_)
      DLOG("Playbin was not initialized correctly.");
    else
      DLOG("Mute is not supported.");
    return false;
  }
}

void GstVideoElement::SetMute(bool mute) {
  if (playbin_ && volume_) {
    g_object_set(G_OBJECT(volume_), "mute", static_cast<gboolean>(mute), NULL);
  } else {
    if (!playbin_)
      DLOG("Playbin was not initialized correctly.");
    else
      DLOG("Mute is not supported.");
  }
}

void GstVideoElement::SetGeometry(double width, double height) {
  if (playbin_ && videosink_) {
    g_object_set(G_OBJECT(videosink_),
                 "geometry-width", static_cast<int>(width),
                 "geometry-height", static_cast<int>(height), NULL);
  } else {
    if (!playbin_)
      DLOG("Playbin was not initialized correctly.");
    else
      DLOG("videosink was not initialized correctly.");
  }
}

GstVideoElement::State
GstVideoElement::GstStateToLocalState(GstState state) {
  switch (state) {
    case GST_STATE_NULL:
      return STATE_STOPPED;
    case GST_STATE_READY:
      return STATE_READY;
    case GST_STATE_PAUSED:
      return STATE_PAUSED;
    case GST_STATE_PLAYING:
      return STATE_PLAYING;
    default:
      return STATE_UNDEFINED;
  }
}

gboolean GstVideoElement::OnNewMessage(GstBus *bus,
                                       GstMessage *msg,
                                       gpointer data) {
  GGL_UNUSED(bus);
  ASSERT(msg && data);
  GstVideoElement *object = static_cast<GstVideoElement*>(data);

  switch (GST_MESSAGE_TYPE(msg)) {
    case GST_MESSAGE_ERROR:
      object->OnError(msg);
      break;
    case GST_MESSAGE_EOS:
      object->OnMediaEnded();
      break;
    case GST_MESSAGE_STATE_CHANGED:
      object->OnStateChange(msg);
      break;
    case GST_MESSAGE_ELEMENT:
      object->OnElementMessage(msg);
      break;
    case GST_MESSAGE_TAG:
      object->OnTagInfo(msg);
      break;
    default:
      break;
  }

  return true;
}

void GstVideoElement::OnError(GstMessage *msg) {
  ASSERT(msg);
  GError *gerror;
  gchar *debug;

  gst_message_parse_error(msg, &gerror, &debug);
  DLOG("GstVideoElement OnError: domain=%d code=%d message=%s debug=%s",
       gerror->domain, gerror->code, gerror->message, debug);

  if (gerror->domain == GST_RESOURCE_ERROR &&
      (gerror->code == GST_RESOURCE_ERROR_NOT_FOUND ||
       gerror->code == GST_RESOURCE_ERROR_OPEN_READ ||
       gerror->code == GST_RESOURCE_ERROR_OPEN_READ_WRITE)) {
    local_error_ = ERROR_BAD_SRC;
  } else if (gerror->domain == GST_STREAM_ERROR &&
             (gerror->code == GST_STREAM_ERROR_NOT_IMPLEMENTED ||
              gerror->code == GST_STREAM_ERROR_TYPE_NOT_FOUND ||
              gerror->code == GST_STREAM_ERROR_WRONG_TYPE ||
              gerror->code == GST_STREAM_ERROR_CODEC_NOT_FOUND ||
              gerror->code == GST_STREAM_ERROR_FORMAT)) {
    local_error_ = ERROR_FORMAT_NOT_SUPPORTED;
  } else {
    local_error_ = ERROR_UNKNOWN;
  }

  local_state_ = STATE_ERROR;
  FireOnStateChangeEvent();

  g_error_free(gerror);
  g_free(debug);
}

void GstVideoElement::OnMediaEnded() {
  StopInternal(false);
  local_state_ = STATE_ENDED;
  State save_state = local_state_;
  FireOnStateChangeEvent();
  if (local_state_ == save_state) {
    local_state_ = STATE_READY;
    FireOnStateChangeEvent();
  }
}

void GstVideoElement::OnStateChange(GstMessage *msg) {
  ASSERT(msg);

  GstState old_state, new_state;
  gst_message_parse_state_changed(msg, &old_state, &new_state, NULL);

  State state = GstStateToLocalState(new_state);
  if (state == STATE_PLAYING) {
    // If any change-event is waiting, we invoke it here as the state of
    // the media stream actually changed.
    if (media_changed_) {
      FireOnMediaChangeEvent();
      media_changed_ = false;
    }
  } else if (state == STATE_UNDEFINED || state == STATE_ERROR) {
    media_changed_ = false;
  }

  if (local_state_ != state) {
    local_state_ = state;
    FireOnStateChangeEvent();
  }
}

void GstVideoElement::OnElementMessage(GstMessage *msg) {
  ASSERT(msg);
  if (GST_MESSAGE_SRC(msg) == reinterpret_cast<GstObject*>(videosink_)) {
    const GstStructure *structure = gst_message_get_structure(msg);
    const GValue* gvalue = gst_structure_get_value(structure,
                                                   kGadgetVideoSinkMessageName);
    GadgetVideoSink::MessageType type =
        static_cast<GadgetVideoSink::MessageType>(g_value_get_int(gvalue));
    if (type == GadgetVideoSink::NEW_IMAGE) {
      ASSERT(receive_image_handler_);
      GadgetVideoSink::Image *image = (*receive_image_handler_)(videosink_);
      if (image) {
        PutImage(image->data,
                 image->x, image->y, image->w, image->h, image->stride);
      }
    }
  }
}

void GstVideoElement::OnTagInfo(GstMessage *msg) {
  ASSERT(msg);
  GstTagList *new_tag_list;

  gst_message_parse_tag(msg, &new_tag_list);
  if (new_tag_list) {
    tag_list_ = gst_tag_list_merge(tag_list_, new_tag_list,
                                   GST_TAG_MERGE_PREPEND);
  }
}

GstStateChangeReturn GstVideoElement::SetPlayState(GstState state) {
  GstStateChangeReturn result = gst_element_set_state(playbin_, state);
  return result;
}

} // namespace gst
} // namespace ggadget
