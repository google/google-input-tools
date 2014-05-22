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
#include <map>
#include <algorithm>
#include "common.h"
#include "file_manager_factory.h"
#include "format_macros.h"
#include "graphics_interface.h"
#include "image_cache.h"
#include "logger.h"
#include "main_loop_interface.h"
#include "small_object.h"
#include "system_utils.h"

#if defined(OS_WIN)
#include "win32/thread_local_singleton_holder.h"
#endif // OS_WIN

#ifdef _DEBUG
#define DEBUG_IMAGE_CACHE
#endif

namespace {

const int kPurgeTrashInterval = 60000;  // 60 seconds

}  // namespace

namespace ggadget {

class ImageCache::Impl : public WatchCallbackInterface {
  class SharedImage;
  typedef LightMap<std::string, SharedImage *> ImageMap;
  typedef LightMap<std::string, ImageInterface *> TrashImageMap;

  class SharedImage : public ImageInterface {
   public:
    SharedImage(Impl *owner,
                const std::string &key,
                const std::string &tag,
                ImageInterface *image,
                bool is_mask)
        : owner_(owner),
          key_(key),
          tag_(tag),
          image_(image),
          is_mask_(is_mask),
          ref_(1) {
    }
    virtual ~SharedImage() {
#ifdef DEBUG_IMAGE_CACHE
      DLOG("Destroy image %s", key_.c_str());
#endif
      if (owner_)
        owner_->Trash(key_, image_, is_mask_);
      else if (image_)
        image_->Destroy();
    }
    void Ref() {
      ASSERT(ref_ >= 0);
      ++ref_;
    }
    void Unref() {
      ASSERT(ref_ > 0);
      --ref_;
      if (ref_ == 0)
        delete this;
    }
    virtual void Destroy() {
      Unref();
    }
    virtual const CanvasInterface *GetCanvas() const {
      return image_ ? image_->GetCanvas() : NULL;
    }
    virtual void Draw(CanvasInterface *canvas, double x, double y) const {
      if (image_)
        image_->Draw(canvas, x, y);
    }
    virtual void StretchDraw(CanvasInterface *canvas,
                             double x, double y,
                             double width, double height) const {
      if (image_)
        image_->StretchDraw(canvas, x, y, width, height);
    }
    virtual double GetWidth() const {
      return image_ ? image_->GetWidth() : 0;
    }
    virtual double GetHeight() const {
      return image_ ? image_->GetHeight() : 0;
    }
    virtual ImageInterface *MultiplyColor(const Color &color) const {
      if (!image_)
        return NULL;
      // TODO: share images with color multiplied.

      // Reture self if the color is white.
      if (color == Color::kMiddleColor) {
        SharedImage *img = const_cast<SharedImage *>(this);
        img->Ref();
        return img;
      }

      return image_->MultiplyColor(color);
    }
    virtual bool GetPointValue(double x, double y,
                               Color *color, double *opacity) const {
      return image_ ? image_->GetPointValue(x, y, color, opacity) : false;
    }
    virtual std::string GetTag() const {
      return tag_;
    }
    virtual bool IsFullyOpaque() const {
      return image_ ? image_->IsFullyOpaque() : false;
    }
   public:
    Impl *owner_;
    std::string key_;
    std::string tag_;
    ImageInterface *image_;
    bool is_mask_;
    int ref_;
  };

 public:
  Impl() : ref_(0), watch_id_(-1) {
#ifdef DEBUG_IMAGE_CACHE
    DLOG("Create ImageCache: %p", this);
    num_new_local_images_ = 0;
    num_shared_local_images_ = 0;
    num_new_global_images_ = 0;
    num_shared_global_images_ = 0;
    num_trashed_images_ = 0;
    num_untrashed_images_ = 0;
#endif
    MainLoopInterface *main_loop = GetGlobalMainLoop();
    if (main_loop)
      watch_id_ = main_loop->AddTimeoutWatch(kPurgeTrashInterval, this);
  }

  ~Impl() {
    MainLoopInterface *main_loop = GetGlobalMainLoop();
    if (main_loop)
      main_loop->RemoveWatch(watch_id_);

#ifdef DEBUG_IMAGE_CACHE
    DLOG("Delete ImageCache: %p", this);
    DLOG("Image statistics(new/shared): "
         "local: %d/%d, global: %d/%d, remained: %"PRIuS
         " trashed/untrashed: %d/%d",
         num_new_local_images_, num_shared_local_images_,
         num_new_global_images_, num_shared_global_images_,
         images_.size() + mask_images_.size(),
         num_trashed_images_, num_untrashed_images_);
#endif
    for (ImageMap::const_iterator it = images_.begin();
         it != images_.end(); ++it) {
      DLOG("!!! Image leak: %s", it->first.c_str());
      // Detach the leaked image.
      it->second->owner_ = NULL;
    }

    for (ImageMap::const_iterator it = mask_images_.begin();
         it != mask_images_.end(); ++it) {
      DLOG("!!! Mask image leak: %s", it->first.c_str());
      it->second->owner_ = NULL;
    }

    PurgeTrashCan();
  }

  // Overridden from WatchCallbackInterface:
  virtual bool Call(MainLoopInterface *main_loop, int watch_id) {
    GGL_UNUSED(main_loop);
    GGL_UNUSED(watch_id);
    PurgeTrashCan();
    return true;
  }

  virtual void OnRemove(MainLoopInterface *main_loop, int watch_id) {
    GGL_UNUSED(main_loop);
    GGL_UNUSED(watch_id);
  }

  ImageInterface *LoadImage(GraphicsInterface *gfx, FileManagerInterface *fm,
                            const std::string &filename, bool is_mask) {
    if (!gfx || filename.empty())
      return NULL;

    FileManagerInterface *global_fm = GetGlobalFileManager();
    ImageMap *image_map = is_mask ? &mask_images_ : &images_;
    ImageMap::const_iterator it;
    std::string local_key;
    std::string global_key;

    // Find image in cache first.
    if (fm) {
      local_key = fm->GetFullPath(filename.c_str());
      it = image_map->find(local_key);
      if (it != image_map->end()) {
#ifdef DEBUG_IMAGE_CACHE
        num_shared_local_images_++;
        DLOG("Local image %s found in cache.", local_key.c_str());
#endif
        it->second->Ref();
        return it->second;
      }
    }

    if (global_fm) {
      global_key = global_fm->GetFullPath(filename.c_str());
      it = image_map->find(global_key);
      if (it != image_map->end()) {
#ifdef DEBUG_IMAGE_CACHE
        num_shared_global_images_++;
        DLOG("Global image %s found in cache.", global_key.c_str());
#endif
        it->second->Ref();
        return it->second;
      }
    }

    // The image was not loaded yet.
    ImageInterface *img = NULL;

    // Find the image in trash can first.
    if (fm) {
      img = Untrash(local_key, is_mask);
      if (img)
        return NewSharedImage(local_key, filename, img, is_mask);
    }
    if (global_fm) {
      img = Untrash(global_key, is_mask);
      if (img)
        return NewSharedImage(global_key, filename, img, is_mask);
    }

    std::string data;
    std::string key;
    if (fm && fm->ReadFile(filename.c_str(), &data)) {
      key = local_key;
      img = gfx->NewImage(filename, data, is_mask);
#ifdef DEBUG_IMAGE_CACHE
      DLOG("Local image %s loaded.", key.c_str());
      num_new_local_images_++;
#endif
    } else if (global_fm && global_fm->ReadFile(filename.c_str(), &data)) {
      key = global_key;
      img = gfx->NewImage(filename, data, is_mask);
#ifdef DEBUG_IMAGE_CACHE
      DLOG("Global image %s loaded.", key.c_str());
      num_new_global_images_++;
#endif
    } else {
      // Use the local key to let later requests to this file get the blank
      // image directly.
      key = local_key;
      DLOG("Failed to load image %s.", filename.c_str());
      // Continue. May still return a SharedImage because the gadget wants
      // the src of an image even if the image can't be loaded.
    }

    if (IsAbsolutePath(filename.c_str())) {
      // Don't cache files loaded with absolute file path, because the gadget
      // might want to load the new file when the file changes.
      return img ? img : new SharedImage(NULL, key, filename, NULL, is_mask);
    }

    return NewSharedImage(key, filename, img, is_mask);
  }

  ImageInterface *NewSharedImage(const std::string &key,
                                 const std::string &tag,
                                 ImageInterface *image,
                                 bool is_mask) {
    ImageMap *image_map = is_mask ? &mask_images_ : &images_;
    SharedImage *shared_img =
        new SharedImage(this, key, tag, image, is_mask);
    (*image_map)[key] = shared_img;
    return shared_img;
  }

  void Trash(const std::string &key, ImageInterface *image, bool is_mask) {
    ImageMap *images = is_mask ? &mask_images_ : &images_;
    images->erase(key);

    if (!image)
      return;

#ifdef DEBUG_IMAGE_CACHE
    DLOG("Trash image: %s", key.c_str());
    num_trashed_images_++;
#endif
    TrashImageMap *trash = is_mask ? &trashed_mask_images_ : &trashed_images_;
    ASSERT(!trash->count(key));
    (*trash)[key] = image;
  }

  ImageInterface* Untrash(const std::string &key, bool is_mask) {
    TrashImageMap *trash = is_mask ? &trashed_mask_images_ : &trashed_images_;
    TrashImageMap::iterator i = trash->find(key);
    if (i != trash->end()) {
#ifdef DEBUG_IMAGE_CACHE
      DLOG("Untrash image: %s", key.c_str());
      num_untrashed_images_++;
#endif
      ImageInterface *image = i->second;
      trash->erase(i);
      return image;
    }
    return NULL;
  }

  void PurgeTrashCan() {
#ifdef DEBUG_IMAGE_CACHE
    DLOG("Purge trashed images: %"PRIuS,
         trashed_images_.size() + trashed_mask_images_.size());
#endif
    for (TrashImageMap::const_iterator it = trashed_images_.begin();
         it != trashed_images_.end(); ++it) {
      if (it->second)
        it->second->Destroy();
    }
    trashed_images_.clear();

    for (TrashImageMap::const_iterator it = trashed_mask_images_.begin();
         it != trashed_mask_images_.end(); ++it) {
      if (it->second)
        it->second->Destroy();
    }
    trashed_mask_images_.clear();
  }

  void Ref() {
    ASSERT(ref_ >= 0);
    ASSERT(this == GetGlobal());
    ++ref_;
  }

  void Unref() {
    ASSERT(ref_ > 0);
    ASSERT(this == GetGlobal());
    --ref_;
    if (ref_ == 0) {
      SetGlobal(NULL);
      delete this;
    }
  }

  static Impl *Get() {
    Impl* impl = GetGlobal();
    if (!impl) {
      impl = new Impl();
      SetGlobal(impl);
    }
    impl->Ref();
    return impl;
  }

  static Impl *GetGlobal() {
#if defined(OS_WIN)
    return win32::ThreadLocalSingletonHolder<Impl>::GetValue();
#else
    return global_impl;
#endif
  }

  static void SetGlobal(Impl* impl) {
#if defined(OS_WIN)
    win32::ThreadLocalSingletonHolder<Impl>::SetValue(impl);
#else
    global_impl = impl;
#endif
  }

 private:
  ImageMap images_;
  ImageMap mask_images_;

  TrashImageMap trashed_images_;
  TrashImageMap trashed_mask_images_;

  int ref_;
  int watch_id_;

#ifdef DEBUG_IMAGE_CACHE
  int num_new_local_images_;
  int num_shared_local_images_;
  int num_new_global_images_;
  int num_shared_global_images_;
  int num_trashed_images_;
  int num_untrashed_images_;
#endif

#if !defined(OS_WIN)
 private:
  static Impl *global_impl;
#endif
};

#if !defined(OS_WIN)
ImageCache::Impl *ImageCache::Impl::global_impl = NULL;
#endif

ImageCache::ImageCache() : impl_(Impl::Get()) {
}

ImageCache::~ImageCache() {
  impl_->Unref();
}

ImageInterface *ImageCache::LoadImage(GraphicsInterface *gfx,
                                      FileManagerInterface *fm,
                                      const std::string &filename,
                                      bool is_mask) {
  return impl_->LoadImage(gfx, fm, filename, is_mask);
}

} // namespace ggadget
