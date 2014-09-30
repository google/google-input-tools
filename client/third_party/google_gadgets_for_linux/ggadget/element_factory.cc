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

#include "element_factory.h"
#include "common.h"
#include "basic_element.h"
#include "view.h"
#include "string_utils.h"

#include "anchor_element.h"
#include "button_element.h"
#include "checkbox_element.h"
#include "combobox_element.h"
#include "contentarea_element.h"
#include "copy_element.h"
#include "div_element.h"
#include "img_element.h"
#include "item_element.h"
#include "label_element.h"
#include "linear_element.h"
#include "listbox_element.h"
#include "object_element.h"
#include "object_videoplayer.h"
#include "progressbar_element.h"
#include "scrollbar_element.h"
#include "small_object.h"

namespace ggadget {

class ElementFactory::Impl : public SmallObject<> {
 public:
  BasicElement *CreateElement(const char *tag_name,
                              View *view,
                              const char *name) {
    CreatorMap::iterator ite = creators_.find(tag_name);
    if (ite == creators_.end())
      return NULL;
    return ite->second(view, name);
  }

  bool RegisterElementClass(const char *tag_name,
                            ElementFactory::ElementCreator creator) {
    CreatorMap::iterator ite = creators_.find(tag_name);
    if (ite != creators_.end())
      return false;
    creators_[tag_name] = creator;
    return true;
  }

  typedef LightMap<const char *, ElementFactory::ElementCreator,
                   GadgetCharPtrComparator> CreatorMap;

  CreatorMap creators_;
};

ElementFactory::ElementFactory()
    : impl_(new Impl) {
  RegisterElementClass("a", &AnchorElement::CreateInstance);
  RegisterElementClass("button", &ButtonElement::CreateInstance);
  RegisterElementClass("div", &DivElement::CreateInstance);
  RegisterElementClass("img", &ImgElement::CreateInstance);
  RegisterElementClass("label", &LabelElement::CreateInstance);
  RegisterElementClass("linear", &LinearElement::CreateInstance);
  // TODO(zkfan): support more kinds of elements
#if defined(OS_POSIX) && !defined(GGL_FOR_GOOPY)
  RegisterElementClass("checkbox",
                       &CheckBoxElement::CreateCheckBoxInstance);
  RegisterElementClass("combobox", &ComboBoxElement::CreateInstance);
  RegisterElementClass("contentarea", &ContentAreaElement::CreateInstance);
  // Internal element. Don't add it for now.
  // RegisterElementClass("_copy", &CopyElement::CreateInstance);
  RegisterElementClass("item", &ItemElement::CreateInstance);
  RegisterElementClass("listbox", &ListBoxElement::CreateInstance);
  RegisterElementClass("listitem", &ItemElement::CreateListItemInstance);
  RegisterElementClass("object", &ObjectElement::CreateInstance);
  // Video player object hosted by the object element.
  RegisterElementClass("clsid:6BF52A52-394A-11d3-B153-00C04F79FAA6",
                       &ObjectVideoPlayer::CreateInstance);
  RegisterElementClass("progid:WMPlayer.OCX.7",
                       &ObjectVideoPlayer::CreateInstance);
  RegisterElementClass("progid:WMPlayer.OCX",
                       &ObjectVideoPlayer::CreateInstance);
  RegisterElementClass("progressbar", &ProgressBarElement::CreateInstance);
  RegisterElementClass("radio", &CheckBoxElement::CreateRadioInstance);
  RegisterElementClass("scrollbar", &ScrollBarElement::CreateInstance);
#endif
}

ElementFactory::~ElementFactory() {
  delete impl_;
}

BasicElement *ElementFactory::CreateElement(const char *tag_name,
                                            View *view,
                                            const char *name) {
  ASSERT(impl_);
  return impl_->CreateElement(tag_name, view, name);
}

bool ElementFactory::RegisterElementClass(const char *tag_name,
                                          ElementCreator creator) {
  ASSERT(impl_);
  return impl_->RegisterElementClass(tag_name, creator);
}

} // namespace ggadget
