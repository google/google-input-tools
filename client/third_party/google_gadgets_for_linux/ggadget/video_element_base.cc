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

#include "video_element_base.h"

#include "basic_element.h"
#include "canvas_interface.h"
#include "elements.h"
#include "scriptable_event.h"
#include "signals.h"
#include "view.h"
#include "small_object.h"

namespace ggadget {

class VideoElementBase::Impl : public SmallObject<> {
 public:
  Impl()
      : image_data_(NULL),
        image_x_(0), image_y_(0), image_w_(0), image_h_(0), image_stride_(0) {
  }
  ~Impl() { }

  const char *image_data_;
  int image_x_;
  int image_y_;
  int image_w_;
  int image_h_;
  int image_stride_;

  EventSignal on_state_change_event_;
  EventSignal on_media_change_event_;
};

VideoElementBase::VideoElementBase(View *view, const char *tag_name,
                                   const char *name)
    : BasicElement(view, tag_name, name, false),
      impl_(new Impl) {
  SetEnabled(true);
}

VideoElementBase::~VideoElementBase() {
  delete impl_;
}

bool VideoElementBase::IsAvailable(const std::string &name) const {
  State state = GetState();
  if (name == "play") {
    return state == STATE_READY || state == STATE_PAUSED ||
           state == STATE_STOPPED;
  } else if (name == "pause") {
    return state == STATE_PLAYING;
  } else if (name == "stop") {
    return state == STATE_PLAYING || state == STATE_PAUSED ||
           state == STATE_ENDED;
  } else if (name == "seek" || name == "currentPosition") {
    return (state == STATE_PLAYING || state == STATE_PAUSED) && IsSeekable();
  }

  // For "volume", "balance", and "mute", let the real video element to
  // decide whether these controls can be supported.
  return false;
}

Connection *VideoElementBase::ConnectOnStateChangeEvent(Slot0<void> *handler) {
  return impl_->on_state_change_event_.Connect(handler);
}

Connection *VideoElementBase::ConnectOnMediaChangeEvent(Slot0<void> *handler) {
  return impl_->on_media_change_event_.Connect(handler);
}

void VideoElementBase::DoClassRegister() {
  BasicElement::DoClassRegister();

  RegisterProperty("currentPosition",
                   NewSlot(&VideoElementBase::GetCurrentPosition),
                   NewSlot(&VideoElementBase::SetCurrentPosition));
  RegisterProperty("duration", NewSlot(&VideoElementBase::GetDuration), NULL);
  RegisterProperty("error", NewSlot(&VideoElementBase::GetErrorCode), NULL);
  RegisterProperty("state", NewSlot(&VideoElementBase::GetState), NULL);
  RegisterProperty("seekable", NewSlot(&VideoElementBase::IsSeekable), NULL);
  RegisterProperty("src",
                   NewSlot(&VideoElementBase::GetSrc),
                   NewSlot(&VideoElementBase::SetSrc));
  RegisterProperty("volume",
                   NewSlot(&VideoElementBase::GetVolume),
                   NewSlot(&VideoElementBase::SetVolume));
  RegisterProperty("balance",
                   NewSlot(&VideoElementBase::GetBalance),
                   NewSlot(&VideoElementBase::SetBalance));
  RegisterProperty("mute",
                   NewSlot(&VideoElementBase::IsMute),
                   NewSlot(&VideoElementBase::SetMute));

  RegisterMethod("isAvailable", NewSlot(&VideoElementBase::IsAvailable));
  RegisterMethod("play", NewSlot(&VideoElementBase::Play));
  RegisterMethod("pause", NewSlot(&VideoElementBase::Pause));
  RegisterMethod("stop", NewSlot(&VideoElementBase::Stop));

  RegisterClassSignal(kOnStateChangeEvent, &Impl::on_state_change_event_,
                      &VideoElementBase::impl_);
  RegisterClassSignal(kOnMediaChangeEvent, &Impl::on_media_change_event_,
                      &VideoElementBase::impl_);
}

void VideoElementBase::DoDraw(CanvasInterface *canvas) {
  if (!impl_->image_data_ ||
      impl_->image_x_ != 0 || impl_->image_y_ != 0 ||
      impl_->image_w_ != GetPixelWidth() ||
      impl_->image_h_ != GetPixelHeight()) {
    canvas->DrawFilledRect(0, 0, GetPixelWidth(), GetPixelHeight(),
                           Color::kBlack);
  }
  if (impl_->image_data_) {
    canvas->DrawRawImage(impl_->image_x_, impl_->image_y_, impl_->image_data_,
                         CanvasInterface::RAWIMAGE_FORMAT_RGB24,
                         impl_->image_w_, impl_->image_h_,
                         impl_->image_stride_);
  }
}

void VideoElementBase::Layout() {
  BasicElement::Layout();
  if (IsSizeChanged())
    SetGeometry(GetPixelWidth(), GetPixelHeight());
}

bool VideoElementBase::PutImage(const char *data,
                                int x, int y, int w, int h, int stride) {
  impl_->image_data_ = reinterpret_cast<const char *>(data);
  impl_->image_x_ = x;
  impl_->image_y_ = y;
  impl_->image_w_ = w;
  impl_->image_h_ = h;
  impl_->image_stride_ = stride;
  QueueDraw();
  return true;
}

void VideoElementBase::ClearImage() {
  impl_->image_data_ = NULL;
  QueueDraw();
}

void VideoElementBase::FireOnStateChangeEvent() {
  SimpleEvent event(Event::EVENT_STATE_CHANGE);
  ScriptableEvent s_event(&event, this, NULL);
  GetView()->FireEvent(&s_event, impl_->on_state_change_event_);
}

void VideoElementBase::FireOnMediaChangeEvent() {
  SimpleEvent event(Event::EVENT_MEDIA_CHANGE);
  ScriptableEvent s_event(&event, this, NULL);
  GetView()->FireEvent(&s_event, impl_->on_media_change_event_);
}

} // namespace ggadget
