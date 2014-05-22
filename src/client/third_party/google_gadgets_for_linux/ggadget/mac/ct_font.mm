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

#include "ggadget/mac/ct_font.h"

#include "ggadget/text_formats.h"

namespace {

using ggadget::mac::ScopedCFTypeRef;

double PointToPixel(double size) {
  // TODO: figure out a better way to convert point to pixel
  return ceil(size * 4 / 3);
}

bool SetFamily(CFMutableDictionaryRef attributes,
               const std::string& font_family) {
  ScopedCFTypeRef<CFStringRef> family(CFStringCreateWithCString(
      NULL, font_family.c_str(), kCFStringEncodingUTF8));
  if (!family.get()) {
    return false;
  }
  CFDictionaryAddValue(attributes,
                       kCTFontFamilyNameAttribute,
                       family.get());
  return true;
}

bool SetSize(CFMutableDictionaryRef attributes, double size) {
  double *pixel_size = new double(PointToPixel(size));
  ScopedCFTypeRef<CFNumberRef> font_size(CFNumberCreate(
      NULL /* allocator */,
      kCFNumberDoubleType,
      &pixel_size));
  if (!font_size.get()) {
    return false;
  }
  CFDictionaryAddValue(attributes,
                       kCTFontSizeAttribute,
                       font_size.get());
  return true;
}

bool SetSymbolicTraits(CFMutableDictionaryRef attributes,
                       CTFontSymbolicTraits value) {
  ScopedCFTypeRef<CFMutableDictionaryRef> traits(CFDictionaryCreateMutable(
      kCFAllocatorDefault, 0,
      &kCFTypeDictionaryKeyCallBacks,
      &kCFTypeDictionaryValueCallBacks));
  if (!traits.get()) {
    return false;
  }
  ScopedCFTypeRef<CFNumberRef> symbolic_traits(CFNumberCreate(
      NULL /* allocator */,
      kCFNumberSInt32Type,
      &value));
  if (!symbolic_traits.get()) {
    return false;
  }
  CFDictionaryAddValue(traits.get(),
                       kCTFontSymbolicTrait,
                       symbolic_traits.get());
  CFDictionaryAddValue(attributes,
                       kCTFontTraitsAttribute,
                       traits.get());
  return true;
}

} // namespace

namespace ggadget {
namespace mac {

CtFont::CtFont()
    : font_(NULL),
      font_family_(""),
      size_(0),
      style_(STYLE_NORMAL),
      weight_(WEIGHT_NORMAL) {
}

CtFont::~CtFont() {
}

bool CtFont::Init(const std::string& font_family,
                  double size,
                  Style style,
                  Weight weight) {
  size_ = size;
  style_ = style;
  weight_ = weight;
  font_family_ = font_family;

  ScopedCFTypeRef<CFMutableDictionaryRef> attributes(CFDictionaryCreateMutable(
      kCFAllocatorDefault, 0,
      &kCFTypeDictionaryKeyCallBacks,
      &kCFTypeDictionaryValueCallBacks));
  if (!attributes.get()) {
    return false;
  }

  if (!SetFamily(attributes.get(), font_family)) {
    return false;
  }

  if (!SetSize(attributes.get(), size)) {
    return false;
  }

  // Sets style and weight
  CTFontSymbolicTraits symbolic_traits_value = 0;
  if (style == STYLE_ITALIC) {
    symbolic_traits_value |= kCTFontItalicTrait;
  }
  if (weight == WEIGHT_BOLD) {
    symbolic_traits_value |= kCTFontBoldTrait;
  }

  if (!SetSymbolicTraits(attributes.get(), symbolic_traits_value)) {
    return false;
  }

  ScopedCFTypeRef<CTFontDescriptorRef> descriptor(
      CTFontDescriptorCreateWithAttributes(attributes.get()));
  if (!descriptor.get()) {
    return false;
  }

  font_.reset(CTFontCreateWithFontDescriptor(
      descriptor.get(), PointToPixel(size), NULL /* matrix */));
  return font_.get() != NULL;
}

} // namespace mac
} // namespace ggadget
