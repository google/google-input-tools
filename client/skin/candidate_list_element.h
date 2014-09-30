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

#ifndef GOOPY_SKIN_CANDIDATE_LIST_ELEMENT_H_
#define GOOPY_SKIN_CANDIDATE_LIST_ELEMENT_H_

#include <string>

#include "third_party/google_gadgets_for_linux/ggadget/basic_element.h"
#include "third_party/google_gadgets_for_linux/ggadget/common.h"
#include "third_party/google_gadgets_for_linux/ggadget/text_formats.h"

namespace ggadget {

class CanvasInterface;
class Connection;
class Variant;
class View;
struct Color;

template <typename R, typename P1, typename P2> class Slot2;

}  // namespace ggadget

namespace ime_goopy {
namespace skin {

class CandidateElement;

// This class implements ggadget UI element for candidate list.
class CandidateListElement : public ggadget::BasicElement {
 public:
  DEFINE_CLASS_ID(0xfe20350c5bcf12dc, ggadget::BasicElement);

  // Candidate layout standing style.
  enum Orientation {
    ORIENTATION_HORIZONTAL = 0,
    ORIENTATION_VERTICAL,
  };

  CandidateListElement(ggadget::View* view, const char* name);
  virtual ~CandidateListElement();

  Orientation GetOrientation() const;
  void SetOrientation(Orientation orientation);

  //////////////////////////////////////////////////////////////////////////////
  // Properties about UI Style of candidate element.
  //    font:
  //    - Font of candidate text.
  //    font_size:
  //    - Font size of candidate text.
  //    color:
  //    - Color of candidate text.
  //    selected text color:
  //    - Color of candidate text.
  //    bold:
  //    - True if candidate text is bold.
  //    italic:
  //    - True if candidate text is italic.
  //    underline:
  //    - True if candidate text is underline.
  //    strikeout:
  //    - True if candidate text is strikeout.
  //    menu width:
  //    - Width of candidate menu button.
  //    menu height:
  //    - Height of candidate menu button.
  //    menu  image:
  //    - TYPE_STRING type image path, image displayed in  state.
  //    menu over image;
  //    - TYPE_STRING type image path, image displayed when mouse over.
  //    down image;
  //    - TYPE_STRING type image path, image displayed when mouse down.
  //     menu image in selection mode:
  //    - TYPE_STRING type image path, image displayed in  state in
  //    selection mode.
  //    menu mouse over effect image in selection mode;
  //    - TYPE_STRING type image path, image displayed when mouse over in
  //    selection mode.
  //    menu mouse down effect image in selection mode;
  //    - TYPE_STRING type image path, image displayed when mouse down in
  //    selection mode.
  //////////////////////////////////////////////////////////////////////////////
  ggadget::Variant GetCandidateColor() const;
  void SetCandidateColor(const ggadget::Variant& color);

  ggadget::Variant GetSelectedCandidateColor() const;
  void SetSelectedCandidateColor(const ggadget::Variant& color);

  std::string GetCandidateFont() const;
  void SetCandidateFont(const char* font);

  double GetCandidateSize() const;
  void SetCandidateSize(double size);

  bool IsCandidateBold() const;
  void SetCandidateBold(bool bold);

  bool IsCandidateItalic() const;
  void SetCandidateItalic(bool italic);

  bool IsCandidateStrikeout() const;
  void SetCandidateStrikeout(bool strikeout);

  bool IsCandidateUnderline() const;
  void SetCandidateUnderline(bool underline);

  double GetCandidateMenuWidth() const;
  void SetCandidateMenuWidth(double width);

  double GetCandidateMenuHeight() const;
  void SetCandidateMenuHeight(double height);

  ggadget::Variant GetCandidateMenuIcon() const;
  void SetCandidateMenuIcon(const ggadget::Variant& img);

  ggadget::Variant GetCandidateMenuDownIcon() const;
  void SetCandidateMenuDownIcon(const ggadget::Variant& img);

  ggadget::Variant GetCandidateMenuOverIcon() const;
  void SetCandidateMenuOverIcon(const ggadget::Variant& img);

  ggadget::Variant GetSelectedCandidateMenuIcon() const;
  void SetSelectedCandidateMenuIcon(const ggadget::Variant& img);

  ggadget::Variant GetSelectedCandidateMenuDownIcon() const;
  void SetSelectedCandidateMenuDownIcon(const ggadget::Variant& img);

  ggadget::Variant GetSelectedCandidateMenuOverIcon() const;
  void SetSelectedCandidateMenuOverIcon(const ggadget::Variant& img);

  // Selection image.
  ggadget::Variant GetSelectionImage() const;
  void SetSelectionImage(const ggadget::Variant& img);

  // Put selected cover image on top or bottom.
  bool IsSelectionImageOnTop() const;
  void SetSelectionImageOnTop(bool ontop);

  // Selected image may want to cover a rectangle bigger than a given
  // area(in most cases its candidate element rectangle.
  // those two properties provide expected x/y margins for a selection image,
  // the margin could be negtive, which indicates the image wants to cover an
  // area smaller than given rectangle(in most cases a candidate element
  // rectangle).
  //
  //  Parameters.
  // - margin: TYPE_STRING, 4 double type numbers concated by space.
  // which are left_width, top_height, right_width, bottom_height.
  void GetSelectionImageMargin(double* left, double* top,
                               double* right, double* bottom) const;
  void SetSelectionImageMargin(double left, double top,
                               double right, double bottom);

  // Selection image Border set four borders to control how selected effect
  // image is displayed in stretch middle mode.
  void GetSelectionImageStretchBorder(
      double* left, double* top, double* right, double* bottom) const;
  void SetSelectionImageStretchBorder(
      double left, double top, double right, double bottom);

  // Selected candidate id.
  // - id == 0 means no selection.
  uint32_t GetSelectedCandidate() const;
  void SetSelectedCandidate(uint32_t id);

  //  Checks/sets if this element's width will be adjusted automatically
  // according to its children's size.
  bool IsHorizontalAutoSizing() const;
  void SetHorizontalAutoSizing(bool auto_sizing);

  // Checks/sets if this element's height will be adjusted automatically
  // according to its children's size.
  bool IsVerticalAutoSizing() const;
  void SetVerticalAutoSizing(bool auto_sizing);

  // Setter and getter for candidate default width/height.
  bool IsVerticalLayoutCandidateAligned() const;
  void SetVerticalLayoutCandidateAligned(bool aligned);

  // Setter and getter for padding between candidates.
  double GetHorizontalPadding() const;
  void SetHorizontalPadding(double padding);

  double GetVerticalPadding() const;
  void SetVerticalPadding(double padding);

  // Append one candidate to the end of all candidates.
  CandidateElement* AppendCandidate(
      uint32_t id, const std::string& text);
  // Append one candidate with specific font and color to the end of candidates.
  CandidateElement* AppendCandidateWithFormat(
      uint32_t id,
      const std::string& text,
      const ggadget::TextFormats& formats);
  // Remove all candidates from candidate list.
  void RemoveAllCandidates();

  // Connect a slot called when a candidate is clicked or right-clicked.
  ggadget::Connection* ConnectOnCandidateSelected(
      ggadget::Slot2<void, uint32_t, bool>* handler);
  // Connect a slot called when a candidate button menu should be popped-out.
  ggadget::Connection* ConnectOnShowCandidateContextMenu(
      ggadget::Slot2<void, uint32_t, ggadget::MenuInterface*>* handler);

  static ggadget::BasicElement* CreateInstance(
      ggadget::View* view, const char* name);

  // Overridden from BasicElement:
  virtual double GetMinWidth() const;
  virtual double GetMinHeight() const;

 private:
  class Impl;

  virtual void DoClassRegister();
  virtual void CalculateSize();
  virtual void BeforeChildrenLayout();
  virtual void Layout();
  virtual void DoDraw(ggadget::CanvasInterface* canvas);

  Impl* impl_;
  DISALLOW_EVIL_CONSTRUCTORS(CandidateListElement);
};

}  // namespace skin
}  // namespace ime_goopy

#endif  // GOOPY_SKIN_CANDIDATE_LIST_ELEMENT_H_
