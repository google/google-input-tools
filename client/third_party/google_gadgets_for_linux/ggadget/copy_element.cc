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

#include <string>

#include "graphics_interface.h"
#include "copy_element.h"
#include "canvas_interface.h"
#include "view.h"
#include "small_object.h"

namespace ggadget {

class CopyElement::Impl : public SmallObject<> {
 public:
  Impl(CopyElement *owner)
    : owner_(owner),
      snapshot_(NULL),
      source_(NULL),
      refchange_connection_(NULL),
      update_connection_(NULL),
      frozen_(false) {
  }

  ~Impl() {
    // Clear the source.
    SetSrc(Variant());
    DestroyCanvas(snapshot_);
  }

  void OnSourceRefChange(int ref_count, int change) {
    GGL_UNUSED(ref_count);
    if (change == 0) {
      // The source's destructor is being called.
      refchange_connection_->Disconnect();
      refchange_connection_ = NULL;
      source_->Unref(true);
      source_ = NULL;
      UpdateSnapshot();
    }
  }

  void UpdateSnapshot() {
    if (frozen_) return;

    // Don't keep snapshot if there is no source or the source is invisible.
    if (!source_ || !source_->IsVisible()) {
      DestroyCanvas(snapshot_);
      snapshot_ = NULL;
      owner_->QueueDraw();
      return;
    }

    double width = source_->GetPixelWidth();
    double height = source_->GetPixelHeight();
    if (snapshot_ &&
        (snapshot_->GetWidth() != width || snapshot_->GetHeight() != height)) {
      DestroyCanvas(snapshot_);
      snapshot_ = NULL;
    }

    if (!snapshot_) {
      GraphicsInterface *gfx = owner_->GetView()->GetGraphics();
      if (gfx)
        snapshot_ = gfx->NewCanvas(width, height);
    }

    if (snapshot_) {
      snapshot_->ClearCanvas();
      owner_->GetView()->EnableClipRegion(false);
      source_->Draw(snapshot_);
      owner_->GetView()->EnableClipRegion(true);
    }

    owner_->QueueDraw();
  }

  void SetSrc(const Variant &src) {
    if (source_) {
      refchange_connection_->Disconnect();
      refchange_connection_ = NULL;
      update_connection_->Disconnect();
      update_connection_ = NULL;
      source_->Unref();
      source_ = NULL;
    }

    Variant::Type type = src.type();
    if (type == Variant::TYPE_STRING) {
      const char *src_name= VariantValue<const char*>()(src);
      if (src_name && *src_name) {
        source_ = owner_->GetView()->GetElementByName(src_name);
        // The source element is not added yet, save the name and try to load
        // it later.
        if (!source_)
          src_name_ = src_name;
      }
    } else if (type == Variant::TYPE_SCRIPTABLE) {
      ScriptableInterface *obj = VariantValue<ScriptableInterface *>()(src);
      if (obj && obj->IsInstanceOf(BasicElement::CLASS_ID))
        source_ = down_cast<BasicElement *>(obj);
    }

    if (source_) {
      source_->Ref();
      refchange_connection_ = source_->ConnectOnReferenceChange(
          NewSlot(this, &Impl::OnSourceRefChange));
      update_connection_ = source_->ConnectOnContentChanged(
          NewSlot(this, &Impl::UpdateSnapshot));

      UpdateSnapshot();
    } else if (snapshot_ && !frozen_) {
      DestroyCanvas(snapshot_);
      snapshot_ = NULL;
    }
  }

  Variant GetSrc() {
    return source_ ? Variant(source_->GetName()) : Variant();
  }

  CopyElement *owner_;
  CanvasInterface *snapshot_;
  BasicElement *source_;
  Connection *refchange_connection_;
  Connection *update_connection_;
  std::string src_name_;
  bool frozen_;
};

CopyElement::CopyElement(View *view, const char *name)
    : BasicElement(view, "copy", name, false),
      impl_(new Impl(this)) {
}

void CopyElement::DoClassRegister() {
  BasicElement::DoClassRegister();
  RegisterProperty("src",
                   NewSlot(&Impl::GetSrc, &CopyElement::impl_),
                   NewSlot(&Impl::SetSrc, &CopyElement::impl_));
}

CopyElement::~CopyElement() {
  delete impl_;
  impl_ = NULL;
}

bool CopyElement::IsFrozen() const {
  return impl_->frozen_;
}

void CopyElement::SetFrozen(bool frozen) {
  impl_->frozen_ = frozen;
  if (!frozen)
    impl_->UpdateSnapshot();
}

bool CopyElement::IsPointIn(double x, double y) const {
  // Return false directly if the point is outside the element boundary.
  if (!BasicElement::IsPointIn(x, y))
    return false;

  double opacity = 0;
  if (impl_->snapshot_) {
    x = x * impl_->snapshot_->GetWidth() / GetPixelWidth();
    y = y * impl_->snapshot_->GetHeight() / GetPixelHeight();

    if (!impl_->snapshot_->GetPointValue(x, y, NULL, &opacity))
      return true;
  }

  return opacity > 0;
}

void CopyElement::DoDraw(CanvasInterface *canvas) {
  if (impl_->snapshot_) {
    double w = GetPixelWidth();
    double h = GetPixelHeight();
    double sw = impl_->snapshot_->GetWidth();
    double sh = impl_->snapshot_->GetHeight();
    double cx = w / sw;
    double cy = h / sh;
    if (cx != 1 || cy != 1)
      canvas->ScaleCoordinates(cx, cy);
    canvas->DrawCanvas(0, 0, impl_->snapshot_);
  }
}

Variant CopyElement::GetSrc() const {
  return impl_->GetSrc();
}

void CopyElement::SetSrc(const Variant &src) {
  impl_->SetSrc(src);
}

void CopyElement::GetDefaultSize(double *width, double *height) const {
  ASSERT(width && height);

  // Try to set source again.
  if (!impl_->source_ && !impl_->src_name_.empty()) {
    impl_->SetSrc(Variant(impl_->src_name_));
    // Only try once.
    impl_->src_name_.clear();
  }

  if (impl_->frozen_) {
    *width = (impl_->snapshot_ ? impl_->snapshot_->GetWidth() : 0);
    *height = (impl_->snapshot_ ? impl_->snapshot_->GetHeight() : 0);
  } else {
    *width = (impl_->source_ ? impl_->source_->GetPixelWidth() : 0);
    *height = (impl_->source_ ? impl_->source_->GetPixelHeight() : 0);
  }
}

void CopyElement::MarkRedraw() {
  if (impl_->snapshot_) {
    DestroyCanvas(impl_->snapshot_);
    impl_->snapshot_ = NULL;
    impl_->UpdateSnapshot();
  }
}

bool CopyElement::HasOpaqueBackground() const {
  if (!impl_->frozen_ && impl_->source_)
    return impl_->source_->HasOpaqueBackground();
  return false;
}

BasicElement *CopyElement::CreateInstance(View *view, const char *name) {
  return new CopyElement(view, name);
}

double CopyElement::GetSrcWidth() const {
  if (impl_->source_)
    return impl_->source_->GetPixelWidth();
  if (impl_->snapshot_)
    return impl_->snapshot_->GetWidth();
  return 0;
}

double CopyElement::GetSrcHeight() const {
  if (impl_->source_)
    return impl_->source_->GetPixelHeight();
  if (impl_->snapshot_)
    return impl_->snapshot_->GetHeight();
  return 0;
}


} // namespace ggadget
