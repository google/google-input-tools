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

#include <QtGui/QFontMetrics>
#include "qt_canvas.h"
#include "qt_font.h"

namespace ggadget {
namespace qt {

QtFont::QtFont(const std::string &family, double size,
               Style style, Weight weight)
    : size_(size), style_(style), weight_(weight) {
  font_ = new QFont(family.c_str());
  if (size > 0) font_->setPointSizeF(size);
  if (weight == WEIGHT_BOLD)
    font_->setWeight(QFont::Bold);
  if (style == STYLE_ITALIC)
    font_->setItalic(true);
}

QtFont::~QtFont() {
  if (font_) delete font_;
}

} // namespace qt
} // namespace ggadget
