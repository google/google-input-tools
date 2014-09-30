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

#include <cstdio>
#include <cmath>
#include "ggadget/common.h"
#include "ggadget/math_utils.h"
#include "unittest/gtest.h"

using namespace ggadget;

const double kErrorDelta = .00000001;

TEST(MathUtilsTest, ChildCoordCalculator) {
  double child_x, child_y;

  ChildCoordCalculator calc = ChildCoordCalculator(0, 0, 50, 50, M_PI_2);
  calc.Convert(0, 0, &child_x, &child_y);
  EXPECT_DOUBLE_EQ(child_x, calc.GetChildX(0, 0));
  EXPECT_DOUBLE_EQ(child_y, calc.GetChildY(0, 0));
  EXPECT_NEAR(50., child_x, kErrorDelta);
  EXPECT_DOUBLE_EQ(50., child_y);

  calc = ChildCoordCalculator(0, 0, 50, 50, M_PI);
  calc.Convert(0, 0, &child_x, &child_y);
  EXPECT_DOUBLE_EQ(child_x, calc.GetChildX(0, 0));
  EXPECT_DOUBLE_EQ(child_y, calc.GetChildY(0, 0));
  EXPECT_DOUBLE_EQ(50., child_x);
  EXPECT_DOUBLE_EQ(50., child_y);

  calc = ChildCoordCalculator(0, 0, 50, 50, M_PI + M_PI_2);
  calc.Convert(0, 0, &child_x, &child_y);
  EXPECT_DOUBLE_EQ(child_x, calc.GetChildX(0, 0));
  EXPECT_DOUBLE_EQ(child_y, calc.GetChildY(0, 0));
  EXPECT_DOUBLE_EQ(50., child_x);
  EXPECT_DOUBLE_EQ(50., child_y);

  calc = ChildCoordCalculator(0, 0, 50, 50, 2 * M_PI);
  calc.Convert(0, 0, &child_x, &child_y);
  EXPECT_DOUBLE_EQ(child_x, calc.GetChildX(0, 0));
  EXPECT_DOUBLE_EQ(child_y, calc.GetChildY(0, 0));
  EXPECT_DOUBLE_EQ(50., child_x);
  EXPECT_DOUBLE_EQ(50., child_y);

  ChildCoordCalculator calc2(0, 0, 0, 0, 0);
  for (int i = 0; i < 360; i++) {
    calc2.Convert(i, i, &child_x, &child_y);
    EXPECT_DOUBLE_EQ(child_x, calc2.GetChildX(i, i));
    EXPECT_DOUBLE_EQ(child_y, calc2.GetChildY(i, i));
    EXPECT_DOUBLE_EQ(i, child_x);
    EXPECT_DOUBLE_EQ(i, child_y);

    calc = ChildCoordCalculator(i, i, 0, 0, 0);
    calc.Convert(0, 0, &child_x, &child_y);
    EXPECT_DOUBLE_EQ(child_x, calc.GetChildX(0, 0));
    EXPECT_DOUBLE_EQ(child_y, calc.GetChildY(0, 0));
    EXPECT_DOUBLE_EQ(-i, child_x);
    EXPECT_DOUBLE_EQ(-i, child_y);

    calc = ChildCoordCalculator(0, 0, i, i, 0);
    calc.Convert(0, 0, &child_x, &child_y);
    EXPECT_DOUBLE_EQ(child_x, calc.GetChildX(0, 0));
    EXPECT_DOUBLE_EQ(child_y, calc.GetChildY(0, 0));
    EXPECT_DOUBLE_EQ(i, child_x);
    EXPECT_DOUBLE_EQ(i, child_y);

    // distance should be constant in a circular rotation around origin
    calc = ChildCoordCalculator(0, 0, 0, 0, DegreesToRadians(i));
    calc.Convert(100, 100, &child_x, &child_y);
    EXPECT_DOUBLE_EQ(child_x, calc.GetChildX(100, 100));
    EXPECT_DOUBLE_EQ(child_y, calc.GetChildY(100, 100));
    EXPECT_DOUBLE_EQ(20000., child_x * child_x + child_y * child_y);

    // distance should be constant in a circular rotation around top-left
    calc = ChildCoordCalculator(100, 100, 0, 0, DegreesToRadians(i));
    calc.Convert(0, 0, &child_x, &child_y);
    EXPECT_DOUBLE_EQ(child_x, calc.GetChildX(0, 0));
    EXPECT_DOUBLE_EQ(child_y, calc.GetChildY(0, 0));
    EXPECT_DOUBLE_EQ(20000., child_x * child_x + child_y * child_y);

    // distance to pin should be constant in a circular rotation
    calc = ChildCoordCalculator(0, 0, 1, 1, DegreesToRadians(i));
    calc.Convert(0, 0, &child_x, &child_y);
    EXPECT_DOUBLE_EQ(child_x, calc.GetChildX(0, 0));
    EXPECT_DOUBLE_EQ(child_y, calc.GetChildY(0, 0));
    EXPECT_NEAR(0., (child_x - 1) * (child_x - 1) +
                    (child_y - 1) * (child_y - 1), kErrorDelta);
  }
}

TEST(MathUtilsTest, GetChildCoord) {
  double child_x, child_y;

  ParentCoordToChildCoord(0, 0, 0, 0, 50, 50, 0, &child_x, &child_y);
  EXPECT_DOUBLE_EQ(50, child_x);
  EXPECT_DOUBLE_EQ(50, child_y);

  ParentCoordToChildCoord(0, 0, 0, 0, 50, 50, M_PI_2, &child_x, &child_y);
  EXPECT_NEAR(50., child_x, kErrorDelta);
  EXPECT_DOUBLE_EQ(50., child_y);

  ParentCoordToChildCoord(0, 0, 0, 0, 50, 50, M_PI, &child_x, &child_y);
  EXPECT_DOUBLE_EQ(50., child_x);
  EXPECT_DOUBLE_EQ(50., child_y);

  ParentCoordToChildCoord(0, 0, 0, 0, 50, 50, M_PI + M_PI_2,
                            &child_x, &child_y);
  EXPECT_DOUBLE_EQ(50., child_x);
  EXPECT_DOUBLE_EQ(50., child_y);

  ParentCoordToChildCoord(0, 0, 0, 0, 50, 50, 2 * M_PI, &child_x, &child_y);
  EXPECT_DOUBLE_EQ(50., child_x);
  EXPECT_DOUBLE_EQ(50., child_y);

  for (int i = 0; i < 360; i++) {
    ParentCoordToChildCoord(i, i, 0, 0, 0, 0, 0, &child_x, &child_y);
    EXPECT_DOUBLE_EQ(i, child_x);
    EXPECT_DOUBLE_EQ(i, child_y);

    ParentCoordToChildCoord(0, 0, i, i, 0, 0, 0, &child_x, &child_y);
    EXPECT_DOUBLE_EQ(-i, child_x);
    EXPECT_DOUBLE_EQ(-i, child_y);

    ParentCoordToChildCoord(0, 0, 0, 0, i, i, 0, &child_x, &child_y);
    EXPECT_DOUBLE_EQ(i, child_x);
    EXPECT_DOUBLE_EQ(i, child_y);

    // distance should be constant in a circular rotation around origin
    ParentCoordToChildCoord(100, 100, 0, 0, 0, 0, DegreesToRadians(i),
                            &child_x, &child_y);
    EXPECT_DOUBLE_EQ(20000., child_x * child_x + child_y * child_y);

    // distance should be constant in a circular rotation around top-left
    ParentCoordToChildCoord(0, 0, 100, 100, 0, 0, DegreesToRadians(i),
                            &child_x, &child_y);
    EXPECT_DOUBLE_EQ(20000., child_x * child_x + child_y * child_y);

    // distance to pin should be constant in a circular rotation
    ParentCoordToChildCoord(0, 0, 0, 0, 1, 1, DegreesToRadians(i),
                            &child_x, &child_y);
    EXPECT_NEAR(0., (child_x - 1) * (child_x - 1) +
                    (child_y - 1) * (child_y - 1), kErrorDelta);
  }
}

TEST(MathUtilsTest, GetParentCoord) {
  double parent_x, parent_y;

  ChildCoordToParentCoord(40, 50, 0, 0, 40, 50, 0, &parent_x, &parent_y);
  EXPECT_NEAR(0, parent_x, kErrorDelta);
  EXPECT_NEAR(0, parent_y, kErrorDelta);

  ChildCoordToParentCoord(40, 50, 0, 0, 40, 50, M_PI_2,
                          &parent_x, &parent_y);
  EXPECT_NEAR(0, parent_x, kErrorDelta);
  EXPECT_NEAR(0, parent_y, kErrorDelta);

  ChildCoordToParentCoord(40, 50, 0, 0, 40, 50, M_PI, &parent_x, &parent_y);
  EXPECT_NEAR(0, parent_x, kErrorDelta);
  EXPECT_NEAR(0, parent_y, kErrorDelta);

  ChildCoordToParentCoord(40, 50, 0, 0, 40, 50, M_PI + M_PI_2,
                          &parent_x, &parent_y);
  EXPECT_NEAR(0, parent_x, kErrorDelta);
  EXPECT_NEAR(0, parent_y, kErrorDelta);

  ChildCoordToParentCoord(40, 50, 0, 0, 40, 50, 2 * M_PI,
                          &parent_x, &parent_y);
  EXPECT_NEAR(0, parent_x, kErrorDelta);
  EXPECT_NEAR(0, parent_y, kErrorDelta);

  for (int i = 0; i < 360; i++) {
    ChildCoordToParentCoord(i, i, 0, 0, 0, 0, 0, &parent_x, &parent_y);
    EXPECT_DOUBLE_EQ(i, parent_x);
    EXPECT_DOUBLE_EQ(i, parent_y);

    ChildCoordToParentCoord(0, 0, i, i, 0, 0, 0, &parent_x, &parent_y);
    EXPECT_DOUBLE_EQ(i, parent_x);
    EXPECT_DOUBLE_EQ(i, parent_y);

    ChildCoordToParentCoord(0, 0, 0, 0, i, i, 0, &parent_x, &parent_y);
    EXPECT_DOUBLE_EQ(-i, parent_x);
    EXPECT_DOUBLE_EQ(-i, parent_y);

    // distance should be constant in a circular rotation around origin
    ChildCoordToParentCoord(100, 100, 0, 0, 0, 0, DegreesToRadians(i),
                            &parent_x, &parent_y);
    EXPECT_DOUBLE_EQ(20000., parent_x * parent_x + parent_y * parent_y);

    // distance should be constant in a circular rotation around top-left
    ChildCoordToParentCoord(0, 0, 100, 100, 0, 0, DegreesToRadians(i),
                            &parent_x, &parent_y);
    EXPECT_DOUBLE_EQ(20000., parent_x * parent_x + parent_y * parent_y);

    // distance to pin should be constant in a circular rotation
    ChildCoordToParentCoord(0, 0, 0, 0, 1, 1, DegreesToRadians(i),
                            &parent_x, &parent_y);
    EXPECT_NEAR(2., parent_x * parent_x + parent_y * parent_y, kErrorDelta);
  }
}

TEST(MathUtilsTest, TestBackAndForth) {
  const double child_x_pos = 25;
  const double child_y_pos = 48;
  const double pin_x = 77;
  const double pin_y = 71;
  const double parent_x = 123.4;
  const double parent_y = 432.1;

  for (int i = 0; i < 360; i++) {
    double child_x, child_y;
    ParentCoordToChildCoord(parent_x, parent_y, child_x_pos, child_y_pos,
                            pin_x, pin_y, DegreesToRadians(i),
                            &child_x, &child_y);
    double parent_x1, parent_y1;
    ChildCoordToParentCoord(child_x, child_y, child_x_pos, child_y_pos,
                            pin_x, pin_y, DegreesToRadians(i),
                            &parent_x1, &parent_y1);
    EXPECT_NEAR(parent_x, parent_x1, kErrorDelta);
    EXPECT_NEAR(parent_y, parent_y1, kErrorDelta);
  }
}

TEST(MathUtilsTest, CheckPointInElement) {
  EXPECT_TRUE(IsPointInElement(0, 0, 50, 20));
  EXPECT_TRUE(IsPointInElement(1, 1, 50, 20));
  EXPECT_TRUE(IsPointInElement(49.9, 19.9, 50, 20));
  EXPECT_FALSE(IsPointInElement(-5, 0, 50, 20));
  EXPECT_FALSE(IsPointInElement(0, -5, 50, 20));
  EXPECT_FALSE(IsPointInElement(0, 30, 50, 20));
  EXPECT_FALSE(IsPointInElement(60, 0, 50, 20));
}

TEST(MathUtilsTest, DegreesToRadians) {
  EXPECT_EQ(2 * M_PI, DegreesToRadians(360.));
  EXPECT_EQ(0., DegreesToRadians(0.));
  EXPECT_EQ(M_PI, DegreesToRadians(180.));
}

TEST(MathUtilsTest, GetChildExtentInParent) {
  double extent_width, extent_height;
  GetChildExtentInParent(40, 50, 0, 0, 7, 8, 0, &extent_width, &extent_height);
  EXPECT_EQ(47, extent_width);
  EXPECT_EQ(58, extent_height);
  GetChildExtentInParent(40, 50, 3, 4, 7, 8, 0, &extent_width, &extent_height);
  EXPECT_EQ(44, extent_width);
  EXPECT_EQ(54, extent_height);
  // TODO: Add more tests.
}

TEST(MathUtilsTest, RectangleGetPolygonExtents) {
  const double p1[] = {0, 0, 0, 1, 1, 1, 1, 0};
  const double p2[] = {0.4, 2, 0.6, 2, 0.6, -2, 0.4, -2};
  const double p3[] = {0.5, 1.5, 1.5, 0.5, 0.5, -0.5, -0.5, 0.5};

  ggadget::Rectangle rect;
  rect = ggadget::Rectangle::GetPolygonExtents(4, p1);
  EXPECT_DOUBLE_EQ(0, rect.x);
  EXPECT_DOUBLE_EQ(0, rect.y);
  EXPECT_DOUBLE_EQ(1, rect.w);
  EXPECT_DOUBLE_EQ(1, rect.h);
  rect = ggadget::Rectangle::GetPolygonExtents(4, p2);
  EXPECT_DOUBLE_EQ(0.4, rect.x);
  EXPECT_DOUBLE_EQ(-2, rect.y);
  EXPECT_DOUBLE_EQ(0.2, rect.w);
  EXPECT_DOUBLE_EQ(4, rect.h);
  rect = ggadget::Rectangle::GetPolygonExtents(4, p3);
  EXPECT_DOUBLE_EQ(-0.5, rect.x);
  EXPECT_DOUBLE_EQ(-0.5, rect.y);
  EXPECT_DOUBLE_EQ(2, rect.w);
  EXPECT_DOUBLE_EQ(2, rect.h);
}

TEST(MathUtilsTest, RectangleUnion) {
  ggadget::Rectangle r1(2, 2, 2, 3);
  ggadget::Rectangle r2(0, 1, 2, 1);
  r1.Union(r2);
  EXPECT_DOUBLE_EQ(0, r1.x);
  EXPECT_DOUBLE_EQ(1, r1.y);
  EXPECT_DOUBLE_EQ(4, r1.w);
  EXPECT_DOUBLE_EQ(4, r1.h);
}

TEST(MathUtilsTest, RectangleOverlaps) {
  ggadget::Rectangle r1(2, 2, 2, 3);
  ggadget::Rectangle r2(0, 1, 2, 1);
  ggadget::Rectangle r3(1, 1, 4, 3);
  ggadget::Rectangle r4(6, 6, 1, 1);
  EXPECT_TRUE(r1.Overlaps(r1));
  EXPECT_TRUE(r1.Overlaps(r3));
  EXPECT_FALSE(r1.Overlaps(r2));
  EXPECT_FALSE(r1.Overlaps(r4));
  EXPECT_TRUE(r2.Overlaps(r3));
  EXPECT_FALSE(r2.Overlaps(r1));
  EXPECT_FALSE(r2.Overlaps(r4));
  EXPECT_FALSE(r3.Overlaps(r4));
}

TEST(MathUtilsTest, RectangleIntersect) {
  ggadget::Rectangle r1(2, 2, 2, 3);
  ggadget::Rectangle r2(0, 1, 2, 1);
  ggadget::Rectangle r3(1, 1, 4, 3);
  ggadget::Rectangle r4 = r1;
  EXPECT_FALSE(r1.Intersect(r2));
  EXPECT_TRUE(r4 == r1);
  EXPECT_TRUE(r1.Intersect(r1));
  EXPECT_TRUE(r4 == r1);
  EXPECT_TRUE(r1.Intersect(r3));
  EXPECT_DOUBLE_EQ(2, r1.x);
  EXPECT_DOUBLE_EQ(2, r1.y);
  EXPECT_DOUBLE_EQ(2, r1.w);
  EXPECT_DOUBLE_EQ(2, r1.h);
}

TEST(MathUtilsTest, RectangleIsInside) {
  ggadget::Rectangle r1(1, 1, 4, 4);
  ggadget::Rectangle r2(1, 1, 4, 4);
  ggadget::Rectangle r3(1.5, 1.5, 3, 3);
  ggadget::Rectangle r4(1, 2, 4, 3);
  ggadget::Rectangle r5(0.5, 1, 3, 2);

  EXPECT_FALSE(r1.IsInside(r3));
  EXPECT_TRUE(r1.IsInside(r1));
  EXPECT_TRUE(r3.IsInside(r1));
  EXPECT_TRUE(r4.IsInside(r1));
  EXPECT_FALSE(r5.IsInside(r1));
}

int main(int argc, char **argv) {
  testing::ParseGTestFlags(&argc, argv);

  return RUN_ALL_TESTS();
}
