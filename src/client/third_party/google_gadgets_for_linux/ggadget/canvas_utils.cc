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

#include "canvas_utils.h"
#include "canvas_interface.h"
#include "graphics_interface.h"
#include "image_interface.h"
#include "logger.h"
#include "math_utils.h"

namespace ggadget {

typedef void (*DrawCanvasAreaFunc)(const CanvasInterface *src,
                                   double src_x, double src_y,
                                   double src_width, double src_height,
                                   const GraphicsInterface *graphics,
                                   CanvasInterface *dest,
                                   double dest_x, double dest_y,
                                   double dest_width, double dest_height);

/**
 * Draw the specified area in the source canvas on the destination canvas.
 * The source will be zoomed if necessary.
 */
static void StretchDrawCanvasArea(const CanvasInterface *src,
                                  double src_x, double src_y,
                                  double src_width, double src_height,
                                  const GraphicsInterface *graphics,
                                  CanvasInterface *dest,
                                  double dest_x, double dest_y,
                                  double dest_width, double dest_height) {
  if (src_width > 0 && src_height > 0 &&
      dest_width > 0 && dest_height > 0) {
    double cx = dest_width / src_width;
    double cy = dest_height / src_height;
    dest->PushState();
    Rectangle dest_rect(dest_x, dest_y, dest_width, dest_height);
    // Integerize to avoid gap between the areas drawn by
    // StretchMiddleDrawCanvas().
    dest_rect.Integerize(true);
    dest->IntersectRectClipRegion(dest_rect.x, dest_rect.y,
                                  dest_rect.w, dest_rect.h);
    dest->ScaleCoordinates(cx, cy);

    double draw_x = dest_x / cx - src_x;
    double draw_y = dest_y / cy - src_y;
    dest->DrawCanvas(draw_x, draw_y, src);
    dest->PopState();
  }
}

/**
 * Draw the specified area int the source canvas on the destination canvas.
 * The source will be tiled if necessary.
 */
static void TileDrawCanvasArea(const CanvasInterface *src,
                               double src_x, double src_y,
                               double src_width, double src_height,
                               const GraphicsInterface *graphics,
                               CanvasInterface *dest,
                               double dest_x, double dest_y,
                               double dest_width, double dest_height) {
  if (src_width <= 0 || src_height <= 0 ||
      dest_width <= 0 || dest_height <= 0) {
    DLOG("TileDrawCanvasArea: src_width = %.0f, src_height = %.0f, "
         "dest_width = %.0f, dest_height = %.0f",
         src_width, src_height, dest_width, dest_height);
    return;
  }

  if (src_width == dest_width && src_height == dest_height) {
    dest->PushState();
    dest->IntersectRectClipRegion(dest_x, dest_y, dest_width, dest_height);
    dest->DrawCanvas(dest_x - src_x, dest_y - src_y, src);
    dest->PopState();
  } else {
    CanvasInterface* temp_canvas = graphics->NewCanvas(src_width, src_height);
    temp_canvas->DrawCanvas(-src_x, -src_y, src);
    dest->DrawFilledRectWithCanvas(dest_x, dest_y, dest_width, dest_height,
                                   temp_canvas);
    temp_canvas->Destroy();
  }
}

static void DrawCanvasWithBorder(const CanvasInterface *src,
                                 CanvasInterface *dest,
                                 const GraphicsInterface *graphics,
                                 double x, double y,
                                 double width, double height,
                                 double left_border_width,
                                 double top_border_height,
                                 double right_border_width,
                                 double bottom_border_height,
                                 DrawCanvasAreaFunc draw_canvas_area) {
  ASSERT(src);
  ASSERT(dest);
  if (!src || !dest) {
    DLOG("DrawCanvasWithBorder: Invalid parameter");
    return;
  }

  double src_width = src->GetWidth();
  double src_height = src->GetHeight();
  if (src_width < 4 || src_height < 4 ||
      (left_border_width == 0 && top_border_height == 0 &&
       right_border_width == 0 && bottom_border_height == 0)) {
    (*draw_canvas_area)(src, 0, 0, src_width, src_height, graphics,
                        dest, x, y, width, height);
    return;
  }

  if (src && dest && src_width > 0 && src_height > 0) {
    if (src->GetWidth() == width && src->GetHeight() == height) {
      dest->DrawCanvas(x, y, src);
    } else {
      if (left_border_width < 0)
        left_border_width += floor(src_width / 2);
      if (right_border_width < 0)
        right_border_width += floor(src_width / 2);
      if (top_border_height < 0)
        top_border_height += floor(src_height / 2);
      if (bottom_border_height < 0)
        bottom_border_height += floor(src_height / 2);

      double total_border_width = left_border_width + right_border_width;
      double total_border_height = top_border_height + bottom_border_height;
      double src_middle_width = src_width - total_border_width;
      double src_middle_height = src_height - total_border_height;
      if (src_middle_width <= 0) {
        src_middle_width = src_width / 2;
        left_border_width = right_border_width = src_width / 2 - 1;
      }
      if (src_middle_height <= 0) {
        src_middle_height = src_width / 2;
        top_border_height = bottom_border_height = src_height / 2 - 1;
      }

      double dest_middle_width = width - total_border_width;
      double dest_middle_height = height - total_border_height;
      double dx1, dx2, dy1, dy2;
      if (dest_middle_width <= 0) {
        left_border_width = right_border_width = width / 2;
        dx1 = dx2 = x + width / 2;
        dest_middle_width = 0;
      } else {
        dx1 = x + left_border_width;
        dx2 = x + width - right_border_width;
      }
      if (dest_middle_height <= 0) {
        top_border_height = bottom_border_height = height / 2;
        dy1 = dy2 = y + height / 2;
        dest_middle_height = 0;
      } else {
        dy1 = y + top_border_height;
        dy2 = y + height - bottom_border_height;
      }
      double sx2 = src_width - right_border_width;
      double sy2 = src_height - bottom_border_height;

      (*draw_canvas_area)(src, 0, 0, left_border_width, top_border_height,
                          graphics,
                          dest, x, y, left_border_width, top_border_height);
      (*draw_canvas_area)(src, left_border_width, 0,
                          src_middle_width, top_border_height, graphics,
                          dest, dx1, y, dest_middle_width, top_border_height);
      (*draw_canvas_area)(src, sx2, 0, right_border_width, top_border_height,
                          graphics,
                          dest, dx2, y, right_border_width, top_border_height);

      (*draw_canvas_area)(src, 0, top_border_height,
                          left_border_width, src_middle_height, graphics,
                          dest, x, dy1, left_border_width, dest_middle_height);
      (*draw_canvas_area)(src, left_border_width, top_border_height,
                          src_middle_width, src_middle_height,
                          graphics,
                          dest, dx1, dy1,
                          dest_middle_width, dest_middle_height);
      (*draw_canvas_area)(src, sx2, top_border_height,
                          right_border_width, src_middle_height,
                          graphics,
                          dest, dx2, dy1,
                          right_border_width, dest_middle_height);

      (*draw_canvas_area)(src, 0, sy2, left_border_width, bottom_border_height,
                          graphics,
                          dest, x, dy2,
                          left_border_width, bottom_border_height);
      (*draw_canvas_area)(src, left_border_width, sy2,
                          src_middle_width, bottom_border_height, graphics,
                          dest, dx1, dy2,
                          dest_middle_width, bottom_border_height);
      (*draw_canvas_area)(src, sx2, sy2,
                          right_border_width, bottom_border_height,
                          graphics,
                          dest, dx2, dy2,
                          right_border_width, bottom_border_height);
    }
  }
}

void StretchMiddleDrawCanvas(const CanvasInterface *src, CanvasInterface *dest,
                             double x, double y, double width, double height,
                             double left_border_width,
                             double top_border_height,
                             double right_border_width,
                             double bottom_border_height) {
  DrawCanvasWithBorder(src, dest, NULL, x, y, width, height,
                       left_border_width,
                       top_border_height,
                       right_border_width,
                       bottom_border_height,
                       StretchDrawCanvasArea);
}

void StretchMiddleDrawImage(const ImageInterface *src, CanvasInterface *dest,
                            double x, double y, double width, double height,
                            double left_border_width,
                            double top_border_height,
                            double right_border_width,
                            double bottom_border_height) {
  ASSERT(src);
  ASSERT(dest);
  if (!src || !dest)
    return;

  double src_width = src->GetWidth();
  double src_height = src->GetHeight();
  if (src_width < 4 || src_height < 4 ||
      (left_border_width == 0 && top_border_height == 0 &&
       right_border_width == 0 && bottom_border_height == 0)) {
    src->StretchDraw(dest, x, y, width, height);
    return;
  }

  const CanvasInterface *src_canvas = src->GetCanvas();
  if (!src_canvas)
    return;

  DrawCanvasWithBorder(src_canvas, dest, NULL,
                       x, y, width, height,
                       left_border_width,
                       top_border_height,
                       right_border_width,
                       bottom_border_height,
                       StretchDrawCanvasArea);
}

void TileMiddleDrawCanvas(const CanvasInterface *src, CanvasInterface *dest,
                          const GraphicsInterface *graphics,
                          double x, double y, double width, double height,
                          double left_border_width,
                          double top_border_height,
                          double right_border_width,
                          double bottom_border_height) {
  DrawCanvasWithBorder(src, dest, graphics,
                       x, y, width, height,
                       left_border_width,
                       top_border_height,
                       right_border_width,
                       bottom_border_height,
                       StretchDrawCanvasArea);
}

void TileMiddleDrawImage(const ImageInterface *src, CanvasInterface *dest,
                         const GraphicsInterface *graphics,
                         double x, double y, double width, double height,
                         double left_border_width,
                         double top_border_height,
                         double right_border_width,
                         double bottom_border_height) {
  ASSERT(src);
  ASSERT(dest);
  ASSERT(graphics);
  if (!src || !dest || !graphics)
    return;

  const CanvasInterface *src_canvas = src->GetCanvas();
  if (!src_canvas)
    return;

  DrawCanvasWithBorder(src_canvas, dest,
                       graphics,
                       x, y, width, height,
                       left_border_width,
                       top_border_height,
                       right_border_width,
                       bottom_border_height,
                       TileDrawCanvasArea);
}

void MapStretchMiddleCoordDestToSrc(double dest_x, double dest_y,
                                    double src_width, double src_height,
                                    double dest_width, double dest_height,
                                    double left_border_width,
                                    double top_border_height,
                                    double right_border_width,
                                    double bottom_border_height,
                                    double *src_x, double *src_y) {
  ASSERT(src_x);
  ASSERT(src_y);

  if (left_border_width < 0)
    left_border_width += src_width / 2;
  if (right_border_width < 0)
    right_border_width += src_width / 2;
  if (top_border_height < 0)
    top_border_height += src_height / 2;
  if (bottom_border_height < 0)
    bottom_border_height += src_height / 2;

  if (dest_x < left_border_width) {
    *src_x = dest_x;
  } else if (dest_x < dest_width - right_border_width) {
    double total_border_width = left_border_width + right_border_width;
    if (dest_width > total_border_width && src_width > total_border_width) {
      double scale_x = (src_width - total_border_width) /
          (dest_width - total_border_width);
      *src_x = (dest_x - left_border_width) * scale_x + left_border_width;
    } else {
      *src_x = left_border_width;
    }
  } else {
    *src_x = dest_x - dest_width + src_width;
  }

  if (dest_y < top_border_height) {
    *src_y = dest_y;
  } else if (dest_y < dest_height - bottom_border_height) {
    double total_border_height = top_border_height + bottom_border_height;
    if (dest_height > total_border_height && src_height > total_border_height) {
      double scale_y = (src_height - total_border_height) /
          (dest_height - total_border_height);
      *src_y = (dest_y - top_border_height) * scale_y + top_border_height;
    } else {
      *src_y = top_border_height;
    }
  } else {
    *src_y = dest_y - dest_height + src_height;
  }
}

}  // namespace ggadget
