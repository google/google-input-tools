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

#ifndef GOOPY_SKIN_CANDIDATE_ELEMENT_H_
#define GOOPY_SKIN_CANDIDATE_ELEMENT_H_

#include <string>

#include "third_party/google_gadgets_for_linux/ggadget/color.h"
#include "third_party/google_gadgets_for_linux/ggadget/common.h"
#include "third_party/google_gadgets_for_linux/ggadget/linear_element.h"
#include "third_party/google_gadgets_for_linux/ggadget/text_formats.h"
#include "third_party/google_gadgets_for_linux/ggadget/variant.h"
#include "third_party/google_gadgets_for_linux/ggadget/view.h"

namespace ggadget {

class BasicElement;
class CanvasInterface;
class MenuInterface;
class Variant;
class View;

template <typename R, typename P1, typename P2> class Slot2;

}  // namespace ggadget

namespace ime_goopy {
namespace skin {

// Ggadget UI element for one candidate.
class CandidateElement : public ggadget::LinearElement {
 public:
  DEFINE_CLASS_ID(0xfe70820c5bbf11dc, ggadget::LinearElement);

  CandidateElement(ggadget::View* view, const char* name);
  virtual ~CandidateElement();

  // Getter and setter for id of candidate.
  uint32_t GetId() const;
  void SetId(uint32_t id);

  // Getter and setter for text of candidate.
  std::string GetText() const;
  void SetText(const std::string& text);

  void SetFormats(const ggadget::TextFormats& formats);

  void SetDefaultFormat(const ggadget::TextFormat& default_format);

  //////////////////////////////////////////////////////////////////////////////
  // Properties about UI Style of candidate element.
  //    double menu_width;
  //    - Width of candidate menu icon, in pixels.
  //    double menu_height;
  //    - Height of candidate menu icon, in pixels.
  //    ggadget::Variant menu_image;
  //    - TYPE_STRING type image path, image displayed in normal state.
  //    ggadget::Variant menu_over_image;
  //    - TYPE_STRING type image path, image displayed when mouse over.
  //    ggadget::Variant menu_down_image;
  //    - TYPE_STRING type image path, image displayed when mouse down.
  //////////////////////////////////////////////////////////////////////////////

  // Getter and setters for each of those in CandidateStyle.
  double GetMenuWidth() const;
  void SetMenuWidth(double width);

  double GetMenuHeight() const;
  void SetMenuHeight(double Height);

  ggadget::Variant GetMenuIcon() const;
  void SetMenuIcon(const ggadget::Variant& img);

  ggadget::Variant GetMenuDownIcon() const;
  void SetMenuDownIcon(const ggadget::Variant& img);

  ggadget::Variant GetMenuOverIcon() const;
  void SetMenuOverIcon(const ggadget::Variant& img);

  // Connect a slot called when a candidate is clicked or right-clicked.
  void ConnectOnCandidateSelected(
      ggadget::Slot2<void, uint32_t, bool>* handler);
  // Connect a slot called when a candidate menu should be popped-out.
  void ConnectOnCandidateContextMenu(
      ggadget::Slot2<void, uint32_t, ggadget::MenuInterface*>* handler);

  static ggadget::BasicElement* CreateInstance(
      ggadget::View* view, const char* name);

 private:
  virtual void CalculateSize();
  virtual void DoClassRegister();

  class Impl;
  Impl* impl_;
  DISALLOW_EVIL_CONSTRUCTORS(CandidateElement);
};

}  // namespace skin
}  // namespace ime_goopy

#endif  // GOOPY_SKIN_CANDIDATE_ELEMENT_H_
