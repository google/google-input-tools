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

#ifndef GOOPY_SKIN_TOOLBAR_ELEMENT_H_
#define GOOPY_SKIN_TOOLBAR_ELEMENT_H_

// Include basictypes.h before headers of ggadget to avoid macro conflicts.
#include "base/basictypes.h"
#include "third_party/google_gadgets_for_linux/ggadget/linear_element.h"
#include "third_party/google_gadgets_for_linux/ggadget/scoped_ptr.h"

namespace ggadget {
class BasicElement;
class CanvasInterface;
class View;
}  // namespace ggadget

namespace ime_goopy {
namespace skin {

// Ggadget UI element for toolbar.
class ToolbarElement : public ggadget::LinearElement {
 public:
  DEFINE_CLASS_ID(0x2dbc5865700f504f, ggadget::LinearElement);

  // Button on toolbar has three visibility properties.
  enum ButtonVisibilityType {
    // The visibility of the button depend on the property collapsed of tooblar
    // element.
    NORMAL_VISIBILITY = 0,
    // The button should always be visible in any case.
    ALWAYS_VISIBLE,
    // The button should always be invisible in any case.
    ALWAYS_INVISIBLE,
  };

  ToolbarElement(ggadget::View* view, const char* name);
  virtual ~ToolbarElement();

  static ggadget::BasicElement* CreateInstance(
      ggadget::View* view, const char* name);

  // This method initializes the properties of children created by xml.
  void Initialize();

  // Adds one button to the end of all elements with direction specified.
  // Returns the appended element;
  ggadget::BasicElement* AddButton(
      const char* button_name,
      ggadget::LinearElement::LayoutDirection direction,
      ButtonVisibilityType visibility);
  // Removes a button by its name.
  // Returns true if the button was removed or hidden successfully; returns
  // false if the button is not found.
  // Since this method may not actually remove the button, the caller should
  // disconnect signal connections on the button before calling this method.
  bool RemoveButton(const char* button_name);

  // Returns true if the toolbar is collapsed.
  bool IsCollapsed();
  // Sets the property collapsed of toolbar element.
  void SetCollapsed(bool is_collapsed);

 protected:
  virtual void DoClassRegister();

 private:
  class Impl;
  ggadget::scoped_ptr<Impl> impl_;

  DISALLOW_COPY_AND_ASSIGN(ToolbarElement);
};

}  // namespace skin
}  // namespace ime_goopy

#endif  // GOOPY_SKIN_TOOLBAR_ELEMENT_H_
