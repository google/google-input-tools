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

#ifndef GGADGET_FONT_INTERFACE_H__
#define GGADGET_FONT_INTERFACE_H__
#include <ggadget/small_object.h>

namespace ggadget {

/**
 * @ingroup Interfaces
 * Class representing a font as understood by the graphics interface.
 * It is created by an associated GraphicsInterface object.
 * Call @c Destroy() to free this object once it is no longer needed.
 */
class FontInterface : public SmallObject<> {
 protected:
  /**
   * Disallow direct deletion.
   */
  virtual ~FontInterface() { }

 public:
  /**
   * Enum used to specify font styles.
   */
  enum Style {
    STYLE_NORMAL,
    STYLE_ITALIC
  };

  /**
   * Enum used to specify font weight.
   */
  enum Weight {
    WEIGHT_NORMAL,
    WEIGHT_BOLD
  };

  /**
   * @return The style option associated with the font.
   */
  virtual Style GetStyle() const = 0;

  /**
   * @return The style option associated with the font.
   */
  virtual Weight GetWeight() const = 0;

  /**
   * @return The size of the font, in points.
   */
  virtual double GetPointSize() const = 0;

  /**
   * Frees the FontInterface object.
   */
  virtual void Destroy() = 0;
};

} // namespace ggadget

#endif // GGADGET_FONT_INTERFACE_H__
