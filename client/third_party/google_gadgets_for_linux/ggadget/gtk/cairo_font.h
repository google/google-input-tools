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

#ifndef GGADGET_GTK_CAIRO_FONT_H__
#define GGADGET_GTK_CAIRO_FONT_H__

#include <pango/pango.h>
#include <ggadget/font_interface.h>

namespace ggadget {
namespace gtk {

/**
 * A Cairo/Pango-based implementation of the FontInterface. Internally,
 * this class wraps a PangoFontDescription object.
 */
class CairoFont : public FontInterface {
 public:
  /**
   * Constructor for CairoFont. Takes a PangoFontDescription object and its
   * ownership. Will free the PangoFontDescription object on destruction.
   */
  CairoFont(PangoFontDescription *font, double size, Style style,
            Weight weight);
  virtual ~CairoFont();

  virtual Style GetStyle() const { return style_; };
  virtual Weight GetWeight() const { return weight_; };
  virtual double GetPointSize() const { return size_; };

  virtual void Destroy() { delete this; };

  const PangoFontDescription *GetFontDescription() const { return font_; };

 private:
  PangoFontDescription *font_;
  double size_;
  Style style_;
  Weight weight_;
};

} // namespace gtk
} // namespace ggadget

#endif // GGADGET_GTK_CAIRO_FONT_H__
