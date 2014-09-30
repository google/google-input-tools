/*
  Copyright 2011 Google Inc.

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

#ifndef GGADGET_WIN32_GDIPLUS_GRAPHICS_H_
#define GGADGET_WIN32_GDIPLUS_GRAPHICS_H_

#include <string>
#include <ggadget/common.h>
#ifdef DrawText
#undef DrawText
#endif
#include <ggadget/graphics_interface.h>
#include <ggadget/slot.h>
#include <ggadget/scoped_ptr.h>
#include <ggadget/text_formats.h>
#include <ggadget/text_renderer_interface.h>

namespace Gdiplus {
class Font;
}  // namespace Gdiplus

namespace ggadget {
namespace win32 {

class PrivateFontDatabase;

// This class realizes the CanvasInterface using the Gdi+ graphics library.
// Internally, the graphics state is represented by a Gdiplus graphics object.
// The owner of this object should set any necessary Graphics properties before
// passing the graphics to the constructor. This may include operator, clipping,
// set initial matrix settings, and clear the drawing surface.

class GdiplusGraphics : public GraphicsInterface {
 public:
  // Constructs a GdiplusGraphics object
  // @param zoom The zoom level for all new canvases.
  // @param private_font_database A PrivateFontDatabase object that mananges all
  //     the private fonts in gadget.
  GdiplusGraphics(double zoom,
                  const PrivateFontDatabase* private_font_database);
  virtual ~GdiplusGraphics();
  virtual CanvasInterface* NewCanvas(double w, double h) const;
  virtual ImageInterface* NewImage(const std::string& tag,
                                   const std::string& data,
                                   bool is_mask) const;
  virtual FontInterface* NewFont(const std::string& family,
                                 double pt_size,
                                 FontInterface::Style style,
                                 FontInterface::Weight weight) const;
  virtual TextRendererInterface* NewTextRenderer() const;
  virtual void SetZoom(double zoom);
  virtual double GetZoom() const;
  virtual Connection* ConnectOnZoom(Slot1<void, double>* slot) const;
  void SetFontScale(double scale);
  double GetFontScale() const;
  const PrivateFontDatabase* GetFontDatabase() const;

  // Creates a GDI+ font for rendering glyphs.
  Gdiplus::Font* CreateFont(const TextFormat* format) const;

  // Creates a GDI font object for shaping and placing glyphs.
  HFONT CreateHFont(const TextFormat* format) const;

 private:
  class Impl;
  scoped_ptr<Impl> impl_;
  DISALLOW_EVIL_CONSTRUCTORS(GdiplusGraphics);
};

}  // namespace win32
}  // namespace ggadget

#endif  // GGADGET_WIN32_GDIPLUS_GRAPHICS_H_
