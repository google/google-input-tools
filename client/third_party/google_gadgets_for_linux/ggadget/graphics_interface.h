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

#ifndef GGADGET_GRAPHICS_INTERFACE_H__
#define GGADGET_GRAPHICS_INTERFACE_H__

#include <string>
#include <ggadget/canvas_interface.h>
#include <ggadget/font_interface.h>
#include <ggadget/image_interface.h>
#include <ggadget/slot.h>
#include <ggadget/text_renderer_interface.h>

namespace ggadget {

struct Color;

/**
 * @ingroup Interfaces
 * This class is the interface for creating objects used in ggadget's
 * graphics rendering.
 * It's implementation should come bundled with a
 * corresponding implementation of CanvasInterface. The gadget view obtains
 * an instance of this class from its HostInterface. Unlike the HostInterface,
 * the host can decide, depending on requirements,
 * how to assign GraphicsInterface objects to Views. For example, the host may
 * choose to:
 * - use a different GraphicsInterface for each view
 * - use a different GraphicsInterface for each gadget, but share it amongst views
 * - use the same GraphicsInterface for all views in the process.
 */
class GraphicsInterface {
 public:
  virtual ~GraphicsInterface() { }

  /**
   * Creates a new blank canvas.
   * @param w Width of the new canvas.
   * @param h Height of the new canvas.
   */
  virtual CanvasInterface *NewCanvas(double w, double h) const = 0;

  /**
   * Creates a new image.
   * @param tag A string tag to the image. It can be anything, for example,
   *     the file name of the image. Images with the same tag will be treated
   *     as the same image.
   * @param data A string containing the raw bytes of the image. The size can
   *     be obtained from data.size().
   * @param is_mask Indicates if the image is a mask image.
   *     For mask image, only alpha channel will be used. And only pure
   *     black color will be treated as fully transparent.
   * @return NULL on error, an ImageInterface object otherwise.
   */
  virtual ImageInterface *NewImage(const std::string &tag,
                                   const std::string &data,
                                   bool is_mask) const = 0;

  /**
   * Create a new font. This font is used when rendering text to a canvas.
   */
  virtual FontInterface *NewFont(const std::string &family,
                                 double pt_size,
                                 FontInterface::Style style,
                                 FontInterface::Weight weight) const = 0;

  /**
   * Creates a new Text Renderer. This renderer is used for rendering formatted
   * text.
   */
  virtual TextRendererInterface *NewTextRenderer() const = 0;

  /** Gets and sets the current zoom level. */
  virtual void SetZoom(double zoom) = 0;
  virtual double GetZoom() const = 0;
  virtual Connection* ConnectOnZoom(Slot1<void, double>* slot) const = 0;
};

} // namespace ggadget

#endif // GGADGET_GRAPHICS_INTERFACE_H__
