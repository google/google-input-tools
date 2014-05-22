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

#ifndef GOOPY_SKIN_COMPOSITION_ELEMENT_H__
#define GOOPY_SKIN_COMPOSITION_ELEMENT_H__

#include <string>
#include <vector>

#include "third_party/google_gadgets_for_linux/ggadget/basic_element.h"
#include "third_party/google_gadgets_for_linux/ggadget/scriptable_interface.h"

namespace ggadget {
struct TextFormatRange;
typedef std::vector<TextFormatRange> TextFormats;
}  // namespace ggadget

namespace ime_goopy {
namespace skin {

// An element to show composition string.
class CompositionElement : public ggadget::BasicElement {
 public:
  DEFINE_CLASS_ID(0x848a2f5e84144988, ggadget::BasicElement);

  /** Enums to specify clause */
  enum ClauseStatus {
    ACTIVE = 0,
    INACTIVE,
    CONVERTED,
    HIGHLIGHT,
  };

  enum TextAttribute {
    TEXT_BOLD = 0,
    TEXT_ITALIC,
    TEXT_STRIKEOUT,
    TEXT_UNDERLINE,
    TEXT_COLOR,
    TEXT_FONT,
    TEXT_SIZE,
  };

  CompositionElement(ggadget::View *view, const char *name);
  virtual ~CompositionElement();

  void SetCompositionText(const std::string &text);
  void SetCompositionFormats(const ggadget::TextFormats &composition);
  void SetCompositionStatus(int start, int end, ClauseStatus status);

  // Reset composition.
  void Clear();

  // Set Clause TextAttribute
  void SetClauseTextAttribute(ClauseStatus clause_status,
                              TextAttribute text_attr,
                              const ggadget::Variant &value);
  ggadget::Variant GetClauseTextAttribute(ClauseStatus clause_status,
                                          TextAttribute text_attr) const;
  // Set SegmentationLabel text attribute.
  void SetSegmentationLabelTextAttribute(TextAttribute text_attr,
                                         const ggadget::Variant &value);
  ggadget::Variant GetSegmentataionLabelTextAttribute(
      TextAttribute text_attr) const;
  // If all the text are appended or TextAttribute is set or any UI attribute
  // has been changed, call this function to notify this control to update UI.
  void UpdateUI();

  // 'caret_pos' should be '0 - clause size'.
  // The caret is supposed to follow a clause which means caret will not
  // appear in the middle of a clause.
  // If its value is 0 means the caret is ahead of all clauses.
  void SetCaretPosition(int caret_pos);
  int GetCaretPosition() const;

  // Color only contain RGB, not contain opacity.
  void SetCaretColor(const std::string &color);
  std::string GetCaretColor() const;

  bool IsHorizontalAutoSizing() const;
  void SetHorizontalAutoSizing(bool auto_sizing);

  bool IsVerticalAutoSizing() const;
  void SetVerticalAutoSizing(bool auto_sizing);

  virtual double GetMinWidth() const;
  virtual double GetMinHeight() const;

  static ggadget::BasicElement *CreateInstance(ggadget::View *view,
                                               const char *name);

 protected:
  virtual void DoClassRegister();
  virtual void DoDraw(ggadget::CanvasInterface *canvas);
  virtual void CalculateSize();

 private:
  class Impl;
  Impl *impl_;

  DISALLOW_EVIL_CONSTRUCTORS(CompositionElement);
};

}  // namespace skin
}  // namespace ime_goopy

#endif  // GOOPY_SKIN_COMPOSITION_ELEMENT_H__
