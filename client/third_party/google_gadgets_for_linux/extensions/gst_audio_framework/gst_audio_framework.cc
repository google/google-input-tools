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

#include <gst/gst.h>
#include <string>
#include <ggadget/logger.h>
#include <ggadget/audioclip_interface.h>
#include <ggadget/framework_interface.h>
#include <ggadget/slot.h>
#include <ggadget/signals.h>
#include <ggadget/math_utils.h>
#include <ggadget/scriptable_interface.h>
#include <ggadget/scriptable_framework.h>
#include <ggadget/registerable_interface.h>

#ifndef GGL_DEFAULT_GST_AUDIO_SINK
  #define GGL_DEFAULT_GST_AUDIO_SINK "autoaudiosink"
#endif

#define Initialize gst_audio_framework_LTX_Initialize
#define Finalize gst_audio_framework_LTX_Finalize
#define RegisterFrameworkExtension \
    gst_audio_framework_LTX_RegisterFrameworkExtension

namespace ggadget {
namespace framework {

// To avoid naming conflicts.
namespace gst_audio {

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
static bool g_gst_init_ok_ = false;

/**
 * Gstreamer based implementation of @c Audioclip for playing back audio files.
 * Users who is to use this class should be single-threaded for safty,
 * and must run in the default g_main_loop context.
 */
class GstAudioclip : public AudioclipInterface {
 public:
  GstAudioclip(const char *src)
    : playbin_(NULL),
      panorama_(NULL),
      local_state_(SOUND_STATE_ERROR),
      local_error_(SOUND_ERROR_NO_ERROR),
      gst_state_(GST_STATE_VOID_PENDING) {
    playbin_ = gst_element_factory_make("playbin", "player");
    GstElement *videosink = gst_element_factory_make("fakesink", "fakevideo");

    // Only do further initialize if playbin is created correctly.
    if (!playbin_) {
      LOG("Failed to create gstreamer playbin element.");
      return;
    }

    if (!videosink) {
      LOG("Failed to create gstreamer fakesink element.");
      gst_object_unref(GST_OBJECT(playbin_));
      playbin_ = NULL;
      return;
    }

    // Use fakesink as video-sink to discard the video output.
    g_object_set(G_OBJECT(playbin_), "video-sink", videosink, NULL);

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

    panorama_ = gst_element_factory_make("audiopanorama", "panorama");

    // If panorama is available then construct a new compound audiosink with
    // panorama support.
    if (panorama_) {
      GstElement *audiobin = gst_bin_new("audiobin");
      gst_bin_add_many(GST_BIN(audiobin), panorama_, audiosink, NULL);
      gst_element_link(panorama_, audiosink);
      GstPad *sinkpad = gst_element_get_pad(panorama_, "sink");
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
    local_state_ = SOUND_STATE_STOPPED;
    if (src && *src) SetSrc(src);
  }

  virtual ~GstAudioclip() {
    if (playbin_) {
      gst_element_set_state(playbin_, GST_STATE_NULL);
      gst_object_unref(GST_OBJECT(playbin_));
      playbin_ = NULL;
      panorama_ = NULL;
    }
  }

  virtual void Destroy() {
    delete this;
  }

  virtual int GetBalance() const {
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

  virtual void SetBalance(int balance) {
    if (playbin_ && panorama_) {
      if (balance < kMinBalance || balance > kMaxBalance) {
        LOG("Invalid balance value, range: [%d, %d].",
            kMinBalance, kMaxBalance);
        balance = Clamp(balance, kMinBalance, kMaxBalance);
      }
      gfloat gst_balance =
        (gfloat(balance - kMinBalance) / (kMaxBalance - kMinBalance)) * 2 - 1;
      g_object_set(G_OBJECT(panorama_), "panorama", gst_balance, NULL);
    } else {
      if (!playbin_)
        DLOG("Playbin was not initialized correctly.");
      else
        DLOG("Balance is not supported.");
    }
  }

  virtual int GetCurrentPosition() const {
    if (playbin_ && local_state_ != SOUND_STATE_ERROR) {
      gint64 position;
      GstFormat format = GST_FORMAT_TIME;
      if (gst_element_query_position(playbin_, &format, &position) &&
          format == GST_FORMAT_TIME) {
        return static_cast<int>(position / GST_SECOND);
      }
    }
    return 0;
  }

  virtual void SetCurrentPosition(int position) {
    if (playbin_ && local_state_ != SOUND_STATE_ERROR) {
      gst_element_seek(playbin_, 1.0, GST_FORMAT_TIME,
                       static_cast<GstSeekFlags>(GST_SEEK_FLAG_FLUSH |
                                                 GST_SEEK_FLAG_KEY_UNIT),
                       GST_SEEK_TYPE_CUR,
                       static_cast<gint64>(position) * GST_SECOND,
                       static_cast<GstSeekType>(GST_SEEK_TYPE_NONE),
                       static_cast<gint64>(0));
    }
  }

  virtual int GetDuration() const {
    if (playbin_ && local_state_ != SOUND_STATE_ERROR) {
      gint64 duration;
      GstFormat format = GST_FORMAT_TIME;
      if (gst_element_query_duration(playbin_, &format, &duration) &&
          format == GST_FORMAT_TIME)
      return static_cast<int>(duration / GST_SECOND);
    }
    return 0;
  }

  virtual ErrorCode GetError() const {
    return local_error_;
  }

  virtual std::string GetSrc() const {
    return src_;
  }

  virtual void SetSrc(const char *src) {
    if (playbin_ && src && *src) {
      local_state_ = SOUND_STATE_STOPPED;
      local_error_ = SOUND_ERROR_NO_ERROR;
      src_ = std::string(src);
      // FIXME:
      // Playbin won't produce ERROR whether it's a bad uri or the file's
      // format is not supported. We must check here.
      g_object_set(G_OBJECT(playbin_), "uri", src, NULL);
    } else {
      if (!playbin_)
        DLOG("Playbin was not initialized correctly.");
      else
        DLOG("Invalid audio src.");
    }
  }

  virtual State GetState() const {
    return local_state_;
  }

  virtual int GetVolume() const {
    if (playbin_) {
      double volume;
      g_object_get(G_OBJECT(playbin_), "volume", &volume, NULL);
      int gg_volume = static_cast<int>((volume / kMaxGstVolume) *
                                        (kMaxVolume - kMinVolume) +
                                        kMinVolume);
      return Clamp(gg_volume, kMinVolume, kMaxVolume);
    }
    DLOG("Playbin was not initialized correctly.");
    return kMinVolume;
  }

  virtual void SetVolume(int volume) {
    if (playbin_) {
      if (volume < kMinVolume || volume > kMaxVolume) {
        LOG("Invalid volume value, range: [%d, %d].",
            kMinVolume, kMaxVolume);
        volume = Clamp(volume, kMinVolume, kMaxVolume);
      }

      gdouble gst_volume =
        (gdouble(volume - kMinVolume) / (kMaxVolume - kMinVolume)) *
        kMaxGstVolume;

      g_object_set(G_OBJECT(playbin_), "volume", gst_volume, NULL);
    } else {
      DLOG("Playbin was not initialized correctly.");
    }
  }

  virtual void Play() {
    DLOG("GstAudioclip: Play(%s)", src_.c_str());
    if (playbin_ && src_.length()) {
      if (gst_element_set_state(playbin_, GST_STATE_PLAYING) ==
          GST_STATE_CHANGE_FAILURE) {
        LOG("Failed to play the audio.");
      }
    } else {
      if (!playbin_)
        DLOG("Playbin was not initialized correctly.");
      else
        LOG("No audio source was set.");
    }
  }

  virtual void Pause() {
    DLOG("GstAudioclip: Pause(%s)", src_.c_str());
    if (playbin_ && local_state_ == SOUND_STATE_PLAYING) {
      if (gst_element_set_state(playbin_, GST_STATE_PAUSED) ==
          GST_STATE_CHANGE_FAILURE) {
        LOG("Failed to pause the audio.");
      }
    }
  }

  virtual void Stop() {
    DLOG("GstAudioclip: Stop(%s)", src_.c_str());
    if (playbin_ && local_state_ != SOUND_STATE_STOPPED) {
      // If set "NULL" state here, playbin won't produce "STATE CHANGED" message.
      if (gst_element_set_state(playbin_, GST_STATE_NULL) ==
          GST_STATE_CHANGE_FAILURE) {
        LOG("Failed to stop the audio.");
      } else if (local_state_ != SOUND_STATE_ERROR) {
        // If an error has ever happened, the state of gstreamer is "PAUSED",
        // so we set it to "NULL" state above. But we don't clear the ERROR
        // sign, let it be there until gstreamer itself changes its state.

        // Playbin won't post "STATE CHANGED" message when being set to
        // "NULL" state. We make a state-change scene manually.
        local_state_ = SOUND_STATE_STOPPED;
        on_state_change_signal_(local_state_);
      }
    }
  }

  virtual Connection *ConnectOnStateChange(OnStateChangeHandler *handler) {
    return on_state_change_signal_.Connect(handler);
  }

 private:
  void OnStateChange(GstMessage *msg) {
    ASSERT(msg);
    GstState old_state, new_state;
    gst_message_parse_state_changed(msg, &old_state, &new_state, NULL);
    DLOG("GstAudioclip: OnStateChange: old=%d new=%d", old_state, new_state);

    // Only care about effective state change.
    if (gst_state_ == GST_STATE_VOID_PENDING || gst_state_ == old_state) {
      State new_local_state = GstStateToLocalState(new_state);
      bool changed = false;
      if (local_state_ == SOUND_STATE_STOPPED) {
        changed = (new_local_state == SOUND_STATE_PLAYING);
      } else if (local_state_ == SOUND_STATE_PLAYING) {
        changed = (new_local_state == SOUND_STATE_STOPPED ||
                   new_local_state == SOUND_STATE_PAUSED);
      } else if (local_state_ == SOUND_STATE_PAUSED) {
        changed = (new_local_state == SOUND_STATE_PLAYING);
      } else if (new_local_state == SOUND_STATE_ERROR) {
        changed = (new_local_state != local_state_);
      }

      if (changed) {
        DLOG("GstAudioclip: local state changed: old=%d new=%d",
             local_state_, new_local_state);
        local_state_ = new_local_state;
        on_state_change_signal_(local_state_);
      }
    }
    gst_state_ = new_state;
  }

  void OnError(GstMessage *msg) {
    ASSERT(msg);
    GError *gerror;
    gchar *debug;
    gst_message_parse_error(msg, &gerror, &debug);
    DLOG("AudioClip OnError: domain=%d code=%d message=%s debug=%s",
         gerror->domain, gerror->code, gerror->message, debug);

    if (gerror->domain == GST_RESOURCE_ERROR &&
        (gerror->code == GST_RESOURCE_ERROR_NOT_FOUND ||
         gerror->code == GST_RESOURCE_ERROR_OPEN_READ ||
         gerror->code == GST_RESOURCE_ERROR_OPEN_READ_WRITE)) {
      local_error_ = SOUND_ERROR_BAD_CLIP_SRC;
    } else if (gerror->domain == GST_STREAM_ERROR &&
               (gerror->code == GST_STREAM_ERROR_NOT_IMPLEMENTED ||
                gerror->code == GST_STREAM_ERROR_TYPE_NOT_FOUND ||
                gerror->code == GST_STREAM_ERROR_WRONG_TYPE ||
                gerror->code == GST_STREAM_ERROR_CODEC_NOT_FOUND ||
                gerror->code == GST_STREAM_ERROR_FORMAT)) {
      local_error_ = SOUND_ERROR_FORMAT_NOT_SUPPORTED;
    } else {
      local_error_ = SOUND_ERROR_UNKNOWN;
    }
    local_state_ = SOUND_STATE_ERROR;
    on_state_change_signal_(local_state_);

    g_error_free(gerror);
    g_free(debug);
  }

  void OnEnd() {
    // Playbin doesnot change the state to "NULL" or "READY" when it
    // reaches the end of the stream, we help make a state-change scene.
    Stop();
  }

  static gboolean OnNewMessage(GstBus *bus, GstMessage *msg, gpointer object) {
    GGL_UNUSED(bus);
    switch (GST_MESSAGE_TYPE(msg)) {
      case GST_MESSAGE_ERROR:
        (reinterpret_cast<GstAudioclip*>(object))->OnError(msg);
        break;
      case GST_MESSAGE_EOS:
        (reinterpret_cast<GstAudioclip*>(object))->OnEnd();
        break;
      case GST_MESSAGE_STATE_CHANGED:
        (reinterpret_cast<GstAudioclip*>(object))->OnStateChange(msg);
        break;
      default:
        break;
    }
    return true;
  }

  // Mapping from gstreamer state to local state.
  static State GstStateToLocalState(GstState state) {
    switch (state) {
      case GST_STATE_NULL:
      case GST_STATE_READY:
        return SOUND_STATE_STOPPED;
      case GST_STATE_PAUSED:
        return SOUND_STATE_PAUSED;
      case GST_STATE_PLAYING:
        return SOUND_STATE_PLAYING;
      default:
        return SOUND_STATE_ERROR;
    }
  }

 private:
  // Audio source to play.
  std::string src_;

  // Audio & video stream player.
  GstElement *playbin_;
  // Control audio balance.
  GstElement *panorama_;

  // Audio state and the latest reported error code.
  State local_state_;
  ErrorCode local_error_;
  GstState gst_state_;

  // Closure to be called when the state of audio player changes.
  Signal1<void, State> on_state_change_signal_;
};

class GstAudio : public AudioInterface {
 public:
  virtual AudioclipInterface * CreateAudioclip(const char *src)  {
    return g_gst_init_ok_ ? new GstAudioclip(src) : NULL;
  }
};

static GstAudio g_gst_audio_;

} // namespace gst_audio
} // namespace framework
} // namespace ggadget

using namespace ggadget;
using namespace ggadget::framework;
using namespace ggadget::framework::gst_audio;

extern "C" {
  bool Initialize() {
    LOGI("Initialize gst_audio_framework extension.");
    GError *error = NULL;
    g_gst_init_ok_ = gst_init_check(NULL, NULL, &error);
    if (error) {
      LOGI("Failed to initialize gstreamer: %s", error->message);
      g_error_free(error);
    }
    return true;
  }

  void Finalize() {
    LOGI("Finalize gst_audio_framework extension.");
    if (g_gst_init_ok_)
      gst_deinit();
  }

  bool RegisterFrameworkExtension(ScriptableInterface *framework,
                                  GadgetInterface *gadget) {
    LOGI("Register gst_audio_framework extension.");
    ASSERT(framework && gadget);

    if (!framework)
      return false;

    RegisterableInterface *reg_framework = framework->GetRegisterable();

    if (!reg_framework) {
      LOG("Specified framework is not registerable.");
      return false;
    }

    // ScriptableAudio is per gadget, so create a new instance here.
    ScriptableAudio *script_audio = new ScriptableAudio(&g_gst_audio_, gadget);
    reg_framework->RegisterVariantConstant("audio", Variant(script_audio));

    return true;
  }
}
