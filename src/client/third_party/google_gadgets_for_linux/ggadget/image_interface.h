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

#ifndef GGADGET_IMAGE_INTERFACE_H__
#define GGADGET_IMAGE_INTERFACE_H__

#include <string>
#include <ggadget/color.h>
#include <ggadget/small_object.h>

namespace ggadget {

class CanvasInterface;

/**
 * @ingroup Interfaces
 * This class is the interface abstracting an image object, which might be
 * implemented by system dependent technology.
 */
class ImageInterface : public SmallObject<> {
 protected:
  /**
   * Disallow direct deletion.
   */
  virtual ~ImageInterface() { }

 public:
  /**
   * Frees this ImageInterface object.
   */
  virtual void Destroy() = 0;

  /**
   * Get the canvas containing the image data.
   * The returned canvas is owned by the image and might be changed
   * subsequently. So the caller shall not keep the canvas nor change it.
   */
  virtual const CanvasInterface *GetCanvas() const = 0;

  /**
   * Draw the image on a canvas without stretching.
   */
  virtual void Draw(CanvasInterface *canvas, double x, double y) const = 0;

  /**
   * Draw the image on a canvas stretching the image to given width and height.
   */
  virtual void StretchDraw(CanvasInterface *canvas,
                           double x, double y,
                           double width, double height) const = 0;

  /**
   * Get the width of the image, in pixel.
   */
  virtual double GetWidth() const = 0;

  /**
   * Get the height of the image, in pixel.
   */
  virtual double GetHeight() const = 0;

  /**
   * Creates a new image in which each pixel equals the color multiplied
   * with corresponding pixel in the current image.
   *
   * The result color if a pixel is calculated with following rules:
   * (value range: 0 ~ 1)
   * result.red = original.red * color.red * 2;
   * result.green = original.green * color.green * 2;
   * result.blue = original.blue * color.blue * 2;
   *
   * So when color equals to Color(0.5, 0.5, 0.5), the image won't be changed.
   * It allows to make the image not only darker but also lighter.
   */
  virtual ImageInterface *MultiplyColor(const Color &color) const = 0;

  /**
   * Gets the value of a point at a specified coordinate in the image.
   *
   * @param x X-coordinate of the point.
   * @param y Y-coordinate of the point.
   * @param[out] color Color of the specified point. It can be NULL.
   * @param[out] opacity Opacity of the specified point. It can be NULL.
   * @return true if the point value is accessible and returned successfully.
   *         otherwise, returns false.
   */
  virtual bool GetPointValue(double x, double y,
                             Color *color, double *opacity) const = 0;

  /**
   * Returns the string tag specified when loading the image.
   */
  virtual std::string GetTag() const = 0;

  /**
   * Checks if an Image is fully opaque, that is: the image has no alpha
   * channel, or there is no pixel which has non-opaque alpha value.
   */
  virtual bool IsFullyOpaque() const = 0;
};

/**
 * Handy function to destroy an image.
 */
inline void DestroyImage(ImageInterface *image) {
  if (image) image->Destroy();
}

/**
 * Handy function to get a tag of an image,
 * taking care of the null pointer issue.
 */
inline std::string GetImageTag(ImageInterface *image) {
  return image ? image->GetTag() : "";
}

} // namespace ggadget

#endif // GGADGET_IMAGE_INTERFACE_H__
