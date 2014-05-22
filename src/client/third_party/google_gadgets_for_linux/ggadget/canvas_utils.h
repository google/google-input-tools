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

#ifndef GGADGET_CANVAS_UTILS_H__
#define GGADGET_CANVAS_UTILS_H__

namespace ggadget {

class CanvasInterface;
class GraphicsInterface;
class ImageInterface;

/**
 * @defgroup CanvasUtilities Canvas utilities
 * @ingroup Utilities
 * @{
 */

/**
 * Draw the source canvas on the destination canvas with given width and height,
 * by stretching or tiling the middle area of the image and keeping the four
 * corners unchanged and the four borders only stretched or tiled in one
 * direction.
 *
 * If the border widths/heights are all 0, this method does nearly the same as
 * @c ImageInterface::StretchDraw() or
 * CanvasInterface::DrawFilledRectWithCanvas. if a border width or height is
 * negative, the middle area is calculated from the center of the image.
 */
void StretchMiddleDrawCanvas(const CanvasInterface *src, CanvasInterface *dest,
                             double x, double y, double width, double height,
                             double left_border_width,
                             double top_border_height,
                             double right_border_width,
                             double bottom_border_height);

void StretchMiddleDrawImage(const ImageInterface *src, CanvasInterface *dest,
                            double x, double y, double width, double height,
                            double left_border_width,
                            double top_border_height,
                            double right_border_width,
                            double bottom_border_height);

void TileMiddleDrawCanvas(const CanvasInterface *src, CanvasInterface *dest,
                          const GraphicsInterface *graphics,
                          double x, double y, double width, double height,
                          double left_border_width,
                          double top_border_height,
                          double right_border_width,
                          double bottom_border_height);

void TileMiddleDrawImage(const ImageInterface *src, CanvasInterface *dest,
                         const GraphicsInterface *graphics,
                         double x, double y, double width, double height,
                         double left_border_width,
                         double top_border_height,
                         double right_border_width,
                         double bottom_border_height);

/**
 * Maps the destination coordinates to the source coordinates if the source
 * is drawn by StretchMiddleDrawCanvas() or StretchMiddleDrawImage() with
 * the same border width and height parameters.
 */
void MapStretchMiddleCoordDestToSrc(double dest_x, double dest_y,
                                    double src_width, double src_height,
                                    double dest_width, double dest_height,
                                    double left_border_width,
                                    double top_border_height,
                                    double right_border_width,
                                    double bottom_border_height,
                                    double *src_x, double *src_y);

/** @} */

} // namespace ggadget

#endif // GGADGET_CANVAS_UTILS_H__
