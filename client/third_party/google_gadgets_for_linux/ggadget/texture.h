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

#ifndef GGADGET_TEXTURE_H__
#define GGADGET_TEXTURE_H__

#include <ggadget/basic_element.h>
#include <ggadget/scriptable_helper.h>
// Undefine the annoying windows macro "#define DrawText DrawTextW"
#undef DrawText
#include <ggadget/canvas_interface.h>
#include <ggadget/image_interface.h>
#include <ggadget/small_object.h>

namespace ggadget {

class FontInterface;
class TextRendererInterface;

/**
 * @ingroup Utilities
 *
 * A helper class to handle color or image texture.
 */
class Texture : public SmallObject<> {
 public:
  /**
   * Creates a texture with an image. Ownership of the specified image will be
   * assumed by the texture.
   */
  Texture(ImageInterface *image);

  /**
   * Creates a new texture from a given color and opacity.
   */
  Texture(const Color &color, double opacity);

  ~Texture();

  /**
   * Draws the texture onto a canvas.
   * If the texture is an image, the image is repeated to fill the specified
   * area.
   */
  void Draw(CanvasInterface *canvas, double x, double y,
            double width, double height) const;

  /**
   * Draws the specified text on canvas.
   */
  void DrawText(CanvasInterface *canvas, double x, double y, double width,
                double height, const char *text, const FontInterface *f,
                CanvasInterface::Alignment align,
                CanvasInterface::VAlignment valign,
                CanvasInterface::Trimming trimming,
                int text_flags) const;

  /**
   * Draws the formatted text specified in renderer on canvas.
   */
  void DrawText(CanvasInterface *canvas,
                TextRendererInterface *renderer) const;

  /**
   * @return the file name this texture is loaded from a file; returns the
   *     color name if this texture is a color; otherwise returns an empty
   *     string.
   */
  std::string GetSrc() const;

  /** Utility function to get the src of a texture which can be @c NULL. */
  static std::string GetSrc(const Texture *texture) {
    return texture ? texture->GetSrc() : "";
  }

  const ImageInterface *GetImage() const;

  /**
   * Check if the texture is fully opaque, that is:
   *  - For color texture, the opacity equals to 1.0
   *  - For image texture, there is no alpha channel.
   */
  bool IsFullyOpaque() const;

 private:
  class Impl;
  Impl *impl_;

  DISALLOW_EVIL_CONSTRUCTORS(Texture);
};

} // namespace ggadget

#endif // GGADGET_IMAGE_H__
