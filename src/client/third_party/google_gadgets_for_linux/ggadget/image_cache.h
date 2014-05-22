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

#ifndef GGADGET_IMAGE_CACHE_H__
#define GGADGET_IMAGE_CACHE_H__

#include <ggadget/graphics_interface.h>
#include <ggadget/image_interface.h>
#include <ggadget/file_manager_interface.h>

namespace ggadget {

/**
 * @ingroup Utilities
 *
 * A class to manipulate shared images.
 *
 * When loading two images with the same tag, they will be treated as identical
 * and share the same underlying object, to save memory usage.
 *
 * From the caller point of view, image objects created by ImageCache can be
 * used as normal image without any difference.
 *
 * Each View shall have its own ImageCache object.
 */
class ImageCache {
 public:
  ImageCache();
  ~ImageCache();

  /**
   * Loads an image from specified Graphics and FileManager objects..
   *
   * It supports loading both per-gadget images and global resource images.
   *
   * @param gfx Graphics object used to create the image.
   * @param fm FileManager object used to load the image data.
   * @param filename File name of the image.
   * @param is_mask If the image is a mask or not.
   */
  ImageInterface *LoadImage(GraphicsInterface *gfx, FileManagerInterface *fm,
                            const std::string &filename, bool is_mask);

 private:
  class Impl;
  Impl *impl_;
  DISALLOW_EVIL_CONSTRUCTORS(ImageCache);
};

} // namespace ggadget

#endif // GGADGET_IMAGE_CACHE_H__
