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

#include "scriptable_menu.h"

#include <vector>
#include "basic_element.h"
#include "gadget_interface.h"
#include "math_utils.h"
#include "menu_interface.h"
#include "slot.h"
#include "small_object.h"
#include "view.h"

namespace ggadget {

class ScriptableMenu::Impl : public SmallObject<> {
 public:
  class MenuItemSlot : public Slot1<void, const char *> {
   public:
    MenuItemSlot(ScriptableMenu *owner, GadgetInterface *gadget, Slot *handler)
        : owner_(owner), gadget_(gadget), handler_(handler) {
      // Let the slot hold a reference to its owner to prevent the owner
      // from being deleted before end-of-life of this slot.
      owner_->Ref();
    }
    virtual ~MenuItemSlot() {
      delete handler_;
      handler_ = NULL;
      owner_->Unref();
      owner_ = NULL;
    }
    virtual ResultVariant Call(ScriptableInterface *object,
                               int argc, const Variant argv[]) const {
      ASSERT(argc == 1);
      if (gadget_)
        gadget_->SetInUserInteraction(true);
      ResultVariant result = handler_->Call(object, argc, argv);
      if (gadget_)
        gadget_->SetInUserInteraction(false);
      return result;
    }
    virtual bool operator==(const Slot &another) const {
      GGL_UNUSED(another);
      // Not used.
      return false;
    }

   private:
    ScriptableMenu *owner_;
    GadgetInterface *gadget_;
    Slot *handler_;
  };

  Impl(ScriptableMenu *owner, GadgetInterface *gadget, MenuInterface *menu)
      : owner_(owner), gadget_(gadget), menu_(menu) {
    ASSERT(menu);
  }

  ~Impl() {
    for (std::vector<ScriptableMenu *>::iterator it = submenus_.begin();
         it != submenus_.end(); ++it) {
      (*it)->Unref();
    }
    submenus_.clear();
  }

  void AddItem(const char *item_text, int style, Slot *handler) {
    // TODO: support stock icon.
    menu_->AddItem(item_text, style, 0,
                   handler ? new MenuItemSlot(owner_, gadget_, handler) : NULL,
                   MenuInterface::MENU_ITEM_PRI_CLIENT);
  }

  ScriptableMenu *AddPopup(const char *popup_text) {
    ScriptableMenu *submenu = new ScriptableMenu(gadget_,
            menu_->AddPopup(popup_text, MenuInterface::MENU_ITEM_PRI_CLIENT));
    submenu->Ref();
    submenus_.push_back(submenu);
    return submenu;
  }

  void SetItemStyle(const char *item_text, int style) {
    menu_->SetItemStyle(item_text, style);
  }

  ScriptableMenu *owner_;
  GadgetInterface *gadget_;
  MenuInterface *menu_;
  std::vector<ScriptableMenu *> submenus_;
};

ScriptableMenu::ScriptableMenu(GadgetInterface *gadget, MenuInterface *menu)
  : impl_(new Impl(this, gadget, menu)) {
}

void ScriptableMenu::DoClassRegister() {
  RegisterMethod("AddItem",
                 NewSlot(&Impl::AddItem, &ScriptableMenu::impl_));
  RegisterMethod("SetItemStyle",
                 NewSlot(&Impl::SetItemStyle, &ScriptableMenu::impl_));
  RegisterMethod("AddPopup",
                 NewSlot(&Impl::AddPopup, &ScriptableMenu::impl_));
  RegisterMethod("setPositionHint",
                 NewSlot(&ScriptableMenu::SetPositionHint));
}

ScriptableMenu::~ScriptableMenu() {
  delete impl_;
  impl_ = NULL;
}

MenuInterface *ScriptableMenu::GetMenu() const {
  return impl_->menu_;
}

void ScriptableMenu::SetPositionHint(const BasicElement *element) {
  if (element && element->GetView()->GetGadget() == impl_->gadget_ &&
      element->IsReallyVisible()) {
    Rectangle r = element->GetExtentsInView();
    const View *view = element->GetView();
    double left, top, right, bottom;
    view->ViewCoordToNativeWidgetCoord(r.x, r.y, &left, &top);
    view->ViewCoordToNativeWidgetCoord(r.x + r.w, r.y + r.h, &right, &bottom);
    impl_->menu_->SetPositionHint(
        Rectangle(left, top, right - left, bottom - top));
  }
}

} // namespace ggadget
