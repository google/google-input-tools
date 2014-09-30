/*
  Copyright 2014 Google Inc.

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
#ifndef GOOPY_COMPONENTS_UI_UI_TYPES_H_
#define GOOPY_COMPONENTS_UI_UI_TYPES_H_

#ifdef OS_WIN
#include "windows.h"
#endif // OS_WIN
#ifdef OS_MACOSX
#include <ApplicationServices/ApplicationServices.h>
#endif

namespace ime_goopy {
namespace components {

template <typename T>
struct Point {
 public:
  T x;
  T y;

  Point() : x(0),
            y(0) {
  }

  Point(T x1, T y1) : x(x1),
                      y(y1) {
  }

#ifdef OS_WIN
  Point(POINT p) : x(p.x),
                   y(p.y) {
  }

  POINT toPOINT() const {
    POINT point = {x, y};
    return point;
  }
#endif // OS_WIN

#ifdef OS_MACOSX
  Point(CGPoint p) : x(p.x),
                     y(p.y) {
  }

  CGPoint toCGPoint() const {
    return CGPointMake(x, y);
  }
#endif // OS_MACOSX
};

template <typename T>
struct Size {
 public:
  T width;
  T height;

  Size() : width(0),
           height(0) {
  }

  Size(T width1, T height1) : width(width1),
                              height(height1) {
  }

#ifdef OS_WIN
  explicit Size(SIZE size) : width(size.cx),
                             height(size.cy) {
  }

  SIZE toSIZE() const {
    SIZE size = {width, height};
    return size;
  }
#endif // OS_WIN

#ifdef OS_MACOSX
  explicit Size(CGSize size) : width(size.width),
                               height(size.height) {
  }

  CGSize toCGSize() const {
    return CGSizeMake(width, height);
  }
#endif // OS_MACOSX

};

template <typename T>
struct Rect {
 public:
  T x;
  T y;
  T width;
  T height;

  Rect() : x(0),
           y(0),
           width(0),
           height(0) {
  }

  explicit Rect(const Point<T> &origin, const Size<T> &size)
      : x(origin.x),
        y(origin.y),
        width(size.width),
        height(size.height) {
  }

  Rect(T x1, T y1, T width1, T height1) : x(x1),
                                          y(y1),
                                          width(width1),
                                          height(height1) {
  }

#ifdef OS_WIN
  explicit Rect(RECT rect) : x(rect.left),
                             y(rect.top),
                             width(rect.right - rect.left),
                             height(rect.bottom - rect.top) {
  }

  RECT toRECT() const {
    RECT rect;
    rect.left = x;
    rect.right = x + width;
    rect.top = y,
    rect.bottom = y + width;
    return rect;
  }
#endif // OS_WIN

#ifdef OS_MACOSX
  explicit Rect(CGRect rect) : x(rect.origin.x),
                               y(rect.origin.y),
                               width(rect.size.width),
                               height(rect.size.height) {
  }

  CGRect toCGRect() const {
    return CGRectMake(x, y, width, height);
  }
#endif // OS_MACOSX

  void SetValue(T x1, T y1, T width1, T height1) {
    x = x1;
    y = y1;
    width = width1;
    height = height1;
  }
};

} // namespace components
} // namespace ime_goopy

#endif // GOOPY_COMPONENTS_UI_UI_TYPES_H_
