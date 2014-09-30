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

#include "progressbar_element.h"
#include "canvas_interface.h"
#include "gadget_consts.h"
#include "image_interface.h"
#include "logger.h"
#include "math_utils.h"
#include "scriptable_event.h"
#include "string_utils.h"
#include "view.h"
#include "small_object.h"

namespace ggadget {

static const char *kOrientationNames[] = {
  "vertical", "horizontal"
};

class ProgressBarElement::Impl : public SmallObject<> {
 public:
  Impl(ProgressBarElement *owner)
    : drag_delta_(0.),
      owner_(owner),
      emptyimage_(NULL),
      fullimage_(NULL),
      thumbdisabledimage_(NULL),
      thumbdownimage_(NULL),
      thumboverimage_(NULL),
      thumbimage_(NULL),
      // The values below are the default ones in Windows.
      min_(0), max_(100), value_(0),
      orientation_(ORIENTATION_HORIZONTAL),
      thumbover_(false),
      thumbdown_(false),
      default_rendering_(false) {
  }

  ~Impl() {
    DestroyImages();
  }

  void DestroyImages() {
    DestroyImage(emptyimage_);
    emptyimage_ = NULL;
    DestroyImage(fullimage_);
    fullimage_ = NULL;
    DestroyImage(thumbdisabledimage_);
    thumbdisabledimage_ = NULL;
    DestroyImage(thumbdownimage_);
    thumbdownimage_ = NULL;
    DestroyImage(thumboverimage_);
    thumboverimage_ = NULL;
    DestroyImage(thumbimage_);
    thumbimage_ = NULL;
  }

  void LoadImage(ImageInterface **image, const Variant &src, bool queue_draw) {
    if (src != Variant(GetImageTag(*image))) {
      DestroyImage(*image);
      *image = owner_->GetView()->LoadImage(src, false);
      if (queue_draw)
        owner_->QueueDraw();
    }
  }

  void EnsureDefaultImages() {
    if (default_rendering_) {
      View *view = owner_->GetView();
      if (!emptyimage_)
        emptyimage_ = view->LoadImageFromGlobal(kProgressBarEmptyImage, false);
      if (!fullimage_)
        fullimage_ = view->LoadImageFromGlobal(kProgressBarFullImage, false);
    }
  }

  void DestroyDefaultImages() {
    if (GetImageTag(emptyimage_) == kProgressBarEmptyImage) {
      DestroyImage(emptyimage_);
      emptyimage_ = NULL;
    }
    if (GetImageTag(fullimage_) == kProgressBarFullImage) {
      DestroyImage(fullimage_);
      fullimage_ = NULL;
    }
  }

  /**
   * Utility function for getting the int value from a position on the
   * progressbar. It does not check to make sure that the value is within range.
   * Assumes that thumb isn't NULL.
   */
  int GetValueFromLocation(double ownerwidth, double ownerheight,
                           ImageInterface *thumb, double x, double y) {
    int delta = max_ - min_;
    double position, denominator;
    if (orientation_ == ORIENTATION_HORIZONTAL) {
      denominator = ownerwidth;
      if (thumbdown_ && thumb) {
        denominator -= thumb->GetWidth();
      }
      if (denominator == 0) { // prevent overflow
        position = 0;
      } else {
        position = delta * (x - drag_delta_) / denominator;
      }
    } else { // Progressbar grows from the bottom in vertical orientation.
      denominator = ownerheight;
      if (thumbdown_ && thumb) {
        denominator -= thumb->GetHeight();
      }
      if (denominator == 0) { // prevent overflow
        position = 0;
      } else {
        position = delta - (delta * (y - drag_delta_) / denominator);
      }
    }

    int answer = static_cast<int>(position);
    answer += min_;
    return answer;
  }

  /** Returns the current value expressed as a fraction of the total progress. */
  double GetFractionalValue() {
    if (max_ == min_) {
      return 0; // handle overflow & corner cases
    }
    return (value_ - min_) / static_cast<double>(max_ - min_);
  }

  ImageInterface *GetThumbAndLocation(double ownerwidth, double ownerheight,
                             double fraction, double *x, double *y) {
    ImageInterface *thumb = GetCurrentThumbImage();
    if (!thumb) {
      *x = *y = 0;
      return NULL;
    }

    double imgw = thumb->GetWidth();
    double imgh = thumb->GetHeight();
    if (orientation_ == ORIENTATION_HORIZONTAL) {
      *x = fraction * (ownerwidth - imgw);
      *y = (ownerheight - imgh) / 2.;
    } else { // Thumb grows from bottom in vertical orientation.
      *x = (ownerwidth - imgw) / 2.;
      *y = (1. - fraction) * (ownerheight - imgh);
    }
    return thumb;
  }

  void SetValue(int value) {
    if (value > max_) {
      value = max_;
    } else if (value < min_) {
      value = min_;
    }

    if (value != value_) {
      value_ = value;
      owner_->QueueDraw();
      SimpleEvent event(Event::EVENT_CHANGE);
      ScriptableEvent s_event(&event, owner_, NULL);
      owner_->GetView()->FireEvent(&s_event, onchange_event_);
    }
  }

  ImageInterface *GetCurrentThumbImage() {
    ImageInterface *img = NULL;
    if (!owner_->IsEnabled()) {
       img = thumbdisabledimage_;
    } else if (thumbdown_) {
      img = thumbdownimage_;
    } else if (thumbover_) {
      img = thumboverimage_;
    }

    if (!img) { // fallback
      img = thumbimage_;
    }
    return img;
  }

  double drag_delta_;
  ProgressBarElement *owner_;
  ImageInterface *emptyimage_;
  ImageInterface *fullimage_;
  ImageInterface *thumbdisabledimage_;
  ImageInterface *thumbdownimage_;
  ImageInterface *thumboverimage_;
  ImageInterface *thumbimage_;
  EventSignal onchange_event_;

  int min_, max_, value_;
  Orientation orientation_ : 1;
  bool thumbover_          : 1;
  bool thumbdown_          : 1;
  bool default_rendering_  : 1;
};

ProgressBarElement::ProgressBarElement(View *view, const char *name)
    : BasicElement(view, "progressbar", name, false),
      impl_(new Impl(this)) {
}

void ProgressBarElement::DoClassRegister() {
  BasicElement::DoClassRegister();
  RegisterProperty("emptyImage",
                   NewSlot(&ProgressBarElement::GetEmptyImage),
                   NewSlot(&ProgressBarElement::SetEmptyImage));
  RegisterProperty("max",
                   NewSlot(&ProgressBarElement::GetMax),
                   NewSlot(&ProgressBarElement::SetMax));
  RegisterProperty("min",
                   NewSlot(&ProgressBarElement::GetMin),
                   NewSlot(&ProgressBarElement::SetMin));
  RegisterStringEnumProperty("orientation",
                   NewSlot(&ProgressBarElement::GetOrientation),
                   NewSlot(&ProgressBarElement::SetOrientation),
                   kOrientationNames, arraysize(kOrientationNames));
  RegisterProperty("fullImage",
                   NewSlot(&ProgressBarElement::GetFullImage),
                   NewSlot(&ProgressBarElement::SetFullImage));
  RegisterProperty("thumbDisabledImage",
                   NewSlot(&ProgressBarElement::GetThumbDisabledImage),
                   NewSlot(&ProgressBarElement::SetThumbDisabledImage));
  RegisterProperty("thumbDownImage",
                   NewSlot(&ProgressBarElement::GetThumbDownImage),
                   NewSlot(&ProgressBarElement::SetThumbDownImage));
  RegisterProperty("thumbImage",
                   NewSlot(&ProgressBarElement::GetThumbImage),
                   NewSlot(&ProgressBarElement::SetThumbImage));
  RegisterProperty("thumbOverImage",
                   NewSlot(&ProgressBarElement::GetThumbOverImage),
                   NewSlot(&ProgressBarElement::SetThumbOverImage));
  RegisterProperty("value",
                   NewSlot(&ProgressBarElement::GetValue),
                   NewSlot(&ProgressBarElement::SetValue));

  // Undocumented property.
  RegisterProperty("defaultRendering",
                   NewSlot(&ProgressBarElement::IsDefaultRendering),
                   NewSlot(&ProgressBarElement::SetDefaultRendering));

  RegisterClassSignal(kOnChangeEvent, &Impl::onchange_event_,
                      &ProgressBarElement::impl_);
}

ProgressBarElement::~ProgressBarElement() {
  delete impl_;
  impl_ = NULL;
}

void ProgressBarElement::DoDraw(CanvasInterface *canvas) {
  impl_->EnsureDefaultImages();

  // Drawing order: empty, full, thumb.
  // Empty and full images only stretch in one direction, and only if
  // element size is greater than that of the image. Otherwise the image is
  // cropped.
  double pxwidth = GetPixelWidth();
  double pxheight = GetPixelHeight();
  double fraction = impl_->GetFractionalValue();
  bool drawfullimage = impl_->fullimage_ && fraction > 0;

  double x = 0, y = 0, fw = 0, fh = 0, fy = 0;
  bool fstretch = false;
  if (drawfullimage) {
    // Need to calculate fullimage positions first in order to determine
    // clip rectangle for emptyimage.
    double imgw = impl_->fullimage_->GetWidth();
    double imgh = impl_->fullimage_->GetHeight();
    if (impl_->orientation_ == ORIENTATION_HORIZONTAL) {
      x = 0;
      fy = y = (pxheight - imgh) / 2.;
      fw = fraction * pxwidth;
      fh = imgh;
      fstretch = (imgw < fw);
    } else { // Progressbar grows from the bottom in vertical orientation.
      x = (pxwidth - imgw) / 2.;
      fw = imgw;
      fh = fraction * pxheight;
      y = pxheight - fh;
      fstretch = (imgh < fh);
      if (!fstretch) {
        fy = pxheight - imgh;
      }
    }
  }

  if (impl_->emptyimage_) {
    double ex, ey, ew, eh, clipx, cliph;
    double imgw = impl_->emptyimage_->GetWidth();
    double imgh = impl_->emptyimage_->GetHeight();
    bool estretch;
    if (impl_->orientation_ == ORIENTATION_HORIZONTAL) {
      ex = 0;
      ey = (pxheight - imgh) / 2.;
      ew = pxwidth;
      eh = imgh;
      estretch = (imgw < ew);
      clipx = fw;
      cliph = pxheight;
    } else {
      ex = (pxwidth - imgw) / 2.;
      ey = 0;
      ew = imgw;
      eh = pxheight;
      estretch = (imgh < eh);
      clipx = 0;
      cliph = y;
    }

    if (drawfullimage) { // This clip only sets the left/bottom border of image.
      canvas->PushState();
      canvas->IntersectRectClipRegion(clipx, 0, pxwidth, cliph);
    }

    if (estretch) {
      impl_->emptyimage_->StretchDraw(canvas, ex, ey, ew, eh);
    }
    else {
      // No need to set clipping since element border is crop border here.
      impl_->emptyimage_->Draw(canvas, ex, ey);
    }

    if (drawfullimage) {
      canvas->PopState();
    }
  }

  if (drawfullimage) {
    if (fstretch) {
      impl_->fullimage_->StretchDraw(canvas, x, y, fw, fh);
    } else {
      canvas->PushState();
      canvas->IntersectRectClipRegion(x, y, fw, fh);
      impl_->fullimage_->Draw(canvas, x, fy);
      canvas->PopState();
    }
  }

  // The thumb is never resized or cropped.
  ImageInterface *thumb = impl_->GetThumbAndLocation(pxwidth, pxheight,
                                                     fraction, &x, &y);
  if (thumb) {
    thumb->Draw(canvas, x, y);
  }
}

int ProgressBarElement::GetMax() const {
  return impl_->max_;
}

void ProgressBarElement::SetMax(int value) {
  if (value != impl_->max_) {
    impl_->max_ = value;
    if (impl_->value_ > value) {
      impl_->value_ = value;
    }
    QueueDraw();
  }
}

int ProgressBarElement::GetMin() const {
  return impl_->min_;
}

void ProgressBarElement::SetMin(int value) {
  if (value != impl_->min_) {
    impl_->min_ = value;
    if (impl_->value_ < value) {
      impl_->value_ = value;
    }
    QueueDraw();
  }
}

int ProgressBarElement::GetValue() const {
  return impl_->value_;
}

void ProgressBarElement::SetValue(int value) {
  impl_->SetValue(value);
}

ProgressBarElement::Orientation ProgressBarElement::GetOrientation() const {
  return impl_->orientation_;
}

void ProgressBarElement::SetOrientation(ProgressBarElement::Orientation o) {
  if (o != impl_->orientation_) {
    impl_->orientation_ = o;
    QueueDraw();
  }
}

Variant ProgressBarElement::GetEmptyImage() const {
  std::string tag = GetImageTag(impl_->emptyimage_);
  return Variant(tag == kProgressBarEmptyImage ? "" : tag);
}

void ProgressBarElement::SetEmptyImage(const Variant &img) {
  // Changing emptyImage always queue draw, because it effects the default
  // size.
  impl_->LoadImage(&impl_->emptyimage_, img, true);
}

Variant ProgressBarElement::GetFullImage() const {
  std::string tag = GetImageTag(impl_->fullimage_);
  return Variant(tag == kProgressBarFullImage ? "" : tag);
}

void ProgressBarElement::SetFullImage(const Variant &img) {
  impl_->LoadImage(&impl_->fullimage_, img, impl_->value_ != impl_->min_);
}

Variant ProgressBarElement::GetThumbDisabledImage() const {
  return Variant(impl_->default_rendering_ ?
                 "" : GetImageTag(impl_->thumbdisabledimage_));
}

void ProgressBarElement::SetThumbDisabledImage(const Variant &img) {
  if (img != GetThumbDisabledImage())
    impl_->LoadImage(&impl_->thumbdisabledimage_, img, !IsEnabled());
}

Variant ProgressBarElement::GetThumbDownImage() const {
  return Variant(impl_->default_rendering_ ?
                 "" : GetImageTag(impl_->thumbdownimage_));
}

void ProgressBarElement::SetThumbDownImage(const Variant &img) {
  if (img != GetThumbDownImage()) {
    impl_->LoadImage(&impl_->thumbdownimage_, img,
                     impl_->thumbdown_ && IsEnabled());
  }
}

Variant ProgressBarElement::GetThumbImage() const {
  return Variant(impl_->default_rendering_ ?
                 "" : GetImageTag(impl_->thumbimage_));
}

void ProgressBarElement::SetThumbImage(const Variant &img) {
  if (img != GetThumbImage()) {
    // Always queue since this is the fallback.
    impl_->LoadImage(&impl_->thumbimage_, img, true);
  }
}

Variant ProgressBarElement::GetThumbOverImage() const {
  return Variant(impl_->default_rendering_ ?
                 "" : GetImageTag(impl_->thumboverimage_));
}

void ProgressBarElement::SetThumbOverImage(const Variant &img) {
  if (img != GetThumbOverImage()) {
    impl_->LoadImage(&impl_->thumboverimage_, img,
                     impl_->thumbover_ && IsEnabled());
  }
}

bool ProgressBarElement::IsDefaultRendering() const {
  return impl_->default_rendering_;
}

void ProgressBarElement::SetDefaultRendering(bool default_rendering) {
  if (default_rendering != impl_->default_rendering_) {
    impl_->default_rendering_ = default_rendering;
    if (!default_rendering)
      impl_->DestroyDefaultImages();
    QueueDraw();
  }
}

BasicElement *ProgressBarElement::CreateInstance(View *view,
                                                 const char *name) {
  return new ProgressBarElement(view, name);
}

EventResult ProgressBarElement::HandleMouseEvent(const MouseEvent &event) {
  double pxwidth = GetPixelWidth();
  double pxheight = GetPixelHeight();
  double fraction = impl_->GetFractionalValue();
  double tx, ty;
  bool over = false;
  ImageInterface *thumb = impl_->GetThumbAndLocation(pxwidth, pxheight,
                                                     fraction, &tx, &ty);
  if (thumb) {
    over = IsPointInElement(event.GetX() - tx, event.GetY() - ty,
                            thumb->GetWidth(), thumb->GetHeight());
  }

  EventResult result = EVENT_RESULT_HANDLED;
  switch (event.GetType()) {
    case Event::EVENT_MOUSE_MOVE:
    case Event::EVENT_MOUSE_OUT:
    case Event::EVENT_MOUSE_OVER:
      if (event.GetButton() & MouseEvent::BUTTON_LEFT) {
        int value = impl_->GetValueFromLocation(pxwidth, pxheight, thumb,
                                                event.GetX(), event.GetY());
        SetValue(value); // SetValue will queue a draw.
      }

      if (over != impl_->thumbover_) {
        impl_->thumbover_ = over;
        QueueDraw();
      }
      break;
    case Event::EVENT_MOUSE_DOWN:
      if (event.GetButton() & MouseEvent::BUTTON_LEFT) {
        if (over) {
          // The drag delta setting here is tricky. If the button is held down
          // initially over the thumb, then the pointer should always stay
          // on top of the same location on the thumb when dragged, thus
          // reflecting the value indicated by the bottom-left corner of the
          // thumb, not the current position of the pointer.
          // If the mouse button is held down over any other part of the
          // progressbar, then the pointer should reflect the value of the
          // point under it.
          // This is different from scrollbar, where there's only a single
          // case for the drag delta setting. In the progressbar, the drag
          // delta depends on whether the initial mousedown is fired over the
          // thumb or not.
          if (impl_->orientation_ == ORIENTATION_HORIZONTAL) {
            impl_->drag_delta_ = event.GetX() - tx;
          }
          else {
            impl_->drag_delta_ = event.GetY() - ty;
          }

          impl_->thumbdown_ = true;
          QueueDraw(); // Redraw because of the thumbdown.
        } else {
          impl_->drag_delta_ = 0;
          int value = impl_->GetValueFromLocation(pxwidth, pxheight, thumb,
                                                  event.GetX(), event.GetY());
          SetValue(value); // SetValue will queue a draw.
        }
      }
      break;
    case Event::EVENT_MOUSE_UP:
      if (impl_->thumbdown_) {
        impl_->thumbdown_ = false;
        QueueDraw();
      }
      break;
    default:
      result = EVENT_RESULT_UNHANDLED;
      break;
  }
  return result;
}

Connection *ProgressBarElement::ConnectOnChangeEvent(Slot0<void> *handler) {
  return impl_->onchange_event_.Connect(handler);
}

void ProgressBarElement::GetDefaultSize(double *width, double *height) const {
  impl_->EnsureDefaultImages();
  if (impl_->emptyimage_) {
    *width = impl_->emptyimage_->GetWidth();
    *height = impl_->emptyimage_->GetHeight();
  } else {
    *width = *height = 0;
  }
}

bool ProgressBarElement::HasOpaqueBackground() const {
  return impl_->fullimage_ && impl_->fullimage_->IsFullyOpaque() &&
         impl_->emptyimage_ && impl_->emptyimage_->IsFullyOpaque();
}

} // namespace ggadget
