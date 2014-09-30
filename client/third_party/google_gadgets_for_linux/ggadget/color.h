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

#ifndef GGADGET_COLOR_H__
#define GGADGET_COLOR_H__

#include <cmath>
#include <ggadget/common.h>
#include <ggadget/string_utils.h>

namespace ggadget {

/**
 * @ingroup Utilities
 * Struct for holding color information.
 * Currently, there is no support for the alpha channel.
 */
struct Color {
  Color() : red(0), green(0), blue(0) {
  }

  Color(double r, double g, double b) : red(r), green(g), blue(b) {
    ASSERT(r >= 0. && r <= 1.);
    ASSERT(g >= 0. && g <= 1.);
    ASSERT(b >= 0. && b <= 1.);
  };

  explicit Color(const char *name) : red(0), green(0), blue(0) {
    ASSERT(name);
    FromString(name, this, NULL);
  }

  Color(const Color &c) : red(c.red), green(c.green), blue(c.blue) {}

  bool operator==(const Color &c) const {
    return red == c.red && green == c.green && blue == c.blue;
  }

  bool operator!=(const Color &c) const {
    return red != c.red || green != c.green || blue != c.blue;
  }

  int RedInt() const {
    return static_cast<int>(round(red * 255));
  }

  int GreenInt() const {
    return static_cast<int>(round(green * 255));
  }

  int BlueInt() const {
    return static_cast<int>(round(blue * 255));
  }

  /**
   * Convert a color into a HTML-style color name string.
   */
  std::string ToString() const;

  /**
   * Utility function to create a Color object from 8-bit color channel values.
   */
  static Color FromChars(unsigned char r, unsigned char g, unsigned char b);

  /**
   * Parses a color name.
   * @param name the color name in HTML-style color format ("#rrggbb"),
   *     HTML-style with alpha ("#aarrggbb"), or using SVG color names
   *     (http://www.w3.org/TR/SVG/types.html#ColorKeywords).
   *     If @a alpha is @c NULL, only color name or "#rrggbb" format is
   *     allowed. If in #rrggbb or #aarrggbb format, any characters not
   *     in hexadecimal character range will be treated as '0'.
   * @param[out] color (required) the parsed color.
   * @param[out] alpha (optional) the parsed alpha value.
   * @return @c true if the format is valid.
   */
  static bool FromString(const char *name, Color *color, double *alpha);

  /** Common color constant. */
  static const Color kWhite;
  static const Color kBlack;

  /** Color(0.5, 0.5, 0.5) */
  static const Color kMiddleColor;

  double red, green, blue;
};

} // namespace ggadget

#endif // GGADGET_COLOR_H__
