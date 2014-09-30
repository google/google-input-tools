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

#include "object_videoplayer.h"

#include <vector>

#include "basic_element.h"
#include "canvas_interface.h"
#include "elements.h"
#include "element_factory.h"
#include "file_manager_interface.h"
#include "object_element.h"
#include "slot.h"
#include "signals.h"
#include "video_element_base.h"
#include "view.h"
#include "small_object.h"

namespace ggadget {

static const char kOnStateChangeEvent[] = "PlayStateChange";
static const char kOnPositionChangeEvent[] = "PositionChange";
static const char kOnMediaChangeEvent[] = "MediaChange";
static const char kOnPlaylistChangeEvent[] = "PlaylistChange";
static const char kOnPlayerDockedStateChangeEvent[] = "PlayerDockedStateChange";

static const int kMaxWmpVolume = 100;
static const int kMinWmpVolume = 0;

// Definition of wmplayer play state.
enum WMPState {
  WMP_STATE_UNDEFINED = 0,
  WMP_STATE_STOPPED = 1,
  WMP_STATE_PAUSED = 2,
  WMP_STATE_PLAYING = 3,
  WMP_STATE_ENDED = 8,
  WMP_STATE_READY = 10,
};

class ObjectVideoPlayer::Impl : public SmallObject<> {
 public:
  class Media : public ScriptableHelperDefault {
   public:
    DEFINE_CLASS_ID(0x72d10c43fea34b38, ScriptableInterface);

    Media(const std::string &uri) : uri_(uri), duration_(0) {
      name_ = uri.substr(uri.find_last_of('/') + 1);
      size_t n = name_.find_last_of('.');
      if (n != std::string::npos)
        name_ = name_.substr(0, n);

      RegisterProperty("name",
                       NewSlot(this, &Media::GetName),
                       NewSlot(this, &Media::SetName));
      RegisterProperty("sourceURL", NewSlot(this, &Media::GetUri), NULL);
      RegisterProperty("duration", NewSlot(this, &Media::GetDuration), NULL);

      RegisterMethod("getItemInfo", NewSlot(this, &Media::GetItemInfo));
      RegisterMethod("setItemInfo", NewSlot(this, &Media::SetItemInfo));
      RegisterMethod("isReadOnlyItem", NewSlot(this, &Media::IsReadOnlyItem));
    }

    ~Media() { }

    // Gets and sets of scriptable properties.
    std::string GetName() { return name_; }
    void SetName(const std::string &name) { name_ = name; }
    std::string GetUri() { return uri_; }
    double GetDuration() { return duration_; }

    std::string GetItemInfo(const std::string& attr) {
      if (attr.compare("Author") == 0)
        return author_;
      else if (attr.compare("Title") == 0)
        return title_;
      else if (attr.compare("WM/AlbumTitle") == 0)
        return album_;
      else
        return "";
    }

    void SetItemInfo(const std::string& attr, const std::string& value) {
      GGL_UNUSED(attr);
      GGL_UNUSED(value);
      // Currently, users are not allowed to modify the tag info.
    }

    bool IsReadOnlyItem(const std::string& attr) {
      GGL_UNUSED(attr);
      return true;
    }

    std::string uri_, name_;
    std::string author_, title_, album_;
    double duration_;
  };

  class Playlist : public ScriptableHelperDefault {
   public:
    DEFINE_CLASS_ID(0x209b1644318849d7, ScriptableInterface);

    Playlist(const std::string &name) : name_(name), current_(0) {
      RegisterProperty("count", NewSlot(this, &Playlist::GetCount), NULL);
      RegisterProperty("name",
                       NewSlot(this, &Playlist::GetName),
                       NewSlot(this, &Playlist::SetName));
      RegisterMethod("appendItem", NewSlot(this, &Playlist::AppendItem));
    }

    ~Playlist() {
      if (items_.size()) {
        for (size_t i = 0; i < items_.size(); i++)
          // Don't use delete, the media may be also being used by others.
          items_[i]->Unref();
      }
    }

    Media *GetFirstMedia() {
      current_ = 0;
      return items_.size() ? items_[0] : NULL;
    }

    Media *GetPreviousMedia(bool loop) {
      if (current_ > 0) {
        current_--;
        return items_[current_];
      } else if (loop && items_.size()) {
        current_ = items_.size() - 1;
        return items_[current_];
      } else {
        return NULL;
      }
    }

    Media *GetNextMedia(bool loop) {
      if (current_ + 1 < items_.size()) {
        current_++;
        return items_[current_];
      } else if (loop && items_.size()) {
        current_ = 0;
        return items_[current_];
      } else {
        return NULL;
      }
    }

    size_t GetCount() { return items_.size(); }
    std::string GetName() { return name_; }
    void SetName(const std::string &name) { name_ = name; }

    void AppendItem(Media *media) {
      media->Ref();
      items_.push_back(media);
      // Let current point to the last item, so that the first call of
      // GetNextMedia() will get the first media.
      current_ = items_.size() - 1;
    }

    std::string name_;
    std::vector<Media *> items_;
    size_t current_;
  };

  Impl(ObjectVideoPlayer *owner, View *view)
      : owner_(owner), view_(view),
        current_media_(NULL),
        current_playlist_(NULL),
        auto_start_(true),
        loop_(false) {

    // Create the video element.
    // Although the ObjectVideoPlayer cannot have any children(otherwise,
    // the children will be exposed to the outside code), it can be (actually
    // must be) the parent of the video element, otherwise, video element has
    // no way to know the size of area in which the video is shown.
    video_element_ = down_cast<VideoElementBase *>(
        view_->GetElementFactory()->CreateElement("video", view_, "video"));
    if (!video_element_) {
      return;
    }

    video_element_->SetParentElement(owner_);
    video_element_->ConnectOnStateChangeEvent(NewSlot(this,
                                                      &Impl::OnStateChange));
    video_element_->ConnectOnMediaChangeEvent(NewSlot(this,
                                                      &Impl::OnMediaChange));
    video_element_->SetRelativeX(0);
    video_element_->SetRelativeY(0);
    video_element_->SetRelativeWidth(1.0);
    video_element_->SetRelativeHeight(1.0);
  }

  void DoRegister() {
    controls_.RegisterProperty(
        "currentPosition",
        NewSlot(video_element_, &VideoElementBase::GetCurrentPosition),
        NewSlot(video_element_, &VideoElementBase::SetCurrentPosition));
    controls_.RegisterMethod("isAvailable", NewSlot(this, &Impl::IsAvailable));
    controls_.RegisterMethod("play", NewSlot(this, &Impl::Play));
    controls_.RegisterMethod("playItem",
                             NewSlot(this, &Impl::SetCurrentMedia, false));
    controls_.RegisterMethod("pause", NewSlot(this, &Impl::Pause));
    controls_.RegisterMethod("stop", NewSlot(this, &Impl::Stop));
    controls_.RegisterMethod("previous",
                             // Always loops for previous().
                             NewSlot(this, &Impl::PlayPrevious, true));
    controls_.RegisterMethod("next",
                             // Always loops for next().
                             NewSlot(this, &Impl::PlayNext, true));
    controls_.RegisterMethod("fastForward", NewSlot(Dummy));
    controls_.RegisterMethod("fastReverse", NewSlot(Dummy));
    controls_.RegisterMethod("step", NewSlot(Dummy));
    controls_.RegisterProperty("currentItem",
                               NewSlot(this, &Impl::GetCurrentMedia),
                               NewSlot(this, &Impl::SetCurrentMedia, true));

    settings_.RegisterMethod("isAvailable", NewSlot(this, &Impl::IsAvailable));
    settings_.RegisterMethod("getMode", NewSlot(this, &Impl::GetMode));
    settings_.RegisterMethod("setMode", NewSlot(this, &Impl::SetMode));
    settings_.RegisterMethod("requestMediaAccessRights",
                             NewSlot(this, &Impl::RequestMediaAccessRights));
    settings_.RegisterProperty("autoStart",
                               NewSlot(this, &Impl::IsAutoStart),
                               NewSlot(this, &Impl::SetAutoStart));
    settings_.RegisterProperty("volume",
                               NewSlot(this, &Impl::GetVolume),
                               NewSlot(this, &Impl::SetVolume));
    settings_.RegisterProperty(
        "balance",
        NewSlot(video_element_, &VideoElementBase::GetBalance),
        NewSlot(video_element_, &VideoElementBase::SetBalance));
    settings_.RegisterProperty(
        "mute",
        NewSlot(video_element_, &VideoElementBase::IsMute),
        NewSlot(video_element_, &VideoElementBase::SetMute));

    application_.RegisterConstant("hasDisplay", true);
    application_.RegisterConstant("playerDocked", true);
    application_.RegisterMethod("switchToControl", NewSlot(Dummy));
    application_.RegisterMethod("switchToPlayerApplication", NewSlot(Dummy));

    owner_->RegisterConstant("controls", &controls_);
    owner_->RegisterConstant("settings", &settings_);
    owner_->RegisterConstant("playerApplication", &application_);
    owner_->RegisterProperty("currentMedia",
                             NewSlot(this, &Impl::GetCurrentMedia),
                             NewSlot(this, &Impl::SetCurrentMedia, true));
    owner_->RegisterProperty("currentPlaylist",
                             NewSlot(this, &Impl::GetCurrentPlaylist),
                             NewSlot(this, &Impl::SetCurrentPlaylist));
    owner_->RegisterProperty("playState",
                             NewSlot(this, &Impl::GetState), NULL);
    owner_->RegisterProperty("url",
                             NewSlot(this, &Impl::GetURL),
                             NewSlot(this, &Impl::SetURL));

    owner_->RegisterMethod("close", NewSlot(this, &Impl::CloseCurrentPlaylist));
    owner_->RegisterMethod("newMedia", NewSlot(this, &Impl::NewMedia));
    owner_->RegisterMethod("newPlaylist", NewSlot(this, &Impl::NewPlaylist));
    owner_->RegisterMethod("launchURL",
                           NewSlot(owner_->GetView(), &View::OpenURL));

    owner_->RegisterProperty("enableContextMenu",
                             NewFixedGetterSlot(Variant(false)),
                             NewSlot(DummySetter));
    owner_->RegisterProperty("enableErrorDialogs",
                             NewFixedGetterSlot(Variant(false)),
                             NewSlot(DummySetter));
    owner_->RegisterProperty("uiMode",
                             NewSlot(this, &Impl::GetUIMode),
                             NewSlot(this, &Impl::SetUIMode));

    BasicElement *parent = owner_->GetParentElement();
    parent->RegisterSignal(kOnStateChangeEvent, &on_state_change_event_);
    parent->RegisterSignal(kOnPositionChangeEvent, &on_position_change_event_);
    parent->RegisterSignal(kOnMediaChangeEvent, &on_media_change_event_);
    parent->RegisterSignal(kOnPlaylistChangeEvent, &on_playlist_change_event_);
    parent->RegisterSignal(kOnPlayerDockedStateChangeEvent,
                           &on_player_docked_state_change_event_);
    parent->RegisterProperty("wmpServiceType",
                             NewFixedGetterSlot(Variant("Local")),
                             NewSlot(DummySetter));
    parent->RegisterProperty("wmpSkin",
                             NewFixedGetterSlot(Variant("")),
                             NewSlot(DummySetter));
  }

  ~Impl() {
    delete video_element_;
    if (current_media_)
      current_media_->Unref();
    if (current_playlist_)
      current_playlist_->Unref();
  }

  static void Dummy() { }

  bool IsAutoStart() { return auto_start_; }
  void SetAutoStart(bool auto_start) { auto_start_ = auto_start; }

  int GetVolume() {
    int volume = video_element_->GetVolume();
    double percent =
        static_cast<double>(volume - VideoElementBase::kMinVolume) /
        (VideoElementBase::kMaxVolume - VideoElementBase::kMinVolume);
    return static_cast<int>(percent * (kMaxWmpVolume - kMinWmpVolume));
  }

  void SetVolume(int volume) {
    double percent = static_cast<double>(volume - kMinWmpVolume) /
        (kMaxWmpVolume - kMinWmpVolume);
    double fvolume = VideoElementBase::kMinVolume +
        percent * (VideoElementBase::kMaxVolume - VideoElementBase::kMinVolume);
    video_element_->SetVolume(static_cast<int>(fvolume));
  }

  bool IsAvailable(const std::string &name) {
    if (name == "previous" || name == "next" || name == "currentItem")
      return current_playlist_ && current_playlist_->GetCount();
    return video_element_->IsAvailable(name);
  }

  void Play() {
    if (current_media_) {
      if (current_media_->uri_ != video_element_->GetSrc()) {
        video_element_->Stop();
        video_element_->SetSrc(current_media_->uri_);
      }
      video_element_->Play();
    }
  }

  void Pause() {
    video_element_->Pause();
  }

  void Stop() {
    video_element_->Stop();
    on_position_change_event_();
  }

  void SetCurrentPosition(double position) {
    video_element_->SetCurrentPosition(position);
    on_position_change_event_();
  }

  void PlayPrevious(bool loop) {
    Media *previous;
    if (current_playlist_ &&
        (previous = current_playlist_->GetPreviousMedia(loop))) {
      SetCurrentMediaInternal(previous, true, true);
    }
  }

  void PlayNext(bool loop) {
    Media *next;
    if (current_playlist_ &&
        (next = current_playlist_->GetNextMedia(loop))) {
      SetCurrentMediaInternal(next, true, true);
    }
  }

  const char *GetURL() {
    return video_element_->GetSrc().c_str();
  }

  void SetURL(const char *url) {
    if (!url || !*url)
      return;
    Media *media = NewMedia(url);
    if (media)
      SetCurrentMedia(media, true);
  }

  WMPState GetState() {
    VideoElementBase::State state = video_element_->GetState();
    switch (state) {
      case VideoElementBase::STATE_READY:
        return WMP_STATE_READY;
      case VideoElementBase::STATE_PLAYING:
        return WMP_STATE_PLAYING;
      case VideoElementBase::STATE_PAUSED:
        return WMP_STATE_PAUSED;
      case VideoElementBase::STATE_STOPPED:
        return WMP_STATE_STOPPED;
      case VideoElementBase::STATE_ENDED:
        return WMP_STATE_ENDED;
      default:
        return WMP_STATE_UNDEFINED;
    }
  }

  void OnStateChange() {
    on_state_change_event_();
    // Turn to the next video in the playlist if the current video is ended.
    if (video_element_->GetState() == VideoElementBase::STATE_ENDED)
      PlayNext(loop_);
  }

  void OnMediaChange() {
    if (current_media_) {
      current_media_->duration_ = video_element_->GetDuration();
      current_media_->author_ =
          video_element_->GetTagInfo(VideoElementBase::TAG_AUTHOR);
      current_media_->title_ =
          video_element_->GetTagInfo(VideoElementBase::TAG_TITLE);
      current_media_->album_ =
          video_element_->GetTagInfo(VideoElementBase::TAG_ALBUM);
      on_media_change_event_();
    }
  }

  Media *NewMedia(const char *uri) {
    if (!uri || !*uri) return NULL;

    std::string real_uri;
    if (strstr(uri, "://")) {
      real_uri = uri;
    } else if (*uri == '/') {
      real_uri = std::string("file://") + uri;
    } else {
      // It may be a relative file name under the base path of the gadget.
      std::string extracted_file;
      FileManagerInterface *file_manager = view_->GetFileManager();
      if (!file_manager || !file_manager->ExtractFile(uri, &extracted_file))
        return NULL;
      real_uri = "file://" + extracted_file;
    }
    return new Media(real_uri);
  }

  Media *GetCurrentMedia() {
    return current_media_;
  }

  bool SetCurrentMediaInternal(Media *media, bool auto_start, bool rewind) {
    if (!media || current_media_ == media)
      return false;
    if (current_media_)
      CloseCurrentMedia();
    current_media_ = media;
    current_media_->Ref();
    if (rewind)
      Stop();
    if (auto_start)
      Play();
    return true;
  }

  bool SetCurrentMedia(Media *media, bool rewind) {
    return SetCurrentMediaInternal(media, auto_start_, rewind);
  }

  Playlist *NewPlaylist(const char *name, const char *meta_file) {
    GGL_UNUSED(meta_file);
    // We don't use any meta file for playlist. The parameter exists for
    // interface compatibility.
    if (name) {
      Playlist *new_playlist = new Playlist(name);
      return new_playlist;
    }
    return NULL;
  }

  Playlist *GetCurrentPlaylist() {
    return current_playlist_;
  }

  bool SetCurrentPlaylist(Playlist *playlist) {
    if (!playlist || current_playlist_ == playlist)
      return false;
    if (current_playlist_)
      CloseCurrentPlaylist();
    current_playlist_ = playlist;
    current_playlist_->Ref();
    on_playlist_change_event_();
    return SetCurrentMedia(current_playlist_->GetFirstMedia(), true);
  }

  void CloseCurrentMedia() {
    if (current_media_) {
      video_element_->Stop();
      current_media_->Unref();
      current_media_ = NULL;
    }
  }

  void CloseCurrentPlaylist() {
    if (current_playlist_) {
      CloseCurrentMedia();
      current_playlist_->Unref();
      current_playlist_ = NULL;
    }
  }

  std::string GetUIMode() {
    return video_element_->IsVisible() ? "none" : "invisible";
  }

  void SetUIMode(const std::string &uimode) {
    video_element_->SetVisible(uimode != "invisible");
  }

  bool GetMode(const std::string &mode) {
    return mode == "loop" ? loop_ : false;
  }

  void SetMode(const std::string &mode, bool state) {
    if (mode == "loop")
      loop_ = state;
  }

  bool RequestMediaAccessRights(const std::string &access) {
    GGL_UNUSED(access);
    return true;
  }

  ObjectVideoPlayer *owner_;
  View *view_;

  // The real play backend we wrap.
  VideoElementBase *video_element_;

  Media *current_media_;
  Playlist *current_playlist_;

  NativeOwnedScriptable<UINT64_C(0x42a88e66ff444ba1)> controls_;
  NativeOwnedScriptable<UINT64_C(0xde2169669ebf4b61)> settings_;
  NativeOwnedScriptable<UINT64_C(0x1af44fe45e404eae)> application_;

  EventSignal on_state_change_event_;
  EventSignal on_position_change_event_;
  EventSignal on_media_change_event_;
  EventSignal on_playlist_change_event_;
  // Never fired.
  EventSignal on_player_docked_state_change_event_;

  // Indicates whether to automatically call Play() after current media or
  // playlist is changed.
  bool auto_start_ : 1;
  // Indicates whether to automatically loop to the first media when the last
  // media finishes. It doesn't affect previous() and next() calls.
  bool loop_       : 1;
};

ObjectVideoPlayer::ObjectVideoPlayer(View *view, const char *name)
    : BasicElement(view, "object", name, false) {
  SetEnabled(true);
  impl_ = new Impl(this, view);
  if (!impl_->video_element_)
    return;

  SetRelativeX(0);
  SetRelativeY(0);
  SetRelativeWidth(1.0);
  SetRelativeHeight(1.0);
}

ObjectVideoPlayer::~ObjectVideoPlayer() {
  delete impl_;
}

BasicElement *ObjectVideoPlayer::CreateInstance(View *view, const char *name) {
  ObjectVideoPlayer *self = new ObjectVideoPlayer(view, name);
  if (!self->impl_->video_element_) {
    delete self;
    return NULL;
  }
  return self;
}

void ObjectVideoPlayer::Layout() {
  BasicElement::Layout();
  if (impl_->video_element_) {
    impl_->video_element_->Layout();
  }
}

void ObjectVideoPlayer::DoClassRegister() {
  // Don't register properties inherited from BasicElement.
  // Properties of this object are exposed to the outside code by the 'object'
  // property of the belonging object element.
}

void ObjectVideoPlayer::DoRegister() {
  // Don't register properties inherited from BasicElement.
  // Properties of this object are exposed to the outside code by the 'object'
  // property of the belonging object element.
  impl_->DoRegister();
}

void ObjectVideoPlayer::DoDraw(CanvasInterface *canvas) {
  if (canvas && impl_->video_element_) {
    impl_->video_element_->Draw(canvas);
  }
}

void ObjectVideoPlayer::AggregateMoreClipRegion(const Rectangle &boundary,
                                                ClipRegion *region) {
  if (impl_->video_element_) {
    impl_->video_element_->AggregateClipRegion(boundary, region);
  }
}

} // namespace ggadget
