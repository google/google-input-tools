/*
  Copyright 2012 Google Inc.

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

// The file contains the implementation of the font interface on Core Text

#ifndef GGADGET_MAC_CT_FONT_H_
#define GGADGET_MAC_CT_FONT_H_

#include <CoreText/CoreText.h>
#include <string>

#include "ggadget/common.h"
#include "ggadget/font_interface.h"
#include "ggadget/mac/scoped_cftyperef.h"

namespace ggadget {

class TextFormat;

} // ggadget

namespace ggadget {
namespace mac {

class CtFont : public FontInterface {
 public:
  CtFont();
  bool Init(const std::string& font_family,
            double size,
            Style style,
            Weight weight);
  virtual ~CtFont();
  virtual Style GetStyle() const {
    return style_;
  }
  virtual Weight GetWeight() const {
    return weight_;
  }
  virtual double GetPointSize() const {
    return size_;
  }
  const std::string& GetFontFamily() const {
    return font_family_;
  }
  virtual void Destroy() {
    delete this;
  }
  CTFontRef GetFont() const {
    return font_.get();
  }

 private:
  ScopedCFTypeRef<CTFontRef> font_;
  std::string font_family_;
  double size_;
  Style style_;
  Weight weight_;

  DISALLOW_EVIL_CONSTRUCTORS(CtFont);
};

} // namespace mac
} // namespace ggadget

#endif // GGADGET_MAC_CT_FONT_H_
