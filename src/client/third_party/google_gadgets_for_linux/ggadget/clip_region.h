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

#ifndef GGADGET_CLIP_REGION_H__
#define GGADGET_CLIP_REGION_H__

#include <ggadget/common.h>
#include <ggadget/math_utils.h>

namespace ggadget {

class Rectangle;
template <typename R, typename P1, typename P2, typename P3, typename P4>
    class Slot4;

/**
 * @ingroup Utilities
 * A class to represent a clip region, which consists of a set of rectangles.`
 *
 * A fuzzy ratio can be specified, so that two rectangles overlaping with
 * certain amount of extent will be merged into one larger rectangle. In some
 * situation, it could reduce the total number of clip rectangles a lot.
 *
 * The definition of fuzzy_ratio:
 * Set rectangle rect to the union of two overlapped rectangles a and b,
 * if (area of a) >= (area of rect) * fuzzy_ratio or
 *    (area of b) >= (area of rect) * fuzzy_ratio then use rect to replace a
 *    and b.
 *
 * The default fuzzy ratio is 1, means no merging at all. It must be greater
 * than 0.5.
 */
class ClipRegion {
 public:
  /** Default constructor, with fuzzy ratio = 1. */
  ClipRegion();

  /** Constructor with a specified fuzzy ratio. */
  explicit ClipRegion(double fuzzy_ratio);

  /** Copy constructor. Fuzzy ratio will also be copied. */
  ClipRegion(const ClipRegion &region);

  ~ClipRegion();

  /** Copy operator. Fuzzy ratio will also be copied. */
  const ClipRegion& operator=(const ClipRegion &region);

  /** Gets the fuzzy ratio. */
  double GetFuzzyRatio() const;

  /** Sets the fuzzy ratio. */
  void SetFuzzyRatio(double fuzzy_ratio);

  /**
   * Adds a rectangle into the region. Merge with available clip rectangles
   * when possible, according to the fuzzy ratio.
   */
  void AddRectangle(const Rectangle &rect);

  /**
   * Clear the region.
   */
  void Clear();

  /**
   * Checks if the region is empty.
   *
   * An empty clip region means that the extent of clip region is infinite.
   */
  bool IsEmpty() const;

  /**
   * Checks if a point is in the region.
   */
  bool IsPointIn(double x, double y) const;

  /** Checks if a rectange overlaps with the region. */
  bool Overlaps(const Rectangle &rect) const;

  /**
   * Checks if the whole region is inside a specified rectangle.
   *
   * An empty clip region is not inside any rectangle.
   */
  bool IsInside(const Rectangle &rect) const;

  /** Gets the extents of this region. */
  Rectangle GetExtents() const;

  /**
   * Integerize all rectangles in the region.
   */
  void Integerize();

  /** Zoom clip region by specified zoom factor. */
  void Zoom(double zoom);

  /**
   * Gets number of rectangles in this region.
   */
  size_t GetRectangleCount() const;

  /** Gets a rectangle in this region. */
  Rectangle GetRectangle(size_t index) const;

  /**
   * Enumerate the rectangles. The region could be represented by a list of
   * rectangles. This method offer a way to enumerate the rectangles.
   * @param slot the slot that handle each rectangle. The slot has one
   *        parameter which is a pointer point to the rectangle. The return
   *        type of the slot is boolean value, return @c true if it keep going
   *        handling the rectangles and @c false when it want to stop.
   * @return @true if all rectangles are handled and @false otherwise. If there
   *         is no rectangle in this clip region, then @false will be returned.
   */
  typedef Slot4<bool, double, double, double, double> RectangleSlot;
  bool EnumerateRectangles(RectangleSlot *slot) const;

  /** For debug purpose. */
  void PrintLog() const;

 private:
  class Impl;
  Impl *impl_;
};

}  // namespace ggadget

#endif  // GGADGET_CLIP_REGION_H__
