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

#ifndef GGADGET_IMG_ELEMENT_H__
#define GGADGET_IMG_ELEMENT_H__

#include <stdlib.h>
#include <ggadget/basic_element.h>

namespace ggadget {

/**
 * @ingroup Elements
 * Class of <a href=
 * "http://code.google.com/apis/desktop/docs/gadget_apiref.html#img">
 * img element</a>.
 */
class ImgElement : public BasicElement {
 public:
  DEFINE_CLASS_ID(0x95b5791e157d4373, BasicElement);

  ImgElement(View *view, const char *name);
  virtual ~ImgElement();

 protected:
  virtual void DoClassRegister();

 public:
  virtual bool IsPointIn(double x, double y) const;

 public:
  /** Possible values of img.cropMaintainAspect property. */
  enum CropMaintainAspect {
    CROP_FALSE = 0,
    CROP_TRUE,
    CROP_PHOTO
  };

  /** Gets the source of image to display. */
  Variant GetSrc() const;
  /**
   * Sets the source of image to display.
   * @see ViewInterface::LoadImage()
   */
  void SetSrc(const Variant &src);

  /** Gets the original width of the image being displayed. */
  double GetSrcWidth() const;

  /** Gets the original height of the image being displayed. */
  double GetSrcHeight() const;

  /** Gets the colorMultiply property of the image. */
  std::string GetColorMultiply() const;
  /** Sets the colorMultiply property of the image. */
  void SetColorMultiply(const char *color);

  /** Gets the cropMaintainAspect property of the image. */
  CropMaintainAspect GetCropMaintainAspect() const;
  /** Sets the cropMaintainAspect property of the image. */
  void SetCropMaintainAspect(CropMaintainAspect crop);

  /**
   * Gets whether the image is stretched normally or stretched only
   * the middle area.
   */
  bool IsStretchMiddle() const;
  /**
   * Sets whether the image is stretched normally or stretched only
   * the middle area. This property is only applicable if cropMaintainAspect
   * is @c CROP_FALSE.
   */
  void SetStretchMiddle(bool stretch_middle);

  /**
   * Resizes the image to specified @a width and @a height via reduced
   * resolution.
   * If the source image is larger than the display area,
   * using this method to resize the image to the output size will save
   * memory and improve rendering performance.
   */
  void SetSrcSize(double width, double height);

 public:
  static BasicElement *CreateInstance(View *view, const char *name);

 protected:
  virtual void DoDraw(CanvasInterface *canvas);
  virtual void GetDefaultSize(double *width, double *height) const;

 public:
  virtual bool HasOpaqueBackground() const;

 private:
  DISALLOW_EVIL_CONSTRUCTORS(ImgElement);

  class Impl;
  Impl *impl_;
};

} // namespace ggadget

#endif // GGADGET_IMG_ELEMENT_H__
