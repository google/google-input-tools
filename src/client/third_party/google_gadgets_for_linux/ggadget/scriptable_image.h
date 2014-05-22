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

#ifndef GGADGET_SCRIPTABLE_IMAGE_H__
#define GGADGET_SCRIPTABLE_IMAGE_H__

#include <ggadget/scriptable_helper.h>

namespace ggadget {

class ImageInterface;

/**
 * @ingroup ScriptableObjects
 *
 * Scriptable decorator for ImageInterface.
 */
class ScriptableImage : public ScriptableHelperDefault {
 public:
  DEFINE_CLASS_ID(0x18d1431907cd4b1e, ScriptableInterface)

  /** This object takes the ownership of the input image. */
  ScriptableImage(ImageInterface *image);

 protected:
  virtual ~ScriptableImage();
  virtual void DoClassRegister();

 public:
  const ImageInterface *GetImage() const;
  void DestroyImage();

 private:
  DISALLOW_EVIL_CONSTRUCTORS(ScriptableImage);
  class Impl;
  Impl *impl_;
};

} // namespace ggadget

#endif  // GGADGET_SCRIPTABLE_Image_H__
